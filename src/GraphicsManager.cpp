#include <QWidget>

#include "GraphicsManager.hpp"
#include "DrawObject.hpp"
#include "config.h"
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
            : GraphicsManagerInterface(theManager), window(nullptr), container(nullptr), guiHelper{new GuiHelper{this}}
        {
            (void)QTWidget;
            dirty = true;
        }

        GraphicsManager::~GraphicsManager()
        {
            if(cfg)
            {
                libManager->releaseLibrary("cfg_manager");
            }
            for(auto it: drawObjects)
            {
                delete it.second;
            }
            if(window)
            {
                delete window;
            }
            delete guiHelper;
        }

        void GraphicsManager::initializeOSG(void *data, bool createWindow)
        {
            (void)data;
            (void)createWindow;
            if(!viewer) {
                cfg = libManager->getLibraryAs<cfg_manager::CFGManagerInterface>("cfg_manager");
                if(!cfg)
                {
                    fprintf(stderr, "******* mars_graphics: couldn't find cfg_manager\n");
                    return;
                }
                setupCFG();

                nextDrawID = 1;

                auto traits = vsg::WindowTraits::create();
                // auto options = vsg::Options::create();
                // options->sharedObjects = vsg::SharedObjects::create();
                // options->fileCache = vsg::getEnv("VSG_FILE_CACHE");
                // options->paths = vsg::getEnvPaths("VSG_FILE_PATH");
                traits->windowTitle = "mars view";

                rootNode = vsg::Group::create();

                if(showCoords_.bValue)
                {
                    showCoords();
                }

                auto ambientLight = vsg::AmbientLight::create();
                ambientLight->name = "ambient";
                ambientLight->color.set(1.0f, 1.0f, 1.0f);
                ambientLight->intensity = 0.02f;
                rootNode->addChild(ambientLight);

                auto directionalLight = vsg::DirectionalLight::create();
                directionalLight->name = "directional";
                directionalLight->color.set(1.0f, 1.0f, 1.0f);
                directionalLight->intensity = 0.7f;
                directionalLight->direction.set(-1.0f, -1.0f, -1.0f);
                rootNode->addChild(directionalLight);


                viewer = vsgQt::Viewer::create();
                window = new vsgQt::Window(viewer, traits, (QWindow*)nullptr);
                window->setTitle("3D Window");
                window->initializeWindow();

                //auto window = vsg::Window::create(traits);
                if (!window)
                {
                    std::cout << "Could not create window." << std::endl;
                    return;
                }

                // if this is the first window to be created, use its device for future window creation.
                if (!traits->device) traits->device = window->windowAdapter->getOrCreateDevice();

                uint32_t width = window->traits->width;
                uint32_t height = window->traits->height;
                fprintf(stderr, "-------- with: %u\theight: %u\n", width, height);

                //viewer->addWindow(window);
                double radius = 1.0;
                lookAt = vsg::LookAt::create(vsg::dvec3(radius * 2.0, 0.0, 0.0), vsg::dvec3(0.0, 0.0, 0.0), vsg::dvec3(0.0, 0.0, 1.0));
                perspective = vsg::Perspective::create(30.0, static_cast<double>(width) / static_cast<double>(height), 0.001 * radius, radius * 100.5);
                auto camera = vsg::Camera::create(perspective, lookAt, vsg::ViewportState::create(VkExtent2D{width, height}));
            
                auto trackball = vsg::Trackball::create(camera);
                trackball->addWindow(*window);
                viewer->addEventHandler(trackball);

                auto clearColor = vsg::vec4(0.2f, 0.2f, 0.7f, 1.0f);

                // todo: we have to search for a clean way to provide this uniform;
                //       we may have to inherit from the camera and update the uniform
                //       in the camera update method
                GuiHelper::worldTransformUniform = WorldTransformUniformValue::create();
                GuiHelper::worldTransformUniform->properties.dataVariance = vsg::DataVariance::DYNAMIC_DATA;
                GuiHelper::worldTransformUniform->value().projInverse = perspective->inverse();
                GuiHelper::worldTransformUniform->value().viewInverse = lookAt->inverse();
                rootNode->addChild(GuiHelper::stateGroupNodes);

                auto renderGraph = vsg::createRenderGraphForView(*window, camera, rootNode, VK_SUBPASS_CONTENTS_INLINE, false);
                renderGraph->setClearValues(vsg::sRGB_to_linear(clearColor));

                auto commandGraph = vsg::CommandGraph::create(*window, renderGraph);
                //auto commandGraph = vsg::createCommandGraphForView(*window, camera, rootNode);

                //viewer->addRecordAndSubmitTaskAndPresentation({commandGraph});
                viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

                // these are vsgQt::Viewer methods
                // these functione would start a time in vsgViewer to render images
                //viewer->setInterval(8);
                //viewer->continuousUpdate = true;

                vsg::ref_ptr<vsg::ResourceHints> resourceHints;
                //viewer->compile(resourceHints);
                //viewer->start_point() = vsg::clock::now();
                container = QWidget::createWindowContainer(window, nullptr);
                container->setGeometry(window->traits->x, window->traits->y, window->traits->width, window->traits->height);
                // add close handler to respond to the close window button and pressing escape
                //viewer->addEventHandler(vsg::CloseHandler::create(viewer));
                vsg::visit<SetGlobalPipelineStates>(rootNode);
                //viewer->compile();
                //myQTWidget->show();
                //container->show();
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
            try {
                NodeData nodeSpec = snode;
                configmaps::ConfigMap spec;
                nodeSpec.toConfigMap(&spec, false, false);
                DrawObject *drawObject = new DrawObject();
                drawObject->createObject(spec);
                drawObjects[nextDrawID++] = drawObject;
                if(!activated)
                {
                    drawObject->setVisible(false);
                }
                //vsg::visit<SetGlobalPipelineStates>(rootNode);
                dirty = true;
                return nextDrawID-1;
            } catch(std::exception &e)
            {
                fprintf(stderr, "While adding DrawObject: %s: %s", snode.name.c_str(), e.what());
            }
            return 0;
        }

        unsigned long GraphicsManager::getDrawID(const std::string &name) const {(void)name; return 0;}
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
                rootNode->children.erase(std::find(rootNode->children.begin(), rootNode->children.end(), coords));
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
                rootNode->addChild(coords);
                dirty = true;
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
        void GraphicsManager::showGrid(void) {}
        void GraphicsManager::hideGrid(void) {}
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
                name += ".vsgt";
                vsg::write(rootNode, name);
            }
        }

        void GraphicsManager::setTexture(unsigned long id, const std::string &filename) {(void)id; (void)filename;}
        unsigned long GraphicsManager::new3DWindow(void *myQTWidget, bool rtt,
                                                   int width, int height, const std::string &name) {(void)myQTWidget;(void)rtt;(void)width;(void)height;(void)name;return 0;}
        void GraphicsManager::setGrabFrames(bool value) {(void)value;}
        GraphicsWindowInterface* GraphicsManager::get3DWindow(unsigned long id) const {(void)id; return 0;}
        GraphicsWindowInterface* GraphicsManager::get3DWindow(const std::string &name) const {(void)name; return 0;}
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
        void* GraphicsManager::getQTWidget(unsigned long id) const {(void)id;return container;}
        void GraphicsManager::showQTWidget(unsigned long id) {(void)id;}
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
            dirty = true;
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
            (void)id;
            static Vector dummy;
            return dummy;
        }
        const utils::Quaternion& GraphicsManager::getDrawObjectQuaternion(unsigned long id)
        {
            (void)id;
            static  Quaternion dummy;
            return dummy;
        }

        void GraphicsManager::draw()
        {
            // todo: remove draw handling via nsview
            for(auto& graphicsUpdateObject: graphicsUpdateObjects)
            {
                graphicsUpdateObject->preGraphicsUpdate();
            }
            GuiHelper::worldTransformUniform->value().projInverse = perspective->inverse();
            GuiHelper::worldTransformUniform->value().viewInverse = lookAt->inverse();
            GuiHelper::worldTransformUniform->dirty();
            // fprintf(stderr, ". ");
            // // pass any events into EventHandlers assigned to the Viewer
            if(dirty)
            {
                viewer->compile();
                dirty = false;
            }
            // could also try
            viewer->render();
            // viewer->handleEvents();
            // viewer->update();
            // viewer->recordAndSubmit();
            // viewer->present();
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
        void GraphicsManager::addOSGNode(void* node) {(void)node;}
        void GraphicsManager::removeOSGNode(void* node) {(void)node;}
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
                    viewer->compile();
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
        }

    } // end of namespace vsg_graphics
} // end of namespace mars


DESTROY_LIB(mars::vsg_graphics::GraphicsManager);
CREATE_LIB(mars::vsg_graphics::GraphicsManager);
