#include <QWidget>

#include "GraphicsManager.hpp"
#include "DrawObject.hpp"
#include "Grid.hpp"
#include "config.h"
#include "ImageUtils.hpp"
#include <vsgXchange/all.h>
#include <mars_utils/misc.h>


namespace mars
{
    using namespace utils;
    using namespace interfaces;

    namespace vsg_graphics
    {

        GraphicsManager::GraphicsManager(lib_manager::LibManager *theManager,
                                         void *QTWidget)
            : GraphicsManagerInterface(theManager), guiHelper{new GuiHelper{this}}
        {
            (void)QTWidget;
            dirty_ = true;
            vsg::Logger::instance()->level = vsg::Logger::LOGGER_INFO;
        }

        GraphicsManager::~GraphicsManager()
        {
            if(cfg)
            {
                auto configPath = cfg->getOrCreateProperty("Config", "config_path",
                                                           std::string{"."});
                std::string saveFile = configPath.sValue;
                saveFile.append("/mars_Graphics.yaml");
                cfg->writeConfig(saveFile.c_str(), "Graphics");
                libManager->releaseLibrary("cfg_manager");
            }
            for(auto it: drawObjects)
            {
                delete it.second;
            }

            for(auto &iter: windows)
            {
                delete iter.second;
            }
            delete guiHelper;
        }

        void GraphicsManager::initializeOSG(void *data, bool createWindow)
        {
            (void)data;
            if(!viewer) {
                cfg = libManager->getLibraryAs<cfg_manager::CFGManagerInterface>("cfg_manager");
                if(!cfg)
                {
                    fprintf(stderr, "******* mars_graphics: couldn't find cfg_manager\n");
                    return;
                }
                setupCFG();

                nextDrawID = 1;
                nextWindowID = 1;

                // todo: understand vsg options e. g. sharedObject and cache
                // auto options = vsg::Options::create();
                // options->sharedObjects = vsg::SharedObjects::create();
                // options->fileCache = vsg::getEnv("VSG_FILE_CACHE");
                // options->paths = vsg::getEnvPaths("VSG_FILE_PATH");

                rootNode = vsg::Group::create();
                contentGroup = vsg::Group::create();
                rootNode->addChild(contentGroup);

                auto ambientLight = vsg::AmbientLight::create();
                ambientLight->name = "ambient";
                ambientLight->color.set(0.5f, 0.5f, 0.5f);
                ambientLight->intensity = 0.02f;
                rootNode->addChild(ambientLight);

                auto directionalLight = vsg::DirectionalLight::create();
                directionalLight->name = "directional";
                directionalLight->color.set(1.0f, 1.0f, 1.0f);
                directionalLight->intensity = 0.7f;
                directionalLight->direction.set(-1.0f, -1.0f, -5.0f);
                rootNode->addChild(directionalLight);


                // todo: create version without qt
                viewer = vsgQt::Viewer::create();


                // todo: we have to search for a clean way to provide this uniform;
                //       we may have to inherit from the camera and update the uniform
                //       in the camera update method
                //GuiHelper::worldTransformUniform = WorldTransformUniformValue::create();
                //GuiHelper::worldTransformUniform->properties.dataVariance = vsg::DataVariance::DYNAMIC_DATA_TRANSFER_AFTER_RECORD;
                contentGroup->addChild(GuiHelper::stateGroupNodes);

                if(createWindow)
                {
                    LOG_ERROR("create new 3d window in initialize");
                    new3DWindow(0);
                }

                auto options = GuiHelper::getOrCreateOptions();
                std::string fontFilename = utils::pathJoin(GuiHelper::resourcePath, "resources/Fonts/IBMPlexSans-Medium.ttf");

                auto shaderSet = options->shaderSets["text"] = vsg::createTextShaderSet(options);

                // create a DepthStencilState, disable depth test and add this to the ShaderSet::defaultGraphicsPipelineStates container so it's used when setting up the TextGroup subgraph
                auto depthStencilState = vsg::DepthStencilState::create();
                depthStencilState->depthTestEnable = VK_FALSE;
                shaderSet->defaultGraphicsPipelineStates.push_back(depthStencilState);

                auto font = vsg::read_cast<vsg::Font>(fontFilename, options);
                if(!font)
                {
                    LOG_ERROR("GraphicsManager: unable to load font %s", fontFilename.c_str());
                }

                fpsLayout = vsg::StandardLayout::create();
                fpsLayout->horizontalAlignment = vsg::StandardLayout::LEFT_ALIGNMENT;
                fpsLayout->position = vsg::vec3(-0.0, -0.75, 0.5);
                fpsLayout->horizontal = vsg::vec3(0.025, 0.0, 0.0);
                fpsLayout->vertical = vsg::vec3(0.0, 0.025, 0.0);
                fpsLayout->color = vsg::vec4(0.2, 0.7, 0.2, 1.0);
                fpsLayout->outlineWidth = 0.1f;
                fpsLayout->billboard = true;

                fpsText = vsg::stringValue::create("fps: 0\navg: 0");

                fpsNode = vsg::Text::create();
                fpsNode->technique = vsg::GpuLayoutTechnique::create();
                fpsNode->text = fpsText;
                fpsNode->font = font;
                fpsNode->layout = fpsLayout;
                fpsNode->setup(32, options);

                // these are vsgQt::Viewer methods
                // these functione would start a time in vsgViewer to render images
                //viewer->setInterval(8);
                //viewer->continuousUpdate = true;

                //vsg::ref_ptr<vsg::ResourceHints> resourceHints;
                //viewer->compile(resourceHints);
                //viewer->start_point() = vsg::clock::now();
                // add close handler to respond to the close window button and pressing escape
                //viewer->addEventHandler(vsg::CloseHandler::create(viewer));

                // not yet useful
                //vsg::visit<SetGlobalPipelineStates>(rootNode);
                if(showCoords_.bValue)
                {
                    showCoords();
                }
                if(showGrid_.bValue)
                {
                    showGrid();
                }
                if(showFPS_.bValue)
                {
                    showFPS();
                }
            }
        }

