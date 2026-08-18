#include <QWidget>

#include "GraphicsManager.hpp"
#include "GraphicsCamera.hpp"
#include "DrawObject.hpp"
#include "Grid.hpp"
#include "config.h"
#include "ImageUtils.hpp"
#include <vsgXchange/all.h>
#include <mars_utils/misc.h>
#include <mars_utils/mathUtils.h>

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
            uvPointerTrackingActive = false;
#ifdef XRTEST
            grabActive = false;
            grabValue = false;
            selectActive = false;
            toggleActive = false;
            cancelActive = false;
            xr = false;
#endif
            vsg::Logger::instance()->level = vsg::Logger::LOGGER_INFO;
            fprintf(stderr, "Create GraphicsManager Instance\n");
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

        void GraphicsManager::initializeXR()
        {
#ifdef XRTEST
            if(!xr) return;
            headPose = vsgvr::SpaceBinding::create(vsgvr::ReferenceSpace::create(vrViewer->getSession(), XrReferenceSpaceType::XR_REFERENCE_SPACE_TYPE_VIEW));
            vrViewer->spaceBindings.push_back(headPose);
            std::map<std::string, std::list<vsgvr::ActionSet::SuggestedInteractionBinding>> actionsToSuggest;
            // actionsToSuggest["/interaction_profiles/khr/simple_controller"] = {
            //     {_leftHandPose, "/user/hand/left/input/aim/pose"},
            //     {_rightHandPose, "/user/hand/right/input/aim/pose"},
            //     {_switchInteractionAction, "/user/hand/right/input/select/click"},
            // };
            // actionsToSuggest["/interaction_profiles/oculus/touch_controller"] = {
            //     {_leftHandPose, "/user/hand/left/input/aim/pose"},
            //     {_rightHandPose, "/user/hand/right/input/aim/pose"},
            //     // A boolean action on a float input will be converted by the OpenXR runtime
            //     {_switchInteractionAction, "/user/hand/right/input/trigger/value"},
            // };
            actionSet = vsgvr::ActionSet::create(xrInstance, "slide", "Slide");
            rightHandPose = vsgvr::ActionPoseBinding::create(xrInstance, actionSet, "right_hand", "Right Hand");
            leftHandPose = vsgvr::ActionPoseBinding::create(xrInstance, actionSet, "left_hand", "Left Hand");
            strafeXAction = vsgvr::Action::create(xrInstance, actionSet, XrActionType::XR_ACTION_TYPE_FLOAT_INPUT, "strafe_x", "Strafe X");
            strafeYAction = vsgvr::Action::create(xrInstance, actionSet, XrActionType::XR_ACTION_TYPE_FLOAT_INPUT, "strafe_y", "Strafe Y");
            grabAction = vsgvr::Action::create(xrInstance, actionSet, XrActionType::XR_ACTION_TYPE_BOOLEAN_INPUT, "grab", "Grab");
            quitAction = vsgvr::Action::create(xrInstance, actionSet, XrActionType::XR_ACTION_TYPE_BOOLEAN_INPUT, "quit", "Quit");
            selectAction = vsgvr::Action::create(xrInstance, actionSet, XrActionType::XR_ACTION_TYPE_BOOLEAN_INPUT, "select", "Select");
            toggleAction = vsgvr::Action::create(xrInstance, actionSet, XrActionType::XR_ACTION_TYPE_BOOLEAN_INPUT, "toggle", "Toggle");
            cancelAction = vsgvr::Action::create(xrInstance, actionSet, XrActionType::XR_ACTION_TYPE_BOOLEAN_INPUT, "cancel", "Cancel");
            actionSet->actions = {
                rightHandPose,
                leftHandPose,
                strafeXAction,
                strafeYAction,
                grabAction,
                quitAction,
                selectAction,
                toggleAction,
                cancelAction
            };
            actionsToSuggest["/interaction_profiles/oculus/touch_controller"] = {
                {strafeXAction, "/user/hand/left/input/thumbstick/x"},
                {strafeYAction, "/user/hand/left/input/thumbstick/y"},
                {grabAction, "/user/hand/right/input/trigger/value"},
                {quitAction, "/user/hand/right/input/b/click"},
                {selectAction, "/user/hand/right/input/a/click"},
                {toggleAction, "/user/hand/left/input/x/click"},
                {cancelAction, "/user/hand/left/input/y/click"},
                {rightHandPose, "/user/hand/right/input/aim/pose"},
                {leftHandPose, "/user/hand/left/input/aim/pose"},
                //{rotateAction, "/user/hand/right/input/thumbstick/x"}
            };
            char* vive_ = getenv("VIVE");
            if(vive_)
            {
                std::string vive = vive_;
                if(vive == "1" || tolower(vive) == "true")
                {
                    actionsToSuggest["/interaction_profiles/htc/vive_focus3_controller"] = {
                        {strafeXAction, "/user/hand/left/input/thumbstick/x"},
                        {strafeYAction, "/user/hand/left/input/thumbstick/y"},
                        {grabAction, "/user/hand/right/input/trigger/value"},
                        {quitAction, "/user/hand/right/input/b/click"},
                        {selectAction, "/user/hand/right/input/a/click"},
                        {toggleAction, "/user/hand/left/input/x/click"},
                        {cancelAction, "/user/hand/left/input/y/click"},
                        {rightHandPose, "/user/hand/right/input/aim/pose"},
                        {leftHandPose, "/user/hand/left/input/aim/pose"},
                    };
                }
            }

            // actionsToSuggest["/interaction_profiles/bytedance/pico4_controller"] = {
            //     {strafeXAction, "/user/hand/left/input/thumbstick/x"},
            //     {strafeYAction, "/user/hand/left/input/thumbstick/y"},
            //     //{rotateAction, "/user/hand/right/input/thumbstick/x"}
            // };
            for (auto& p : actionsToSuggest)
            {
                if (! vsgvr::ActionSet::suggestInteractionBindings(xrInstance, p.first, p.second))
                {
                    throw std::runtime_error("Failed to configure interaction bindings for controllers");
                }
            }
            vrViewer->actionSets.push_back(actionSet);
            vrViewer->activeActionSets.push_back(actionSet);
            originPosition = vsg::dvec3(0.0, 0.0, 0.0);
            originRotation = 0.0;
            lastFrameTime = vsg::clock::now();
#endif
        }

        void GraphicsManager::initializeOSG(void *data, bool createWindow)
        {
            (void)data;
            fprintf(stderr, "GraphicsManager – initializeOSG\n");
            if(!viewer) {
                cfg = libManager->getLibraryAs<cfg_manager::CFGManagerInterface>("cfg_manager");
                if(!cfg)
                {
                    fprintf(stderr, "******* mars_graphics: couldn't find cfg_manager\n");
                    return;
                }
                setupCFG();
#ifdef XRTEST
                char* xrenv_ = getenv("XR");
                if(xrenv_)
                {
                    std::string xrenv = xrenv_;
                    if(xrenv == "1" || xrenv == "true" || xrenv=="TRUE")
                    {
                        xr = true;
                    }
                }
#endif
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
                initializeXR();
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
                fpsLayout->verticalAlignment = vsg::StandardLayout::TOP_ALIGNMENT;
                fpsLayout->position = vsg::vec3(-0.0, 0.0, 0.0);
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
                    snprintf(n, 24, "%s_%llu.vsgt", name.c_str(), it.first);
                    fprintf(stderr, "export to %s", n);
                    vsg::write(it.second->contentGroup, n);
                }
            }
        }

        void GraphicsManager::setTexture(unsigned long id, const std::string &filename) {(void)id; (void)filename;}

        unsigned long GraphicsManager::new3DWindow(void *myQTWidget, bool rtt,
                                                   int width, int height, const std::string &name)
        {
            (void) myQTWidget;

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

        void GraphicsManager::handleXREvents()
        {
#ifdef XRTEST
            if(!xr) return;
            // OpenXR events must be checked first
            xrPollResult = vrViewer->pollEvents();
            if(quitAction->getStateValid())
            {
                auto qState = quitAction->getStateBool();
                if(qState.isActive)
                {
                    if(static_cast<bool>(qState.currentState))
                    {
                        exit(0);
                    }
                }
            }

            GraphicsWidget *gw = windows[1];
            // handle strafe actions
            if(strafeXAction->getStateValid() && strafeYAction->getStateValid())
            {
                auto xState = strafeXAction->getStateFloat();
                auto yState = strafeYAction->getStateFloat();
                bool updateOrigin = false;
                auto deltaT = std::chrono::duration<double, std::chrono::milliseconds::period>(now - lastFrameTime).count() * 0.001;

                lastFrameTime = now;
                if(xState.isActive && yState.isActive)
                {
                    vsg::dvec3 lRight(1, 0, 0);
                    vsg::dvec3 lForward(0, 0, -1);
                    const double strafeSensitivity = 1.8; // m/s
                    const double deadZone = 0.1;
                    vsg::dvec3 d = (lRight * static_cast<double>(xState.currentState) ) + (lForward * static_cast<double>(yState.currentState) );
                    if (vsg::length(d) > deadZone)
                    {
                        d *= strafeSensitivity * deltaT;
                        vsg::dvec3 v(0, 0, 0);
                        v = vsg::inverse(xrCameras.front()->viewMatrix->transform())*v;
                        d = vsg::inverse(xrCameras.front()->viewMatrix->transform())*d-v;
                        d.z = 0;
                        originPosition += d;
                        //originPosition.y = 0;
                        updateOrigin = true;
                    }
                }
                if(updateOrigin)
                {
                    headsetCompositionLayer->originPosition = originPosition;
                    //userOrigin->setOriginInScene(originPosition, vsg::dquat(originRotation, { 0.0, 0.0, 1.0 }), vsg::dvec3(1.0, 1.0, 1.0));
                }
            }

            // handle grab actions
            vsg::dvec3 v(0, 0, 0), s, v2, s2;
            vsg::dquat q, q2;
            bool havePose = false;

            // for picking in ui
            vsg::dvec3 start, end, pickPos;

            if(rightHandPose->getTransformValid())
            {
                havePose = true;
                vsg::decompose(rightHandPose->getTransform(), v, q, s);
                v += originPosition;
                start = v;
                end = start + q*vsg::dvec3(0.0, 0.0, -2.0);
                for(auto &c: xrClients)
                {
                    c->rightHandPoseUpdate(utils::Vector(v.x, v.y, v.z),
                                           utils::Quaternion(q.w, q.x, q.y, q.z));
                }
            }
            if(leftHandPose->getTransformValid())
            {
                vsg::decompose(leftHandPose->getTransform(), v2, q2, s2);
                v2 += originPosition;
                for(auto &c: xrClients)
                {
                    c->leftHandPoseUpdate(utils::Vector(v2.x, v2.y, v2.z),
                                          utils::Quaternion(q2.w, q2.x, q2.y, q2.z));
                }
            }

            // add pick handling for pointer device
            // we have some performance issues and don't know if the line intersector might be the reason
            // so we test a simple workaround for our special use-case (imgui)
            if(headPose->getTransformValid() && havePose)
            {
                // auto viewMatrix = vsg::inverse(headPose->getTransform());
                // // transform pointer ray into view
                // start = viewMatrix*(start-originPosition);
                // end = viewMatrix*(end-originPosition);
                // // project ray on z -1.0 plane (where we have the imgui)
                // double zTarget = -1.0 - start.z;
                // pickPos = end-start;
                // double scale = zTarget/pickPos.z;
                // pickPos *= scale;
                // pickPos += start;
                // // transform back into target object space
                // pickPos.x += 1.92 * 0.5;
                // pickPos.y += 1.08 * 0.5;
                // for(auto &client: uvPointerClients)
                // {
                //     for(auto &ec: client.second)
                //     {
                //         ec->pointerEvent(pickPos.x, pickPos.y);
                //     }
                // }
                for(auto &client: uvPointerClients)
                {
                    for(auto &ec: client.second)
                    {
                        ec->pointerRayEvent(utils::Vector(start.x, start.y, start.z),
                                            utils::Vector(end.x, end.y, end.z));
                    }
                }
            }

            // vsg::ref_ptr<vsg::LineSegmentIntersector::Intersection> intersection;
            // if(uvPointerTrackingActive && havePose && false)
            // {
            //     auto iter = windows.begin();
            //     if(iter != windows.end())
            //     {
            //         auto window = iter->second;
            //         auto intersector = vsg::LineSegmentIntersector::create(start, end);
            //         window->contentGroup->accept(*intersector);
            //         if(!intersector->intersections.empty())
            //         {
            //             //std::cout << "pick: " << std::endl;
            //             // sort the intersections front to back
            //             intersection = intersector->intersections[0];
            //             for(auto &it: intersector->intersections)
            //             {
            //                 if(it->ratio < intersection->ratio)
            //                 {
            //                     intersection = it;
            //                 }
            //             }
            //             handlePickEvent(intersection, false);
            //         }
            //     }
            // }

            if(grabAction->getStateValid())
            {
                auto grabState = grabAction->getStateBool();
                if(grabState.isActive)
                {
                    bool value = static_cast<bool>(grabState.currentState);
                    if(value != grabValue)
                    {
                        grabValue = value;
                        if(value)
                        {
                            if(havePose)
                            {
                                bool handled = false;
                                for(auto &client: uvPointerClients)
                                {
                                    for(auto &ec: client.second)
                                    {
                                        handled = ec->pointerRayClickEvent(utils::Vector(start.x, start.y, start.z),
                                            utils::Vector(end.x, end.y, end.z));
                                        //handled = ec->pointerClickEvent(pickPos.x, pickPos.y);
                                    }
                                }

                                // first check if we have a pick event
                                // otherwise perform grab operation
                                if(!handled)
                                    //if(intersection && !handlePickEvent(intersection))
                                {
                                    grabActive = true;
                                    if(xrClients.size() > 0)
                                    {
                                        for(auto &c: xrClients)
                                        {
                                            c->grabStart(utils::Vector(v.x, v.y, v.z), utils::Quaternion(q.w, q.x, q.y, q.z));
                                        }
                                    } else if(manipulators.size() > 0) // search for first manipulator
                                    {
                                        auto &manipulator = manipulators.begin()->second;
                                        manipulator->setStartPosition(utils::Vector(v.x, v.y, v.z));
                                        manipulator->setStartRotation(utils::Quaternion(q.w, q.x, q.y, q.z));
                                    }
                                }
                            }
                        } else
                        {
                            // create release event
                            grabActive = false;
                            handleReleaseEvent();
                        }
                    } else if(grabActive)
                    {
                        if(xrClients.size() > 0)
                        {
                            for(auto &c: xrClients)
                            {
                                c->grabMove(utils::Vector(v.x, v.y, v.z), utils::Quaternion(q.w, q.x, q.y, q.z));
                            }
                        } else if(manipulators.size() > 0) // search for first manipulator
                        {
                            auto &manipulator = manipulators.begin()->second;
                            manipulator->setGrabPosition(utils::Vector(v.x, v.y, v.z));
                            manipulator->setGrabRotation(utils::Quaternion(q.w, q.x, q.y, q.z));
                        }
                    }
                }
            }

            if(selectAction->getStateValid())
            {
                auto selectState = selectAction->getStateBool();
                if(selectState.isActive)
                {
                    bool value = static_cast<bool>(selectState.currentState);
                    if(value != selectActive)
                    {
                        selectActive = value;
                        if(value)
                        {
                            for(auto &c: xrClients)
                            {
                                c->select(utils::Vector(v.x, v.y, v.z));
                            }
                        }
                    }
                }
            }

            if(toggleAction->getStateValid())
            {
                auto toggleState = toggleAction->getStateBool();
                if(toggleState.isActive)
                {
                    bool value = static_cast<bool>(toggleState.currentState);
                    if(value != toggleActive)
                    {
                        toggleActive = value;
                        if(value)
                        {
                            for(auto &c: xrClients)
                            {
                                c->toggleMode(utils::Vector(v.x, v.y, v.z));
                            }
                        }
                    }
                }
            }

            if(cancelAction->getStateValid())
            {
                auto cancelState = cancelAction->getStateBool();
                if(cancelState.isActive)
                {
                    bool value = static_cast<bool>(cancelState.currentState);
                    if(value != cancelActive)
                    {
                        cancelActive = value;
                        if(value)
                        {
                            for(auto &c: xrClients)
                            {
                                c->cancel();
                            }
                        }
                    }
                }
            }

            {
                // todo:
                // // Match the desktop camera to the HMD view - mirroring the projectionMatrix exactly
                // // is non-optimal here, but works as a simple desktop mirror setup.

                // there is a difference of the projection system between vsgVr and vsg
                // so we have to convert the pose
                // copied directly from vsgvr:
                if(headPose->getTransformValid())
                {
                    auto cameraMatrix = headPose->getTransform();
                    vsg::dvec3 v, s;
                    vsg::dquat q, q2;
                    auto worldRotateMat = vsg::rotate(-vsg::PI / 2.0, 1.0, 0.0, 0.0);
                    vsg::decompose(cameraMatrix, v, q, s);
                    v = v+originPosition;
                    q2 = q;
                    gw->graphicsCamera->updateViewportQuat(v.x, v.y, v.z, q2.x, q2.y, q2.z, q2.w);
                    //gw->graphicsCamera->camera->viewMatrix = xrCameras.front()->viewMatrix;
                    //gw->graphicsCamera->camera->projectionMatrix = xrCameras.front()->projectionMatrix;
                }
            }
#endif
        }

        void GraphicsManager::compileXRViewer()
        {
#ifdef XRTEST
            if(!xr) return;
            for(auto &cl: vrViewer->compositionLayers)
            {
                cl->compile();
            }
#endif
        }

        void GraphicsManager::drawXR()
        {
#ifdef XRTEST
            if(!xr) return;
            // The PollEventsResult signifies that the session is running in some form.
            // In this case a frame must be rendered by the application, even if it is empty,
            // this is important to maintain sync between the application's render loop
            // and OpenXR runtime.
            if(vrViewer->advanceToNextFrame())
            {
                if(xrPollResult == vsgvr::Viewer::PollEventsResult::RunningDontRender)
                {
                    // XR Runtime requested that rendering is not performed (not visible to user)
                    // While this happens frames must still be acquired and released
                }
                else
                {
                    // Render each of the composition layers to their swapchains
                    vrViewer->recordAndSubmit();
                }
            }

            // End the frame, and present composition layers to the OpenXR compositor
            // Frames must be explicitly released, even if the previous advanceToNextFrame returned false
            vrViewer->releaseFrame();
#endif
        }

        void GraphicsManager::draw()
        {
            static unsigned long framecount = 0;
            static unsigned long framecount2 = 0;
            static auto time1 = vsg::clock::now();
            static auto time2 = vsg::clock::now();
            ++framecount;
            ++framecount2;

            now = vsg::clock::now();
            double td = std::chrono::duration<double, std::chrono::milliseconds::period>(now - time1).count();
            double td2 = std::chrono::duration<double, std::chrono::milliseconds::period>(now - time2).count();
            if(td > 100)
            {
                if(showFPS_.bValue)
                {
                    auto mainWindow = windows.find(1);
                    if(mainWindow != windows.end())
                    {
                        const int width = mainWindow->second->overlayView->camera->viewportState->viewports[0].width;
                        const int height = mainWindow->second->overlayView->camera->viewportState->viewports[0].height;
                        const double ratio = static_cast<double>(width) / static_cast<double>(height);
                        const double top = 0.5-(0.5/height)*62;
                        const double left = -0.5*ratio+(0.5*ratio/width)*20;
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

            handleXREvents();

            // todo: remove draw handling via nsview
            for(auto& graphicsUpdateObject: graphicsUpdateObjects)
            {
                graphicsUpdateObject->preGraphicsUpdate();
            }

            // todo: solve this for each camera
            auto iter = windows.begin();
#ifdef XRTEST
            if(iter != windows.end() && !xr)
#else
                if(iter != windows.end())
#endif
            {
                auto window = iter->second;
                if(uvPointerTrackingActive)
                {
                    auto intersector = vsg::LineSegmentIntersector::create(*(window->graphicsCamera->camera), window->mouseX, window->mouseY);
                    window->contentGroup->accept(*intersector);
                    if(!intersector->intersections.empty())
                    {
                        //std::cout << "pick: " << window->mouseX << " " << window->mouseY << std::endl;                                 // sort the intersections front to back
                        auto intersection = intersector->intersections[0];
                        for(auto &it: intersector->intersections)
                        {
                            if(it->ratio < intersection->ratio)
                            {
                                intersection = it;
                            }
                        }
                        handlePickEvent(intersection, false);
                    }

                }
            }
            // fprintf(stderr, ". ");
            // // pass any events into EventHandlers assigned to the Viewer
            if(dirty_)
            {
                viewer->compile();
                compileXRViewer();
                dirty_ = false;
            }
            // could also try
            //viewer->render();

            //if (viewer->advanceToNextFrame(vsg::Viewer::UseTimeSinceStartPoint))

            if (viewer->advanceToNextFrame())
            {
                viewer->handleEvents();
                for(auto& graphicsUpdateObject: graphicsUpdateObjects)
                {
                    graphicsUpdateObject->preGraphicsDrawUpdate();
                }
                viewer->update();
                viewer->recordAndSubmit();
                viewer->present();
            }

            //viewer->handleEvents();
            //viewer->update();
            //viewer->recordAndSubmit();
            //viewer->present();

            drawXR();

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
            ExternNode n{*(static_cast<vsg::ref_ptr<vsg::Node>*>(node)), windowMask};
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
            vsg::ref_ptr<vsg::Node> n(*(static_cast<vsg::ref_ptr<vsg::Node>*>(node)));
            for(auto &it: windows)
            {
                auto &contentGroup_ = it.second->contentGroup;
                auto it2 = std::find(contentGroup_->children.begin(), contentGroup_->children.end(), n);
                if(it2 != contentGroup_->children.end())
                {
                    contentGroup_->children.erase(it2);
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
#ifdef XRTEST
            xr = cfg->getOrCreateProperty("Graphics", "xr", xr, this).bValue;
#endif
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
                    if((unsigned int)w != it2->second->extent.width)
                    {
                        LOG_ERROR("GraphicsManager::captureTextureData width %d doesn't match image width %d!", w, it2->second->extent.width);
                        return;
                    }
                    if((unsigned int)h != it2->second->extent.height)
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
                    if((unsigned int)w != it2->second->extent.width)
                    {
                        LOG_ERROR("GraphicsManager::getTextureData width %d doesn't match image width %d!", w, it2->second->extent.width);
                        return;
                    }
                    if((unsigned int)h != it2->second->extent.height)
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
                    if((unsigned int)w != it2->second->extent.width)
                    {
                        LOG_ERROR("GraphicsManager::setTextureData width %d doesn't match image width %d!", w, it2->second->extent.width);
                        return;
                    }
                    if((unsigned int)h != it2->second->extent.height)
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

        void* GraphicsManager::getEngineWindow(unsigned long id)
        {
            auto wit = windows.find(id);
            if(wit == windows.end())
            {
                LOG_ERROR("GraphicsManager::getEngineWindow window with id %lu not found!", id);
                return nullptr;
            }
            return (void*)&(wit->second->window->windowAdapter);
        }

        void* GraphicsManager::getEngineDevice(unsigned long id)
        {
            // at the moment we don't support different devices
            return (void*)&(GuiHelper::device);
        }

        void* GraphicsManager::getEngineRenderPass(unsigned long id)
        {
            auto wit = windows.find(id);
            if(wit == windows.end())
            {
                LOG_ERROR("GraphicsManager::getEngineRenderPass window with id %lu not found!", id);
                return nullptr;
            }
            return (void*)(wit->second->renderGraph->getRenderPass());
        }

        void* GraphicsManager::getEngineViewer()
        {
// #ifdef XRTEST
//             if(xr) return (void*)&(vrViewer);
// #endif
            return (void*)&(viewer);
        }

        void GraphicsManager::addUVPointerClient(const std::string &node, UVPointerClient *cl)
        {
            // todo: check that we add a client only once
            uvPointerClients[node].push_back(cl);
            uvPointerTrackingActive = true;
        }

        bool GraphicsManager::handlePickEvent(vsg::ref_ptr<vsg::LineSegmentIntersector::Intersection> intersection, bool click)
        {
            auto worldPos = intersection->worldIntersection;

            // if(click)
            // {
            //     std::cout << "pick: " << worldPos.x << " " <<  worldPos.y << "   " << click << std::endl;
            // }

            // identify draw object
            bool handled = false;
            for(auto node: intersection->nodePath)
            {
                auto transform = node->cast<vsg::MatrixTransform>();
                if(transform)
                {
                    //LOG_ERROR("... found transform");
                    // compare with drawobjects
                    for(auto &obj: drawObjects)
                    {
                        if(obj.second->poseTransform == transform)
                        {
                            auto pos = vsg::inverse(transform->matrix)*worldPos;
                            //LOG_ERROR("...   found drawObject (%s)", obj.second->name.c_str());
                            // check if we have an uv tracking client
                            for(auto &client: uvPointerClients)
                            {
                                //LOG_ERROR("...   check: %s", client.first.c_str());
                                if(obj.second->name == client.first)
                                {
                                    //LOG_ERROR("...     found client");
                                    for(auto &ec: client.second)
                                    {
                                        if(click)
                                        {
                                            if(ec->pointerClickEvent(pos.x, pos.y))
                                            {
                                                handled = true;
                                            }
                                        }
                                        else
                                        {
                                            if(ec->pointerEvent(pos.x, pos.y))
                                            {
                                                handled = true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
            }
            return handled;
        }

        bool GraphicsManager::handleReleaseEvent()
        {
            bool handled = false;
            for(auto &client: uvPointerClients)
            {
                for(auto &ec: client.second)
                {
                    if(ec->pointerReleaseEvent())
                    {
                        handled = true;
                    }
                }
            }
            return handled;
        }

        void GraphicsManager::addXRClient(XRClient *c)
        {
            xrClients.push_back(c);
        }

        void GraphicsManager::removeXRClient(XRClient *c)
        {
            auto it = find(std::begin(xrClients), std::end(xrClients), c);
            if(it!=std::end(xrClients))
            {
                xrClients.erase(it);
            }
        }

    } // end of namespace vsg_graphics
} // end of namespace mars


DESTROY_LIB(mars::vsg_graphics::GraphicsManager);
CREATE_LIB(mars::vsg_graphics::GraphicsManager);
