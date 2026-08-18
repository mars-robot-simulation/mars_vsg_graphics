#include <QWidget>
#include <QVBoxLayout>

#include "GraphicsWidget.hpp"
#include "GraphicsCamera.hpp"
#include "GraphicsManager.hpp"
#include "ViewDependentState.hpp"

#include <mars_utils/Color.h>

#include <iostream>
#include <string>

#include <opencv2/opencv.hpp>

#define CULL_LAYER (1 << (widgetID-1))

#include "ImageUtils.hpp"

namespace mars
{
    namespace vsg_graphics
    {

        using namespace std;
        using namespace interfaces;
        using std::cout;
        using std::cerr;
        using std::endl;

        void EventHandler::apply(vsg::KeyPressEvent& keyPress)
        {
            //cout << "key press: " << keyPress.keyModifier << "\tkey: " << keyPress.keyBase << endl;
            if(activeHandler)
            {
                bool active = true;
                activeHandler->keyPressEvent(keyPress, active);
                if(!active)
                {
                    activeHandler->setActive(false);
                    activeHandler = nullptr;
                }
            }
        }

        void EventHandler::apply(vsg::ButtonPressEvent& event)
        {
            gw->mouseX = event.x;
            gw->mouseY = event.y;
            bool handlerClick = false;
            if(gw->pick(event.x, event.y))
            {

                //LOG_ERROR("test interactionHandlers");
                for(auto &it: interactionHandlers)
                {
                    //LOG_ERROR("-------");
                    if(it->haveInteraction(gw->pickNodePath))
                    {
                        if(activeHandler != it)
                        {
                            if(activeHandler)
                            {
                                activeHandler->setActive(false);
                            }
                            activeHandler = it;
                            activeHandler->setActive(true);
                        }
                        handlerClick = true;
                        break;
                    }
                }
                if(!handlerClick && activeHandler)
                {
                    activeHandler->setActive(false);
                    activeHandler = nullptr;
                }
                if(!activeHandler)
                {
                    if(gw->gm->handlePickEvent(gw->intersection))
                    {
                        event.handled = true;
                    }
                }
            }
            else if(activeHandler)
            {
                activeHandler->pointerClickEvent(event.x, event.y);
            }
        }

        void EventHandler::apply(vsg::ButtonReleaseEvent& event)
        {
            //activeHandler = nullptr;
            if(activeHandler)
            {
                activeHandler->pointerReleaseEvent(event.x, event.y);
            }
            else if(gw->mouseX - event.x < 5 && gw->mouseY - event.y < 5)
            {
                // todo: apply selection
                gw->pick(event.x, event.y);
            }
            if(gw->gm->handleReleaseEvent())
            {
                event.handled = true;
            }
        }

        void EventHandler::apply(vsg::MoveEvent& moveEvent)
        {
            if(activeHandler)
            {
                moveEvent.handled = activeHandler->pointerMoveEvent(moveEvent.x-gw->mouseX,
                                                                    moveEvent.y-gw->mouseY);
            }
            gw->mouseX = moveEvent.x;
            gw->mouseY = moveEvent.y;
            //cout << "moveEvent: " << moveEvent.x << " " << moveEvent.y << endl;
        }

        GraphicsWidget::GraphicsWidget(void* parent,
                                       vsg::ref_ptr<vsg::Group> scene_,
                                       unsigned long id,
                                       bool isRTTWidget_,
                                       GraphicsManager* gm_):
            graphicsCamera(nullptr), gm{gm_}, hasFocus{false}
        {
            // todo: check parent handling
            (void)parent;

            widgetID = id;

            isRTTWidget = isRTTWidget_;
            isStereoDisplay = isFullscreen = false;
            isMouseMoving = isMouseButtonDown = false;
            isHUDShown = true;
            widgetX = 20;
            widgetY = 50;
            widgetWidth = 720;
            widgetHeight = 405;

            cameraEyeSeparation = 0.1;
            mouseX = mouseY = 0;
            //pickmode = DISABLED;

            scene = scene_;
            //view = new osgViewer::View;
            name = "3D View";
            clearColor = {0.2, 0.2, 0.2, 1.0};
        }

        GraphicsWidget::~GraphicsWidget()
        {
            /* if the destructor is called from somewhere else than osg
             * (e.g. from the QWidget) we have to increment the referece counter
             * to prevent osg from calling the destructor one more time.
             */
            if(graphicsCamera)
            {
                delete graphicsCamera;
            }

            if(gm)
            {
                fprintf(stderr, "~GraphicsWidget: do we have to remove the widget from manager?\n");
                //gm->removeGraphicsWidget(widgetID);
            }
            delete window;
        }