        void* GraphicsManager::getWindowManager(int id) {(void)id; return 0;}

        void GraphicsManager::addDrawItems(drawStruct *draw) {(void)draw;}
        void GraphicsManager::removeDrawItems(DrawInterface *iface) {(void)iface;}
        void GraphicsManager::clearDrawItems(void) {}

        void GraphicsManager::addLight(LightData &ls) {(void)ls;}

        void GraphicsManager::addGraphicsUpdateInterface(GraphicsUpdateInterface *g)
        {
            graphicsUpdateObjects.push_back(g);
        }

        void GraphicsManager::removeGraphicsUpdateInterface(GraphicsUpdateInterface *g)
        {
            auto it = find(std::begin(graphicsUpdateObjects), std::end(graphicsUpdateObjects), g);
            if(it!=std::end(graphicsUpdateObjects))
            {
                graphicsUpdateObjects.erase(it);
            }

        }

        unsigned long GraphicsManager::addDrawObject(const NodeData &snode,
                                    bool activated)
        {
            {
                unsigned long id = getDrawID(snode.name);
                if(id) return id;
            }

            try {
                NodeData nodeSpec = snode;
                configmaps::ConfigMap spec;
                nodeSpec.toConfigMap(&spec, false, false);
                DrawObject *drawObject = new DrawObject();
                drawObject->createObject(spec);
                drawObject->name = snode.name;
                drawObject->id = nextDrawID;
                drawObjects[nextDrawID++] = drawObject;

                for(auto &it: windows)
                {
                    int windowMask = (1 << (it.first-1));
                    //LOG_ERROR("windowMask: %d, drawObjectMask: %d (set: %d)", windowMask, drawObject->mask, drawObject->maskSet);
                    if(drawObject->drawObject)
                    {
                        if(!drawObject->maskSet || (drawObject->maskSet && drawObject->mask & windowMask))
                        {
                            drawObject->parents.push_back(it.second->contentGroup);
                            it.second->contentGroup->addChild(drawObject->materialStateGroup);
                            //LOG_ERROR("DrawObject %s: add to window with id: %d.", drawObject->name.c_str(), it.first);
                            if(!drawObject->materialStateGroup)
                            {
                                LOG_ERROR("DrawObject %s: have no materialStateGroup.", drawObject->name.c_str());
                            }
                            if(drawObject->materialStateGroup->captureCommands.size() > 0 && !drawObject->appliedCaptureCommands)
                            {
                                for(auto &it2: drawObject->materialStateGroup->captureCommands)
                                {
                                    it.second->commandGraph->addChild(it2);
                                }
                                drawObject->appliedCaptureCommands = true;
                            }
                        }
                    } else
                    {
                        //LOG_ERROR("DrawObject %s: have no drawObejct.", drawObject->name.c_str());
                    }
                }

                // todo: handle management of node / window mask
                if(!activated)
                {
                    drawObject->setVisible(false);
                }

                //vsg::visit<SetGlobalPipelineStates>(rootNode);
                dirty_ = true;
                return nextDrawID-1;
            } catch(std::exception &e)
            {
                fprintf(stderr, "While adding DrawObject: %s: %s", snode.name.c_str(), e.what());
            }
            return 0;
        }

        unsigned long GraphicsManager::getDrawID(const std::string &name) const
        {
            (void)name;
            for(auto &it: drawObjects)
            {
                if(it.second->name == name)
                {
                    return it.second->id;
                }
            }
            return 0;
        }

        void GraphicsManager::removeDrawObject(unsigned long id) {(void)id;}
        void GraphicsManager::setDrawObjectPos(unsigned long id,
                                      const mars::utils::Vector &pos)
        {
            auto drawObjectIter = drawObjects.find(id);
            if(drawObjectIter != drawObjects.end())
            {
                drawObjectIter->second->setPosition(pos);
            }
        }

        void GraphicsManager::setDrawObjectRot(unsigned long id,
                                      const mars::utils::Quaternion &q)
        {
            auto drawObjectIter = drawObjects.find(id);
            if(drawObjectIter != drawObjects.end())
            {
                drawObjectIter->second->setQuaternion(q);
            }
        }

        void GraphicsManager::setDrawObjectScale(unsigned long id,
                                                 const mars::utils::Vector &scale) {(void)id; (void)scale;}
        void GraphicsManager::setDrawObjectScaledSize(unsigned long id,
                                                      const mars::utils::Vector &ext) {(void)id; (void)ext;}
        void GraphicsManager::setDrawObjectMaterial(unsigned long id,
                                                    const MaterialData &material) {(void)id; (void)material;}
        void GraphicsManager::addMaterial(const MaterialData &material) {(void)material;}
        void GraphicsManager::setDrawObjectNodeMask(unsigned long id, unsigned int bits) {(void)id; (void)bits;}

        void GraphicsManager::closeAxis() {}

        void GraphicsManager::drawAxis(const mars::utils::Vector &first,
                                       const mars::utils::Vector &second,
                                       const mars::utils::Vector &third,
                                       const mars::utils::Vector &axis1,
                                       const mars::utils::Vector &axis2) {(void)first;(void)second;(void)third;(void)axis1;(void)axis2;}

        void GraphicsManager::getCameraInfo(cameraStruct *cs) const {(void)cs;}

        void* GraphicsManager::getStateSet() const {return 0;}

        void* GraphicsManager::getScene() const {return 0;}
        void* GraphicsManager::getScene2() const {return 0;}

        void GraphicsManager::hideCoords()
        {
            if(coords)
            {
                // todo: this could be a util method removeNode()
                for(auto &it: windows)
                {
                    auto &group = it.second->contentGroup;
                    auto it2 = std::find(group->children.begin(), group->children.end(), coords);
                    if(it2 != group->children.end())
                    {
                        group->children.erase(it2);
                        dirty_ = true;
                    }
                }
            }
        }

        void GraphicsManager::hideCoords(const mars::utils::Vector &pos) {(void)pos;}

        void GraphicsManager::showClouds() {}
        void GraphicsManager::hideClouds() {}

        void GraphicsManager::preview(int action, bool resize,
                             const std::vector<NodeData> &allNodes,
                             unsigned int num,
                                      const MaterialData *mat) {(void)action;(void)resize;(void)allNodes;(void)num;(void)mat;}

        void GraphicsManager::removeLight(unsigned int index) {(void)index;}

        void GraphicsManager::removePreviewNode(unsigned long id) {(void)id;}

        void GraphicsManager::reset() {}

        void GraphicsManager::setCamera(int type) {(void)type;}


        void GraphicsManager::showFPS()
        {
            if(windows.find(1) != windows.end() && windows[1]->overlayGroup)
            {
                windows[1]->overlayGroup->addChild(fpsNode);
                dirty_ = true;
            }
        }

        void GraphicsManager::hideFPS()
        {
            if(windows.find(1) != windows.end() && windows[1]->overlayGroup)
            {
                vsg::ref_ptr<vsg::Group> &node = windows[1]->overlayGroup;
                node->children.erase(std::find(node->children.begin(), node->children.end(), fpsNode));
                dirty_ = true;
            }
        }

        void GraphicsManager::showCoords()
        {
            if(!coords)
            {
                std::string coordsFile = pathJoin(resourcesPath.sValue, "resources/Objects/coords.3ds");
                coords = GuiHelper::readNodeFromFile(coordsFile);
                if(!coords)
                {
                    LOG_ERROR("Failed to load: %s", coordsFile.c_str());
                    return;
                }
                // Extract the materialValue which can be used to change the property
                auto materialValue = extractMaterialValue(coords);
                auto material = (vsg::PbrMaterial*)(materialValue->dataPointer(0));
                material->diffuseFactor = vsg::vec4{1.0,1.0,1.0,1.0};
                //material->ambientFactor = vsg::vec4{0.5,0.5,0.5,1.0};
            }
            if(coords)
            {
                for(auto &it: windows)
                {
                    int windowMask = (1 << (it.first-1));
                    if(coordsWindowMask & windowMask)
                    {
                        it.second->contentGroup->children.insert(it.second->contentGroup->children.begin(), coords);
                        dirty_ = true;
                    }
                }
            }
        }

        void GraphicsManager::showCoords(const mars::utils::Vector &pos,
                                const mars::utils::Quaternion &rot,
                                         const mars::utils::Vector &size) {(void)pos; (void)rot; (void)size;}

        bool GraphicsManager::coordsVisible(void) const {return 0;}
        bool GraphicsManager::gridVisible(void) const {return 0;}
        bool GraphicsManager::cloudsVisible(void) const {return 0;}

        void GraphicsManager::update()
        {
        }

        void GraphicsManager::saveScene(const std::string &filename) const
        {
            (void)filename;
        }

        const interfaces::GraphicData GraphicsManager::getGraphicOptions(void) const {return graphicOptions;}
        void GraphicsManager::setGraphicOptions(const GraphicData &options,
                                                bool ignoreClearColor) {(void)options; (void)ignoreClearColor;}
        void GraphicsManager::showGrid(void)
        {
            if(!grid)
            {
                grid = Grid::create();
            }
            if(grid)
            {
                for(auto &it: windows)
                {
                    int windowMask = (1 << (it.first-1));
                    if(gridWindowMask & windowMask)
                    {
                        it.second->contentGroup->children.insert(it.second->contentGroup->children.end(), grid);
                        dirty_ = true;
                    }
                }
            }
        }

        void GraphicsManager::hideGrid(void)
        {
            if(grid)
            {
                for(auto &it: windows)
                {
                    auto &group = it.second->contentGroup;
                    auto it2 = std::find(group->children.begin(), group->children.end(), grid);
                    if(it2 != group->children.end())
                    {
                        group->children.erase(it2);
                        dirty_ = true;
                    }
                }
            }
        }