        void GraphicsWidget::initialize(void* data,
                                        GraphicsWidget* shared, int width, int height,
                                        bool vsync)
        {
            (void) data;
            (void) vsync;

            if(!gm)
            {
                LOG_ERROR("GraphicsWidget::initialize no GraphicsManager available!");
            }

            if(shared && shared->traits)
            {
                LOG_ERROR("GraphicsWidget::initialize have shared traits!");
                traits = shared->traits;
            } else
            {
                traits = vsg::WindowTraits::create();
                traits->windowTitle = name;
                traits->swapchainPreferences.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }

            if(width > 0 && height > 0)
            {
                traits->width = width;
                traits->height = height;
            } else
            {
                traits->width = widgetWidth;
                traits->height = widgetHeight;
            }
            auto clearColor_ = vsg::vec4(clearColor.r, clearColor.g, clearColor.b, clearColor.a);

#ifdef XRTEST
            vsg::ref_ptr<vsgvr::Traits> xrTraits;
            vsgvr::VulkanRequirements xrVulkanReqs;
            if(gm->xr)
            {
                if(!isRTTWidget)
                {
                    xrTraits = vsgvr::Traits::create();
                    xrTraits->applicationName = "VSGVR Generic OpenXR Example";
                    xrTraits->setApplicationVersion(0, 0, 0);
                    xrTraits->setEngineVersion(1, 1, 0);
                    //xrTraits->xrExtensions.push_back("XR_BD_controller_interaction");
                    gm->xrInstance = vsgvr::Instance::create(XrFormFactor::XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY, xrTraits);
                    xrTraits->viewConfigurationType = selectViewConfigurationType(gm->xrInstance);
                    xrTraits->environmentBlendMode = selectEnvironmentBlendMode(gm->xrInstance, xrTraits->viewConfigurationType);

                    // Retrieve the vulkan requirements - The OpenXR runtime will require certain vulkan versions,
                    // along with a specific physical device, and instance/device extensions
                    xrVulkanReqs = vsgvr::GraphicsBindingVulkan::getVulkanRequirements(gm->xrInstance);
                    configureXrVulkanRequirements(traits, xrVulkanReqs);
                }
            }
#endif
            
            //auto clearColor = vsg::vec4(0.2f, 0.2f, 0.7f, 1.0f);
            // todo: evaluate msaa
            VkSampleCountFlagBits samples = false ? VK_SAMPLE_COUNT_8_BIT : VK_SAMPLE_COUNT_1_BIT;

            if(isRTTWidget)
            {
                if(!shared)
                {
                    LOG_ERROR("GraphicsWidget create RTT window only support as shared version at the moment.");
                    return;
                }

                LOG_ERROR("GraphicsWidget create RTT window. (%d, %d)", width, height);

                graphicsCamera = new GraphicsCamera(width, height);
                VkExtent2D targetExtent{(unsigned int)width, (unsigned int)height};
                auto context = vsg::Context::create(traits->device);

                VkFormat offscreenImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
                VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

                // todo: add msaa handling
                auto transferImageView = createTransferImageView(offscreenImageFormat, targetExtent, VK_SAMPLE_COUNT_1_BIT);
                auto transferDepthImageView = createTransferImageView(depthFormat, targetExtent, VK_SAMPLE_COUNT_1_BIT);
                captureImage = createCaptureImage(offscreenImageFormat, targetExtent);
                LOG_ERROR("GraphicsWidget: Store captureImage for %s", name.c_str());
                GuiHelper::fbCaptureImages[name] = captureImage;
                captureDepthImage = createCaptureImage(depthFormat, targetExtent);
                auto captureCommands = createTransferCommands(transferImageView->image,
                                                              captureImage);
                auto captureDepthCommands = createTransferCommands(transferDepthImageView->image,
                                                                   captureDepthImage);
                renderGraph = vsg::RenderGraph::create();
                renderGraph->framebuffer = createOffscreenFramebuffer(transferImageView, transferDepthImageView, samples);

                renderGraph->renderArea.offset = {0, 0};
                renderGraph->renderArea.extent = renderGraph->framebuffer->extent2D();
                renderGraph->viewportState->set(0, 0, width, height);

                //auto renderGraph = createOffscreenRendergraph(*context, targetExtent);
                // Vulkan default depth buffer should be cleared with 1.0, somehow with vsg we have to clear it with 0.0
                // we assume that vsg configures vulkan to use reversed depth to have more precision on long distances
                renderGraph->setClearValues(vsg::sRGB_to_linear(clearColor_), VkClearDepthStencilValue{0.0f, 0});

                auto view = vsg::View::create(graphicsCamera->camera);
                view->viewDependentState = ViewDependentState::create(view);
                renderGraph->addChild(view);
                contentGroup = vsg::Group::create();
                contentGroup->addChild(gm->rootNode);
                view->addChild(contentGroup);

                commandGraph = vsg::CommandGraph::create(*(shared->window));
                commandGraph->submitOrder = -widgetID; // render before the main_commandGraph
                commandGraph->addChild(renderGraph);
                // todo: integrate switch from vsgoffsreen example to allow defined framerate for capturing
                commandGraph->addChild(captureCommands);
                commandGraph->addChild(captureDepthCommands);

                gm->viewer->addRecordAndSubmitTaskAndPresentation({commandGraph});

            } else
            {
                // todo: creaet option without qt
                //auto window = vsg::Window::create(traits);
                window = new vsgQt::Window(gm->viewer, traits, (QWindow*)nullptr);
                window->setTitle(traits->windowTitle.c_str());
                window->initializeWindow();
                //window->setFocusPolicy(Qt::StrongFocus);

                if (!window)
                {
                    LOG_ERROR("GraphicsWidget::initialize Could not create window.");
                    return;
                }

                contentGroup = vsg::Group::create();
                contentGroup->addChild(gm->rootNode);

#ifdef XRTEST
                if(gm->xr)
                {
                    auto vkInstance = window->windowAdapter->getOrCreateInstance();
                    VkPhysicalDevice xrRequiredDevice = vsgvr::GraphicsBindingVulkan::getVulkanDeviceRequirements(gm->xrInstance, vkInstance, xrVulkanReqs);
                    vsg::ref_ptr<vsg::PhysicalDevice> physicalDevice;
                    for (auto& dev : vkInstance->getPhysicalDevices())
                    {
                        if (dev->vk() == xrRequiredDevice)
                        {
                            physicalDevice = dev;
                        }
                    }
                    if (!physicalDevice)
                    {
                        LOG_ERROR("Unable to select physical device, as required by OpenXR");
                        return;// EXIT_FAILURE;
                    }
                    window->windowAdapter->setPhysicalDevice(physicalDevice);
                    // Bind OpenXR to the desktop window's vulkan instance
                    window->windowAdapter->getOrCreateSurface();
                    auto vkDevice = window->windowAdapter->getOrCreateDevice();
                    auto graphicsBinding = vsgvr::GraphicsBindingVulkan::create(vkInstance, physicalDevice, vkDevice, physicalDevice->getQueueFamily(VK_QUEUE_GRAPHICS_BIT), 0);
                    traits->device = vkDevice;
                    
                    // Configure the OpenXR session, managed by a vsgvr Viewer
                    // As part of this, perform further trait validation, and selection of appropriate rendering parameters
                    gm->vrViewer = vsgvr::Viewer::create(gm->xrInstance, graphicsBinding);
                
                    auto vrSession = gm->vrViewer->getSession();
                    xrTraits->swapchainFormat = selectSwapchainFormat(vrSession);
                    xrTraits->swapchainSampleCount = selectSwapchainSampleCount(vrSession, traits->samples);
                    // Lastly define the world space used for the application, from one of the OpenXR reference spaces
                    // This is required both for rendering, and as a reference to locate tracked devices within.
                    auto referenceSpaceType = selectReferenceSpaceType(vrSession);
                    // Configure world space to be the origin of the selected reference space type
                    // Our reference space may be rotated or translated if required
                    auto referenceSpace = vsgvr::ReferenceSpace::create(vrSession, referenceSpaceType);
                    gm->vrViewer->referenceSpace = referenceSpace;

                    // Configure rendering of the vsg scene into a composition layer
                    // Multiple composition layers may be provided, but at minimum a single CompositionLayerProjection is needed
                    // to display the scene within the headset / OpenXR displays.
                    //
                    // CompositionLayerProjection is somewhat special in that it doesn't represent an object within the world,
                    // rather it is always bound to the headset, and cameras / views within the layer will be auto-assigned.
                    //
                    // Other object-based layers such as CompositionLayerQuad appear within the world as textured objects,
                    // and may be moved / rotated by defining another ReferenceSpace, based upon our defined world space
                    gm->headsetCompositionLayer = vsgvr::CompositionLayerProjection::create(referenceSpace);
                    auto xrGroup = vsg::Group::create();
                    gm->userOrigin = vsgvr::UserOrigin::create();
                    xrGroup->addChild(gm->userOrigin);
                    gm->userOrigin->addChild(contentGroup);
                    auto xrCommandGraphs = gm->headsetCompositionLayer->createCommandGraphsForView(gm->xrInstance, vrSession, xrGroup, gm->xrCameras, false);
                    auto xrRenderGraph = xrCommandGraphs[0]->children[0]->cast<vsg::RenderGraph>();
                    xrRenderGraph->setClearValues(vsg::sRGB_to_linear(clearColor_), VkClearDepthStencilValue{0.0f, 0});
                    auto xrView = xrRenderGraph->children[0]->cast<vsg::View>();
                    xrView->viewDependentState = ViewDependentState::create(xrView);

                    gm->headsetCompositionLayer->assignRecordAndSubmitTask(xrCommandGraphs);
                    gm->headsetCompositionLayer->compile();
                    gm->vrViewer->compositionLayers.push_back(gm->headsetCompositionLayer);
                }
                else if(!traits->device)
                {
                    traits->device = window->windowAdapter->getOrCreateDevice();
                }
#else
                // if this is the first window to be created, use its device for future window creation.
                if(!traits->device)
                {
                    traits->device = window->windowAdapter->getOrCreateDevice();
                }
#endif
                GuiHelper::device = traits->device;

                uint32_t width_ = window->traits->width;
                uint32_t height_ = window->traits->height;
                fprintf(stderr, "-------- with: %u\theight: %u\n", width_, height_);

                graphicsCamera = new GraphicsCamera(width_, height_);

                eventHandler = EventHandler::create();
                eventHandler->gw = this;
                gm->viewer->addEventHandler(eventHandler);

                auto trackball = vsg::Trackball::create(graphicsCamera->camera);
                trackball->addWindow(*window);
                gm->viewer->addEventHandler(trackball);

                auto view = vsg::View::create(graphicsCamera->camera);
                // try to override view dependent state implementation
                view->viewDependentState = ViewDependentState::create(view);
                view->addChild(contentGroup);

                // set up the render graph
                renderGraph = vsg::RenderGraph::create(*window, view);
                renderGraph->contents = VK_SUBPASS_CONTENTS_INLINE;

                renderGraph->setClearValues(vsg::sRGB_to_linear(clearColor_));
                commandGraph = vsg::CommandGraph::create(*window, renderGraph);
                //auto commandGraph = vsg::createCommandGraphForView(*window, camera, gm->rootNode);

                //viewer->addRecordAndSubmitTaskAndPresentation({commandGraph});
                // todo: second window replaces first one at the moment
                gm->viewer->addRecordAndSubmitTaskAndPresentation({commandGraph});

#ifdef WIN32d
                container = new QWidget();
                container->setGeometry(window->traits->x, window->traits->y, window->traits->width, window->traits->height);

                auto container2 = QWidget::createWindowContainer(window, nullptr);
                container2->setGeometry(window->traits->x, window->traits->y, window->traits->width, window->traits->height);
                //container2->setFocusPolicy(Qt::StrongFocus);
                auto layout = new QVBoxLayout();
                layout->addWidget(container2);
                container->setLayout(layout);
                //container2->setParent(container);
                
#else
                container = QWidget::createWindowContainer(window, nullptr);
                container->setGeometry(window->traits->x, window->traits->y, window->traits->width, window->traits->height);
                container->setFocusPolicy(Qt::StrongFocus);
#endif
                // for none rtt widgets we create an overlay view
                overlayGroup = vsg::Group::create();
                VkExtent2D targetExtent{(unsigned int)width_, (unsigned int)height_};
                double radius = 1.0;
                auto lookAt = vsg::LookAt::create(vsg::dvec3(radius * 2.0, 0.0, 0.0), vsg::dvec3(0.0, 0.0, 0.0), vsg::dvec3(0.0, 0.0, 1.0));
                double ratio = static_cast<double>(width_) / static_cast<double>(height_);
                auto perspective = vsg::Perspective::create(30.0, ratio, 0.001 * radius, radius * 100.5);
                auto ortho = vsg::Orthographic::create(-0.5*ratio, 0.5*ratio,
                                                       -0.5, 0.5,
                                                       0.001 * radius, radius * 100.5);
                auto camera = vsg::Camera::create(ortho, lookAt, vsg::ViewportState::create(targetExtent));
                overlayView = vsg::View::create(camera);
                // try to override view dependent state implementation
                overlayView->viewDependentState = ViewDependentState::create(overlayView);
                overlayView->addChild(overlayGroup);
                renderGraph->addChild(overlayView);
            }
        }