        void GraphicsManager::updateLight(unsigned int index, bool recompileShader) {(void)index;(void)recompileShader;}
        void GraphicsManager::getLights(std::vector<LightData*> *lightList) {(void)lightList;}
        void GraphicsManager::getLights(std::vector<LightData> *lightList) const {(void)lightList;}
        int GraphicsManager::getLightCount(void) const {return 0;}
        void GraphicsManager::exportScene(const std::string &filename) const
        {
            fprintf(stderr, "export scene called: %s\n", filename.c_str());
            std::string name = filename;
            if(utils::getFilenameSuffix(name) == ".osg")
            {
                utils::removeFilenameSuffix(&name);
                for(auto &it: windows)
                {
                    char n[25];
                    snprintf(n, 24, "%s_%d.vsgt", name.c_str(), it.first);
                    fprintf(stderr, "export to %s", n);
                    vsg::write(it.second->contentGroup, n);
                }
            }
        }

        void GraphicsManager::setTexture(unsigned long id, const std::string &filename) {(void)id; (void)filename;}

        unsigned long GraphicsManager::new3DWindow(void *myQTWidget, bool rtt,
                                                   int width, int height, const std::string &name)
        {
            GraphicsWidget *shared = nullptr;
            if(windows.size() > 0)
            {
                shared = windows.begin()->second;
                LOG_ERROR("GraphicsManager::new3DWindow have shared window!");
            }
            else
            {
                LOG_ERROR("GraphicsManager::new3DWindow create first window!");
            }
            GraphicsWidget *window = new GraphicsWidget(nullptr, rootNode, nextWindowID, rtt, this);
            window->name = name;

            window->initialize(nullptr, shared, width, height);
            windows[nextWindowID] = window;

            if(!shared)
            {
                // todo: solve this for each camera
                //GuiHelper::worldTransformUniform->value().projInverse = window->perspective->inverse();
                //GuiHelper::worldTransformUniform->value().viewInverse = window->lookAt->inverse();
            }
            // setup content
            int windowMask = (1 << (nextWindowID-1));
            for(auto &it: drawObjects)
            {
                auto &drawObject = it.second;
                if(drawObject->drawObject)
                {
                    if(!drawObject->maskSet || (drawObject->maskSet && drawObject->mask & windowMask))
                    {
                        drawObject->parents.push_back(window->contentGroup);
                        if(drawObject->visible)
                        {
                            window->contentGroup->addChild(drawObject->materialStateGroup);
                        }
                        if(drawObject->materialStateGroup->captureCommands.size() > 0 && !drawObject->appliedCaptureCommands)
                        {
                            for(auto &it2: drawObject->materialStateGroup->captureCommands)
                            {
                                window->commandGraph->addChild(it2);
                            }
                            drawObject->appliedCaptureCommands = true;
                        }
                    }
                }
            }
            for(auto &it: externNodes)
            {
                if(it.windowMask & windowMask)
                {
                    window->contentGroup->addChild(it.node);
                }
            }
            if(coords && showCoords_.bValue && coordsWindowMask & windowMask)
            {
                window->contentGroup->children.insert(window->contentGroup->children.begin(), coords);
            }
            if(grid && showGrid_.bValue && gridWindowMask & windowMask)
            {
                window->contentGroup->children.insert(window->contentGroup->children.end(), grid);
            }

            dirty_ = true;
            return nextWindowID++;
        }

        void GraphicsManager::setGrabFrames(bool value) {(void)value;}

        GraphicsWindowInterface* GraphicsManager::get3DWindow(unsigned long id) const
        {
            auto iter = windows.find(id);
            if(iter != windows.end())
            {
                return iter->second;
            }
            return 0;
        }

        GraphicsWindowInterface* GraphicsManager::get3DWindow(const std::string &name) const
        {
            for(auto &iter: windows)
            {
                if(iter.second->name == name)
                {
                    return iter.second;
                }
            }
            return 0;
        }

        void GraphicsManager::remove3DWindow(unsigned long id) {(void)id;}
        void GraphicsManager::getList3DWindowIDs(std::vector<unsigned long> *ids) const {(void) ids;}
        void GraphicsManager::removeLayerFromDrawObjects(unsigned long window_id) {(void)window_id;}

        // HUD Interface:
        unsigned long GraphicsManager::addHUDElement(hudElementStruct *new_hud_element) {(void)new_hud_element;return 0;}
        void GraphicsManager::removeHUDElement(unsigned long id) {(void)id;}
        void GraphicsManager::switchHUDElementVis(unsigned long id) {(void)id;}
        void GraphicsManager::setHUDElementPos(unsigned long id, double x, double y) {(void)id;(void)x;(void)y;}
        void GraphicsManager::setHUDElementTextureData(unsigned long id, void* data) {(void)id; (void)data;}
        void GraphicsManager::setHUDElementTextureRTT(unsigned long id,
                                             unsigned long window_id,
                                                      bool depthComponent) {(void)id; (void)window_id;(void)depthComponent;}
        void GraphicsManager::setHUDElementTexture(unsigned long id,
                                                   std::string texturename) {(void)id; (void)texturename;}
        void GraphicsManager::setHUDElementLabel(unsigned long id, std::string text,
                                                 double text_color[4]) {(void)id; (void)text; (void)text_color;}
        void GraphicsManager::setHUDElementLines(unsigned long id, std::vector<double> *v,
                                                 double color[4]) {(void)id; (void)v;(void)color;}

        void* GraphicsManager::getQTWidget(unsigned long id) const
        {
            auto iter = windows.find(id);
            if(iter != windows.end())
            {
                return iter->second->container;
            }
            return 0;
        }

        void GraphicsManager::showQTWidget(unsigned long id)
        {
            auto iter = windows.find(id);
            if(iter != windows.end())
            {
                iter->second->container->show();
            }
        }

        void GraphicsManager::addGuiEventHandler(GuiEventInterface *_guiEventHandler) {(void)_guiEventHandler;}
        void GraphicsManager::removeGuiEventHandler(GuiEventInterface *_guiEventHandler) {(void)_guiEventHandler;}
        void GraphicsManager::exportDrawObject(unsigned long id,
                                               const std::string &name) const {(void)id; (void)name;}
        void GraphicsManager::setBlending(unsigned long id, bool mode) {(void)id; (void)mode;}
        void GraphicsManager::setBumpMap(unsigned long id, const std::string &bumpMap) {(void)id;(void)bumpMap;}
        void GraphicsManager::setGraphicsWindowGeometry(unsigned long id, int top,
                                                        int left, int width, int height) {(void)id;(void)top;(void)left;(void)width;(void)height;}
        void GraphicsManager::getGraphicsWindowGeometry(unsigned long id,
                                               int *top, int *left,
                                               int *width, int *height) const {(void)id;(void)top;(void)left;(void)width;(void)height;}
        void GraphicsManager::setActiveWindow(unsigned long win_id) {(void)win_id;}
        void GraphicsManager::setDrawObjectSelected(unsigned long id, bool val) {(void)id; (void)val;}
        void GraphicsManager::setDrawObjectShow(unsigned long id, bool val)
        {
            auto it = drawObjects.find(id);
            if(it != drawObjects.end())
            {
                it->second->setVisible(val);
            }
            dirty_ = true;
        }

        void GraphicsManager::setDrawObjectRBN(unsigned long id, int val) {(void)id; (void)val;}
        void GraphicsManager::addEventClient(GraphicsEventClient* theClient) {(void)theClient;}
        void GraphicsManager::removeEventClient(GraphicsEventClient* theClient) {(void)theClient;}
        void GraphicsManager::setSelectable(unsigned long id, bool val) {(void)id; (void)val;}
        void GraphicsManager::showNormals(bool val) {(void)val;}
        void* GraphicsManager::getView(unsigned long id) {(void)id;return 0;}
        void GraphicsManager::collideSphere(unsigned long id, mars::utils::Vector pos,
                                            sReal radius) {(void)id;(void)pos;(void)radius;}

        const utils::Vector& GraphicsManager::getDrawObjectPosition(unsigned long id)
        {
            static Vector dummy;
            auto it = drawObjects.find(id);
            if(it != drawObjects.end())
            {
                return it->second->getPosition();
            }
            return dummy;
        }

        const utils::Quaternion& GraphicsManager::getDrawObjectQuaternion(unsigned long id)
        {
            (void)id;
            static  Quaternion dummy;
            auto it = drawObjects.find(id);
            if(it != drawObjects.end())
            {
                return it->second->getQuaternion();
            }
            return dummy;
        }

        void GraphicsManager::draw()
        {
            static unsigned long framecount = 0;
            static unsigned long framecount2 = 0;
            static auto time1 = vsg::clock::now();
            static auto time2 = vsg::clock::now();
            ++framecount;
            ++framecount2;

            auto now = vsg::clock::now();
            double td = std::chrono::duration<double, std::chrono::milliseconds::period>(now - time1).count();
            double td2 = std::chrono::duration<double, std::chrono::milliseconds::period>(now - time2).count();
            if(td > 100)
            {
                if(showFPS_.bValue)
                {
                    auto mainWindow = windows.find(1);
                    if(mainWindow != windows.end())
                    {
                        const int width = mainWindow->second->window->width();
                        const int height = mainWindow->second->window->height();
                        const double ratio = static_cast<double>(width) / static_cast<double>(height);
                        const double top = 0.5;
                        const double left = -0.52*ratio;
                        fpsLayout->position = vsg::vec3(0.0, left, top);
                    }

                    int fps = framecount*1000./td;
                    int fps2 = framecount2*1000./td2;
                    fpsText->value() = vsg::make_string("fps: ", fps, "\navg: ", fps2);
                    auto options = GuiHelper::getOrCreateOptions();
                    fpsNode->setup(0, options);
                }
                time1 = now;
                framecount = 0;
            }


            // todo: remove draw handling via nsview
            for(auto& graphicsUpdateObject: graphicsUpdateObjects)
            {
                graphicsUpdateObject->preGraphicsUpdate();
            }

            // todo: solve this for each camera
            auto iter = windows.begin();
            if(iter != windows.end())
            {
                auto window = iter->second;
                //GuiHelper::worldTransformUniform->value().projInverse = window->perspective->inverse();
                //GuiHelper::worldTransformUniform->value().viewInverse = window->lookAt->inverse();
                //GuiHelper::worldTransformUniform->dirty();
            }
            // fprintf(stderr, ". ");
            // // pass any events into EventHandlers assigned to the Viewer
            if(dirty_)
            {
                viewer->compile();
                dirty_ = false;
            }
            // could also try
            viewer->render();
            // viewer->handleEvents();
            // viewer->update();
            // viewer->recordAndSubmit();
            // viewer->present();
            for(auto& graphicsUpdateObject: graphicsUpdateObjects)
            {
                graphicsUpdateObject->postGraphicsUpdate();
            }
        }

        void GraphicsManager::lock() {}
        void GraphicsManager::unlock() {}