        unsigned long GraphicsWidget::getID(void)
        {
            return widgetID;
        }

        // osgViewer::View* GraphicsWidget::getView()
        // {
        //     return view;
        // }

        // mars::utils::Vector GraphicsWidget::getMousePos()
        // {
        //     return mars::utils::Vector(mouseX, mouseY, 0.0);
        // }

        void GraphicsWidget::setFullscreen(bool val, int display)
        {
            (void)val;
            (void)display;
            LOG_ERROR("GraphicsWidget: setFullscreen no implemented yet!");
        }

        void GraphicsWidget::addGraphicsEventHandler(GraphicsEventInterface* _graphicsEventHandler)
        {
            this->graphicsEventHandler.push_back(_graphicsEventHandler);
        }

        GraphicsCameraInterface* GraphicsWidget::getCameraInterface(void) const
        {
            return graphicsCamera;
        }

        void GraphicsWidget::switchHudElemtVis(int num_element)
        {
            (void)num_element;
        }

        void GraphicsWidget::setGrabFrames(bool grab)
        {
            (void) grab;
            LOG_ERROR("GraphicsWidget: setGrabFrames no implemented yet!");
            //if(!isRTTWidget) postDrawCallback->setGrab(grab);
        }

        void GraphicsWidget::setSaveFrames(bool grab)
        {
            (void) grab;
            LOG_ERROR("GraphicsWidget: setSaveFrames no implemented yet!");
        }