        LoadMeshInterface* GraphicsManager::getLoadMeshInterface(void)
        {
            //return guiHelper;
            return nullptr;
        }

        LoadHeightmapInterface* GraphicsManager::getLoadHeightmapInterface(void) {return 0;}

        void GraphicsManager::makeChild(unsigned long parentId, unsigned long childId) {(void)parentId;(void)childId;}
        void GraphicsManager::attacheCamToNode(unsigned long winID, unsigned long drawID) {(void)winID;(void)drawID;}

        void GraphicsManager::setExperimentalLineLaser(utils::Vector pos, utils::Vector normal, utils::Vector color, utils::Vector laserAngle, float openingAngle) {(void)pos;(void)normal;(void)color;(void)laserAngle;(void)openingAngle;}
        void GraphicsManager::deactivate3DWindow(unsigned long id) {(void)id;}
        void GraphicsManager::activate3DWindow(unsigned long id) {(void)id;}

        // be carful with this method, only add a valid pointer osg::Node*
        void GraphicsManager::addOSGNode(void* node)
        {
            addGraphicsNode(node);
        }

        void GraphicsManager::removeOSGNode(void* node)
        {
            removeGraphicsNode(node);
        }

        void GraphicsManager::addGraphicsNode(void* node, int windowMask)
        {
            ExternNode n{vsg::ref_ptr<vsg::Node>(static_cast<vsg::Node*>(node)), windowMask};
            for(auto &it: windows)
            {
                int mask = (1 << (it.first-1));
                if(mask & windowMask)
                {
                    it.second->contentGroup->addChild(n.node);
                    dirty_ = true;
                }
            }
            externNodes.push_back(n);
        }

        void GraphicsManager::removeGraphicsNode(void* node)
        {
            vsg::ref_ptr<vsg::Node> n(static_cast<vsg::Node*>(node));
            for(auto &it: windows)
            {
                auto &contentGroup = it.second->contentGroup;
                auto it2 = std::find(contentGroup->children.begin(), contentGroup->children.end(), n);
                if(it2 != contentGroup->children.end())
                {
                    contentGroup->children.erase(it2);
                    dirty_ = true;
                }
            }
            auto it = externNodes.begin();
            while(it != externNodes.end())
            {
                if(it->node == n)
                {
                    externNodes.erase(it);
                } else
                {
                    ++it;
                }
            }
        }

        unsigned long GraphicsManager::addHUDOSGNode(void* node) {(void)node;return 0;}
        bool GraphicsManager::isInitialized() const {return 0;}
        std::vector<interfaces::MaterialData> GraphicsManager::getMaterialList() const
        {
            return std::vector<interfaces::MaterialData>{};
        }

        void GraphicsManager::editMaterial(std::string materialName, std::string key,
                                           std::string value) {(void)materialName;(void)key;(void)value;}
        void GraphicsManager::setCameraDefaultView(int view) {(void)view;}
        void GraphicsManager::setDrawObjectBrightness(unsigned long id, double v) {(void)id;(void)v;}
        void GraphicsManager::editLight(unsigned long id, const std::string &key,
                                        const std::string &value) {(void)id;(void)key;(void)value;}
        void GraphicsManager::edit(const std::string &key, const std::string &value) {(void)key;(void)value;}
        void GraphicsManager::edit(unsigned long widgetID, const std::string &key,
                                   const std::string &value) {(void)widgetID;(void)key;(void)value;}
        void GraphicsManager::brushTest(mars::utils::Vector start, mars::utils::Vector end) {(void)start;(void)end;}
        void GraphicsManager::brushTestThreaded(std::vector<utils::Vector> start_, std::vector<utils::Vector> end) {(void)start_;(void)end;}

        void GraphicsManager::cfgUpdateProperty(cfg_manager::cfgPropertyStruct _property)
        {
            fprintf(stderr, "cfgUpdateProperty called\n");
            for(auto prop: cfgProperties)
            {
                if(_property.paramId == prop->paramId)
                {
                    if(_property.propertyType == cfg_manager::intProperty) prop->iValue = _property.iValue;
                    else if(_property.propertyType == cfg_manager::boolProperty) prop->bValue = _property.bValue;
                    else if(_property.propertyType == cfg_manager::doubleProperty) prop->dValue = _property.dValue;
                    else if(_property.propertyType == cfg_manager::stringProperty) prop->sValue = _property.sValue;
                }
            }
            if(_property.paramId == showCoords_.paramId)
            {
                if(showCoords_.bValue != _property.bValue)
                {
                    showCoords_.bValue = _property.bValue;
                    fprintf(stderr, "update show coords\n");
                    if(showCoords_.bValue)
                    {
                        showCoords();
                    }
                    else
                    {
                        hideCoords();
                    }
                }
            }
            else if(_property.paramId == showGrid_.paramId)
            {
                if(showGrid_.bValue != _property.bValue)
                {
                    showGrid_.bValue = _property.bValue;
                    if(showGrid_.bValue)
                    {
                        showGrid();
                    }
                    else
                    {
                        hideGrid();
                    }
                }
            }
            else if(_property.paramId == showFPS_.paramId)
            {
                if(showFPS_.bValue != _property.bValue)
                {
                    showFPS_.bValue = _property.bValue;
                    if(showFPS_.bValue)
                    {
                        showFPS();
                    }
                    else
                    {
                        hideFPS();
                    }
                }
            }
        }

        void GraphicsManager::produceData(const data_broker::DataInfo &info,
                                          data_broker::DataPackage *dbPackage,
                                          int callbackParam)
        {
            (void)info;
            (void)dbPackage;
            (void)callbackParam;
            //dbPackageMapping.writePackage(dbPackage);
        }

        void GraphicsManager::setupCFG(void)
        {
            auto configPath = cfg->getOrCreateProperty("Config", "config_path",
                                                       std::string{"."});

            auto loadFile = configPath.sValue;
            loadFile.append("/mars_Graphics.yaml");
            cfg->loadConfig(loadFile.c_str());

            resourcesPath.propertyType = cfg_manager::stringProperty;
            resourcesPath.propertyIndex = 0;
            cfgProperties.push_back(&resourcesPath);

            const auto& s = cfg->getOrCreateProperty("Graphics", "resources_path",
                                                     "",
                                                     dynamic_cast<cfg_manager::CFGClient*>(this)).sValue;
            if(s != "")
            {
                resourcesPath.sValue = s;
            } else
            {
                resourcesPath.sValue = std::string(MARS_VSG_GRAPHICS_DEFAULT_RESOURCES_PATH);
            }
            GuiHelper::resourcePath = resourcesPath.sValue;
            showCoords_ = cfg->getOrCreateProperty("Graphics", "showCoords",
                                                   true, this);
            showGrid_ = cfg->getOrCreateProperty("Graphics", "showGrid",
                                                   true, this);
            showFPS_ = cfg->getOrCreateProperty("Graphics", "showFPS",
                                                   false, this);
            coordsWindowMask = cfg->getOrCreateProperty("Graphics", "coordsWindowMask",
                                                        std::numeric_limits<int>::max(), this).iValue;
            gridWindowMask = cfg->getOrCreateProperty("Graphics", "gridWindowMask",
                                                      std::numeric_limits<int>::max(), this).iValue;
        }

        void GraphicsManager::getTextureSize(std::string materialName,
                                             std::string textureName,
                                             int *w, int *h)
        {
            auto it = GuiHelper::stateGroups.find(materialName);
            if(it != GuiHelper::stateGroups.end())
            {
                auto it2 = it->second->images.find(textureName);
                if(it2 != it->second->images.end())
                {
                    *w = it2->second->extent.width;
                    *h = it2->second->extent.height;
                } else
                {
                    LOG_ERROR("GraphicsManager::getTextureSize texture: %s not found in list of:", textureName.c_str());
                    for(auto &nameIt: it->second->images)
                    {
                        LOG_ERROR("GraphicsManager::getTextureSize  - %s", nameIt.first.c_str());
                    }
                }
            } else
            {
                LOG_ERROR("GraphicsManager::getTextureSize mateial: %s not found in list of:", materialName.c_str());
                for(auto &nameIt: GuiHelper::stateGroups)
                {
                    LOG_ERROR("GraphicsManager::getTextureSize  - %s", nameIt.first.c_str());
                }
            }
        }

        void GraphicsManager::captureTextureData(std::string materialName,
                                                 std::string textureName,
                                                 char *buffer,
                                                 int w, int h)
        {
            auto it = GuiHelper::stateGroups.find(materialName);
            if(it != GuiHelper::stateGroups.end())
            {
                auto it2 = it->second->captureImages.find(textureName);
                if(it2 != it->second->captureImages.end())
                {
                    if(w != it2->second->extent.width)
                    {
                        LOG_ERROR("GraphicsManager::captureTextureData width %d doesn't match image width %d!", w, it2->second->extent.width);
                        return;
                    }
                    if(h != it2->second->extent.height)
                    {
                        LOG_ERROR("GraphicsManager::captureTextureData height %d doesn't match image height %d!", h, it2->second->extent.height);
                        return;
                    }

                    auto imageData = vsg_graphics::getImageData(viewer, it2->second);
                    char* srcBuffer = (char*)imageData->dataPointer();
                    memcpy(buffer, srcBuffer, h*w*4);
                } else
                {
                    LOG_ERROR("GraphicsManager::captureTextureData texture: %s not found in list of:", textureName.c_str());
                    for(auto &nameIt: it->second->captureImages)
                    {
                        LOG_ERROR("GraphicsManager::captureTextureData  - %s", nameIt.first.c_str());
                    }
                }
            } else
            {
                LOG_ERROR("GraphicsManager::captureTextureData mateial: %s not found in list of:", materialName.c_str());
                for(auto &nameIt: GuiHelper::stateGroups)
                {
                    LOG_ERROR("GraphicsManager::captureTextureData  - %s", nameIt.first.c_str());
                }
            }
        }

        void GraphicsManager::getTextureData(std::string materialName,
                                             std::string textureName,
                                             char *buffer,
                                             int w, int h)
        {
            auto it = GuiHelper::stateGroups.find(materialName);
            if(it != GuiHelper::stateGroups.end())
            {
                auto it2 = it->second->images.find(textureName);
                if(it2 != it->second->images.end())
                {
                    if(w != it2->second->extent.width)
                    {
                        LOG_ERROR("GraphicsManager::getTextureData width %d doesn't match image width %d!", w, it2->second->extent.width);
                        return;
                    }
                    if(h != it2->second->extent.height)
                    {
                        LOG_ERROR("GraphicsManager::getTextureData height %d doesn't match image height %d!", h, it2->second->extent.height);
                        return;
                    }

                    char* srcBuffer = (char*)it2->second->data->dataPointer();
                    memcpy(buffer, srcBuffer, h*w*4);
                } else
                {
                    LOG_ERROR("GraphicsManager::getTextureData texture: %s not found in list of:", textureName.c_str());
                    for(auto &nameIt: it->second->images)
                    {
                        LOG_ERROR("GraphicsManager::getTextureData  - %s", nameIt.first.c_str());
                    }
                }
            } else
            {
                LOG_ERROR("GraphicsManager::getTextureData mateial: %s not found in list of:", materialName.c_str());
                for(auto &nameIt: GuiHelper::stateGroups)
                {
                    LOG_ERROR("GraphicsManager::getTextureData  - %s", nameIt.first.c_str());
                }
            }
        }