        void GraphicsWidget::setClearColor(mars::utils::Color color)
        {
            clearColor = color;
            auto clearColor_ = vsg::vec4(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
            if(renderGraph)
            {
                renderGraph->setClearValues(vsg::sRGB_to_linear(clearColor_));
            }
        }

        const mars::utils::Color& GraphicsWidget::getClearColor() const
        {
            return clearColor;
        }

        void GraphicsWidget::getImageData(char* buffer, int& width, int& height)
        {
            if(isRTTWidget)
            {
                if((unsigned int)width != captureImage->extent.width)
                {
                    LOG_ERROR("GraphcisWidget::getImageData width doesn't fit to image of framebuffer");
                    return;
                }
                if((unsigned int)height != captureImage->extent.height)
                {
                    LOG_ERROR("GraphcisWidget::getImageData heigt doesn't fit to image of framebuffer");
                    return;
                }
                auto imageData = vsg_graphics::getImageData(gm->viewer, captureImage);
                // in openscenegraph / mars_graphics images where flipped; to stay compatible we flip
                // the image as well
                char* destinationBuffer = (char*)imageData->dataPointer();
                for(int i=0; i<height; ++i)
                {
                    memcpy(buffer+(height-i-1)*width*4, destinationBuffer+i*width*4, width*4);
                }

            } else
            {
                LOG_ERROR("GraphicsWidget: getImageData no implemented for non rtt windows!");
            }
        }

        void GraphicsWidget::getImageData(void **data, int &width, int &height)
        {
            if(isRTTWidget)
            {
                width = captureImage->extent.width;
                height = captureImage->extent.height;
                *data = malloc(width*height*4);
                getImageData((char *) *data, width, height);
            } else
            {
                LOG_ERROR("GraphicsWidget: getImageData no implemented for non rtt windows!");
            }
        }

        void GraphicsWidget::getRTTDepthData(float* buffer, int& width, int& height)
        {
            if(isRTTWidget)
            {
                if((unsigned int)width != captureDepthImage->extent.width)
                {
                    LOG_ERROR("GraphcisWidget::getRTTDepthData width doesn't fit to image of framebuffer");
                    return;
                }
                if((unsigned int)height != captureDepthImage->extent.height)
                {
                    LOG_ERROR("GraphcisWidget::getRTTDepthData heigt doesn't fit to image of framebuffer");
                    return;
                }
                double Zn, Zf; //fovy, aspectRatio
                //fovy = graphicsCamera->perspective->fieldOfViewY;
                //aspectRatio = graphicsCamera->perspective->aspectRatio;
                Zn  = graphicsCamera->perspective->nearDistance;
                Zf  = graphicsCamera->perspective->farDistance;
                auto imageData = vsg_graphics::getImageData(gm->viewer, captureDepthImage);
                int d = 0;
                float *data2 = (float*)(imageData->dataPointer());
                float di;
                for(int i=0; i<height; ++i)
                {
                    for(int k=0; k<width; ++k)
                    {
                        di = data2[i*width+k];
                        //if(di > 0.0001) fprintf(stderr, " %f", di);
                        //const double dv = ((double) di) / std::numeric_limits< float >::max() ;
                        // the documentation of vulkan says that most far value in depth buffer should be 1
                        // but here we figured out that the depht buffer seems to be inverted.
                        // Todo: we have to verify that the calculation gives the correct distances
                        const double dv = 1.0-di;
                        //const double di2 = (float)(Zn*Zf/(Zf-dv*(Zf-Zn)));
                        //if(di > 0.0001) fprintf(stderr, "/%f", di2);
                        // 1.0 is the max depth in the depth buffer, and
                        // is represented as a nan in the distance image
                        if( dv >= 1.0 )
                            buffer[d++] = std::numeric_limits<float>::quiet_NaN();
                        else
                            buffer[d++] = (float)(Zn*Zf/(Zf-dv*(Zf-Zn)));
                    }
                    //fprintf(stderr, "\n");
                }
            } else
            {
                LOG_ERROR("GraphicsWidget: getRTTDepthData no implemented for non rtt windows!");
            }
        }

        void GraphicsWidget::getRTTDepthData(float **data, int &width, int &height) {
            if(isRTTWidget)
            {
                width = captureDepthImage->extent.width;
                height = captureDepthImage->extent.height;
                *data = (float*)malloc(width*height*4);
                getRTTDepthData(*data, width, height);
            } else
            {
                LOG_ERROR("GraphicsWidget: getRTTDepthData no implemented for non rtt windows!");
            }
        }

        void GraphicsWidget::setHUDViewOffsets(double x1, double y1,
                                               double x2, double y2)
        {
            (void) x1;
            (void) y1;
            (void) x2;
            (void) y2;
            LOG_ERROR("GraphicsWidget: setHUDViewOffset no implemented and deprecated!");
        }

        void GraphicsWidget::setupDistortion(double factor)
        {
            (void) factor;
            LOG_ERROR("GraphicsWidget: setupDistortion no implemented yet!");
        }

        void GraphicsWidget::writeRTTImages(void)
        {
            if(isRTTWidget)
            {
                LOG_ERROR("GraphicsWidget::writeRTTImage called");
                auto imageData = vsg_graphics::getImageData(gm->viewer, captureImage);
                vsg::write(imageData, "color_image.vsgt");
                imageData = vsg_graphics::getImageData(gm->viewer, captureDepthImage);
                vsg::write(imageData, "depth_image.vsgt");
                // cv::Mat image=cv::Mat(cv::Size(captureImage->extent.width, captureImage->extent.height),
                //                       CV_8UC4, imageData->dataPointer(), cv::Mat::AUTO_STEP);
                // cv::Mat converted;
                // cv::cvtColor(image, converted, cv::COLOR_RGBA2BGRA);
                // cv::imwrite("foo.jpg", converted);
                //cv::Mat img;
                //imageData.data()
            }
        }

        std::vector<const vsg::MatrixTransform*> GraphicsWidget::getPickedObjects()
        {
            return pickedObjects;
        }

        void GraphicsWidget::clearSelectionVectors(void)
        {
            pickedObjects.clear();
        }

        bool GraphicsWidget::pick(const double x, const double y)
        {
            auto intersector = vsg::LineSegmentIntersector::create(*(graphicsCamera->camera), x, y);
            contentGroup->accept(*intersector);
            if (intersector->intersections.empty()) return false;
            // sort the intersections front to back
            intersection = intersector->intersections[0];
            for(auto &it : intersector->intersections)
            {
                if(it->ratio < intersection->ratio)
                {
                    intersection = it;
                }
            }
            //LOG_ERROR("found intersection at %g %g %g", intersection->worldIntersection.x, intersection->worldIntersection.y, intersection->worldIntersection.z);
            //const vsg::MatrixTransform* transform;

            pickNodePath = intersection->nodePath;
            // for (auto node : intersection->nodePath)
            // {
            //     transform = node->cast<vsg::MatrixTransform>();
            //     if(transform)
            //     {
            //         LOG_ERROR("... found transform");
            //         pickedObjects.push_back(transform);
            //         return true;
            //     }
            // }
            if(intersection->nodePath.size() > 0) return true;
            return false;
        }

    } // end of namespace vsg_graphics
} // end of namespace mars