        void GraphicsManager::setTextureData(std::string materialName,
                                             std::string textureName,
                                             char *buffer,
                                             int w, int h)
        {
            auto it = GuiHelper::stateGroups.find(materialName);
            if(it != GuiHelper::stateGroups.end())
            {
                auto it2 = it->second->images.find(textureName);
                if(it2 != it->second->images.end())
                {
                    if(w != it2->second->extent.width)
                    {
                        LOG_ERROR("GraphicsManager::setTextureData width %d doesn't match image width %d!", w, it2->second->extent.width);
                        return;
                    }
                    if(h != it2->second->extent.height)
                    {
                        LOG_ERROR("GraphicsManager::setTextureData height %d doesn't match image height %d!", h, it2->second->extent.height);
                        return;
                    }
                    char* dstBuffer = (char*)it2->second->data->dataPointer();
                    memcpy(dstBuffer, buffer, h*w*4);
                    it2->second->data->dirty();
                } else
                {
                    LOG_ERROR("GraphicsManager::setTextureData texture: %s not found in list of:", textureName.c_str());
                    for(auto &nameIt: it->second->images)
                    {
                        LOG_ERROR("GraphicsManager::setTextureData  - %s", nameIt.first.c_str());
                    }
                }

            } else
            {
                LOG_ERROR("GraphicsManager::setTextureData mateial: %s not found in list of:", materialName.c_str());
                for(auto &nameIt: GuiHelper::stateGroups)
                {
                    LOG_ERROR("GraphicsManager::setTextureData  - %s", nameIt.first.c_str());
                }
            }
        }

        bool GraphicsManager::createManipulator(std::string name,
                                                interfaces::ManipulatorClient *mc,
                                                int windowMask)
        {
            if(manipulators.find(name) != manipulators.end())
            {
                LOG_ERROR("Manipulator with name %s already created!", name.c_str());
                return false;
            }
            auto node = Manipulator::create(resourcesPath.sValue);
            node->eventClient = mc;
            node->mask = windowMask;
            manipulators[name] = node;
            for(auto &it: windows)
            {
                int mask = (1 << (it.first-1));
                if(mask & windowMask)
                {
                    if(it.second->eventHandler)
                    {
                        it.second->contentGroup->addChild(node);
                        //vsg::ref_ptr<InteractionHandler> g(node->cast<InteractionHandler>());
                        it.second->eventHandler->interactionHandlers.push_back(node);
                    }
                }
            }
            return true;
        }

        void GraphicsManager::setManipulatorPose(std::string name,
                                                 const utils::Vector &v,
                                                 const utils::Quaternion &q)
        {
            auto it = manipulators.find(name);
            if(it == manipulators.end())
            {
                LOG_ERROR("setPose: Manipulator %s not found!", name.c_str());
                return;
            }
            it->second->setPose(v, q);
        }

        void GraphicsManager::setManipulatorScale(std::string name,
                                                  const utils::Vector &s)
        {
            auto it = manipulators.find(name);
            if(it == manipulators.end())
            {
                LOG_ERROR("setScale: Manipulator %s not found!", name.c_str());
                return;
            }
            it->second->setScale(s);
        }

        void GraphicsManager::dirty()
        {
            LOG_ERROR("dirty");
            dirty_ = true;
        }

        bool GraphicsManager::getIntersection(unsigned long windowID, const utils::Vector &start,
                                              const utils::Vector &end, utils::Vector &pos)
        {
            auto wit = windows.find(windowID);
            if(wit == windows.end())
            {
                LOG_ERROR("GraphicsManager::getIntersection window with id %lu not found!", windowID);
                return false;
            }
            vsg::dvec3 s(start.x(), start.y(), start.z());
            vsg::dvec3 e(end.x(), end.y(), end.z());
            auto intersector = vsg::LineSegmentIntersector::create(s, e);
            wit->second->contentGroup->accept(*intersector);
            if (intersector->intersections.empty()) return false;
            // sort the intersections front to back
            auto intersection = intersector->intersections[0];
            for(auto &it : intersector->intersections)
            {
                if(it->ratio < intersection->ratio)
                {
                    intersection = it;
                }
            }
            pos.x() = intersection->worldIntersection.x;
            pos.y() = intersection->worldIntersection.y;
            pos.z() = intersection->worldIntersection.z;
            return true;
            //LOG_ERROR("found intersection at %g %g %g", intersection->worldIntersection.x, intersection->worldIntersection.y, intersection->worldIntersection.z);

        }

    } // end of namespace vsg_graphics
} // end of namespace mars


DESTROY_LIB(mars::vsg_graphics::GraphicsManager);
CREATE_LIB(mars::vsg_graphics::GraphicsManager);
