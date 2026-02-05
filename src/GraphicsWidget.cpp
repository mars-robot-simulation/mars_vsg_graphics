#include <QWidget>

#include "GraphicsWidget.hpp"
#include "GraphicsManager.hpp"
#include "ViewDependentState.hpp"

#include <mars_utils/Color.h>

#include <iostream>
#include <string>

#include <opencv2/opencv.hpp>

#define CULL_LAYER (1 << (widgetID-1))

#include "vsgoffscreenshot.hpp"

namespace mars
{
    namespace vsg_graphics
    {

        using namespace std;
        using namespace interfaces;
        using std::cout;
        using std::cerr;
        using std::endl;

        vsg::ref_ptr<vsg::RenderGraph> GraphicsWidget::createOffscreenRendergraph(vsg::Context& context,
                                                                                  const VkExtent2D& extent)
        {
            auto device = context.device;

            VkExtent3D attachmentExtent{extent.width, extent.height, 1};
            // Attachments
            // create image for color attachment
            colorImage = vsg::Image::create();
            colorImage->imageType = VK_IMAGE_TYPE_2D;
            colorImage->format = VK_FORMAT_R8G8B8A8_SRGB;
            colorImage->extent = attachmentExtent;
            colorImage->mipLevels = 1;
            colorImage->arrayLayers = 1;
            colorImage->samples = VK_SAMPLE_COUNT_1_BIT;
            colorImage->tiling = VK_IMAGE_TILING_OPTIMAL;
            colorImage->usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            colorImage->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorImage->flags = 0;
            colorImage->sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            auto colorImageView = createImageView(context, colorImage, VK_IMAGE_ASPECT_COLOR_BIT);

            // // Sampler for accessing attachment as a texture
            // auto colorSampler = vsg::Sampler::create();
            // colorSampler->flags = 0;
            // colorSampler->magFilter = VK_FILTER_LINEAR;
            // colorSampler->minFilter = VK_FILTER_LINEAR;
            // colorSampler->mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            // colorSampler->addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            // colorSampler->addressModeV = colorSampler->addressModeU;
            // colorSampler->addressModeW = colorSampler->addressModeU;
            // colorSampler->mipLodBias = 0.0f;
            // colorSampler->maxAnisotropy = 1.0f;
            // colorSampler->minLod = 0.0f;
            // colorSampler->maxLod = 1.0f;
            // colorSampler->borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

            colorImageInfo = vsg::ImageInfo::create();
            colorImageInfo->imageView = colorImageView;
            colorImageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            colorImageInfo->sampler = nullptr;

            // create depth buffer
            VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
            depthImage = vsg::Image::create();
            depthImage->imageType = VK_IMAGE_TYPE_2D;
            depthImage->extent = attachmentExtent;
            depthImage->mipLevels = 1;
            depthImage->arrayLayers = 1;
            depthImage->samples = VK_SAMPLE_COUNT_1_BIT;
            depthImage->format = depthFormat;
            depthImage->tiling = VK_IMAGE_TILING_OPTIMAL;
            depthImage->usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            depthImage->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthImage->flags = 0;
            depthImage->sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            // // Sampler for accessing attachment as a texture
            // auto depthSampler = vsg::Sampler::create();
            // depthSampler->flags = 0;
            // depthSampler->magFilter = VK_FILTER_LINEAR;
            // depthSampler->minFilter = VK_FILTER_LINEAR;
            // depthSampler->mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            // depthSampler->addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            // depthSampler->addressModeV = depthSampler->addressModeU;
            // depthSampler->addressModeW = depthSampler->addressModeU;
            // depthSampler->mipLodBias = 0.0f;
            // depthSampler->maxAnisotropy = 1.0f;
            // depthSampler->minLod = 0.0f;
            // depthSampler->maxLod = 1.0f;
            // depthSampler->borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

            // XXX Does layout matter?
            depthImageInfo = vsg::ImageInfo::create();
            depthImageInfo->sampler = nullptr;//depthSampler;
            depthImageInfo->imageView = vsg::createImageView(context, depthImage, VK_IMAGE_ASPECT_DEPTH_BIT);
            depthImageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            // attachment descriptions
            vsg::RenderPass::Attachments attachments(2);
            // Color attachment
            attachments[0].format = VK_FORMAT_R8G8B8A8_SRGB;
            attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            // Depth attachment
            attachments[1].format = depthFormat;
            attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            vsg::AttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
            vsg::AttachmentReference depthReference = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
            vsg::RenderPass::Subpasses subpassDescription(1);
            subpassDescription[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescription[0].colorAttachments.emplace_back(colorReference);
            subpassDescription[0].depthStencilAttachments.emplace_back(depthReference);

            vsg::RenderPass::Dependencies dependencies(2);

            // XXX This dependency is copied from the offscreenrender.cpp
            // example. I don't completely understand it, but I think its
            // purpose is to create a barrier if some earlier render pass was
            // using this framebuffer's attachment as a texture.
            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            // This is the heart of what makes Vulkan offscreen rendering
            // work: render passes that follow are blocked from using this
            // passes' color attachment in their fragment shaders until all
            // this pass' color writes are finished.
            dependencies[1].srcSubpass = 0;
            dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            auto renderPass = vsg::RenderPass::create(device, attachments, subpassDescription, dependencies);

            // Framebuffer
            auto fbuf = vsg::Framebuffer::create(renderPass, vsg::ImageViews{colorImageInfo->imageView, depthImageInfo->imageView}, extent.width, extent.height, 1);

            auto rendergraph = vsg::RenderGraph::create();
            rendergraph->renderArea.offset = VkOffset2D{0, 0};
            rendergraph->renderArea.extent = extent;
            rendergraph->framebuffer = fbuf;

            rendergraph->clearValues.resize(2);
            rendergraph->clearValues[0].color = vsg::sRGB_to_linear(0.4f, 0.2f, 0.4f, 1.0f);
            rendergraph->clearValues[1].depthStencil = VkClearDepthStencilValue{1.0f, 0};

            return rendergraph;
        }

        GraphicsWidget::GraphicsWidget(void* parent,
                                       vsg::ref_ptr<vsg::Group> scene,
                                       unsigned long id,
                                       bool isRTTWidget,
                                       GraphicsManager* gm):
            gm{gm}, hasFocus{false}
        {
            // todo: check parent handling
            (void)parent;

            widgetID = id;

            this->isRTTWidget = isRTTWidget;
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

            this->scene = scene;
            //view = new osgViewer::View;
            name = "3D View";
            clearColor = {0.2, 0.2, 0.7, 1.0};
        }

        GraphicsWidget::~GraphicsWidget()
        {
            /* if the destructor is called from somewhere else than osg
             * (e.g. from the QWidget) we have to increment the referece counter
             * to prevent osg from calling the destructor one more time.
             */
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
            }

            //traits->width = width;
            //traits->height = height;
            auto clearColor_ = vsg::vec4(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
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

                LOG_ERROR("GraphicsWidget create RTT window.");
                VkExtent2D targetExtent{(unsigned int)width, (unsigned int)height};
                double radius = 1.0;
                lookAt = vsg::LookAt::create(vsg::dvec3(radius * 2.0, 0.0, 0.0), vsg::dvec3(0.0, 0.0, 0.0), vsg::dvec3(0.0, 0.0, 1.0));
                perspective = vsg::Perspective::create(30.0, static_cast<double>(width) / static_cast<double>(height), 0.001 * radius, radius * 100.5);
                camera = vsg::Camera::create(perspective, lookAt, vsg::ViewportState::create(targetExtent));

                auto context = vsg::Context::create(traits->device);

                VkFormat offscreenImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
                VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

                // todo: add msaa handling
                auto transferImageView = createTransferImageView(traits->device, offscreenImageFormat, targetExtent, VK_SAMPLE_COUNT_1_BIT);
                auto transferDepthImageView = createTransferImageView(traits->device, depthFormat, targetExtent, VK_SAMPLE_COUNT_1_BIT);
                captureImage = createCaptureImage(traits->device, offscreenImageFormat, targetExtent);
                captureDepthImage = createCaptureImage(traits->device, depthFormat, targetExtent);
                auto captureCommands = createTransferCommands(traits->device,
                                                              transferImageView->image,
                                                              captureImage);
                auto captureDepthCommands = createTransferCommands(traits->device,
                                                                   transferDepthImageView->image,
                                                                   captureDepthImage);
                auto renderGraph = vsg::RenderGraph::create();
                renderGraph->framebuffer = createOffscreenFramebuffer(traits->device, transferImageView, transferDepthImageView, samples);
                renderGraph->renderArea.extent = renderGraph->framebuffer->extent2D();

                //auto renderGraph = createOffscreenRendergraph(*context, targetExtent);
                renderGraph->setClearValues(vsg::sRGB_to_linear(clearColor_), VkClearDepthStencilValue{0.0f, 0});

                auto view = vsg::View::create(camera);
                view->viewDependentState = ViewDependentState::create(view);
                renderGraph->addChild(view);
                view->addChild(gm->rootNode);

                auto commandGraph = vsg::CommandGraph::create(*(shared->window));
                commandGraph->submitOrder = -1; // render before the main_commandGraph
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
                if (!window)
                {
                    LOG_ERROR("GraphicsWidget::initialize Could not create window.");
                    return;
                }
                // if this is the first window to be created, use its device for future window creation.
                if (!traits->device) traits->device = window->windowAdapter->getOrCreateDevice();
                uint32_t width = window->traits->width;
                uint32_t height = window->traits->height;
                fprintf(stderr, "-------- with: %u\theight: %u\n", width, height);

                // todo: move camera handling to GraphicsCamera implementation
                double radius = 1.0;
                lookAt = vsg::LookAt::create(vsg::dvec3(radius * 2.0, 0.0, 0.0), vsg::dvec3(0.0, 0.0, 0.0), vsg::dvec3(0.0, 0.0, 1.0));
                perspective = vsg::Perspective::create(30.0, static_cast<double>(width) / static_cast<double>(height), 0.001 * radius, radius * 100.5);
                auto camera = vsg::Camera::create(perspective, lookAt, vsg::ViewportState::create(VkExtent2D{width, height}));

                auto trackball = vsg::Trackball::create(camera);
                trackball->addWindow(*window);
                gm->viewer->addEventHandler(trackball);

                auto view = vsg::View::create(camera);
                // try to override view dependent state implementation
                view->viewDependentState = ViewDependentState::create(view);
                view->addChild(gm->rootNode);

                // set up the render graph
                auto renderGraph = vsg::RenderGraph::create(*window, view);
                renderGraph->contents = VK_SUBPASS_CONTENTS_INLINE;

                renderGraph->setClearValues(vsg::sRGB_to_linear(clearColor_));
                auto commandGraph = vsg::CommandGraph::create(*window, renderGraph);
                //auto commandGraph = vsg::createCommandGraphForView(*window, camera, gm->rootNode);

                //viewer->addRecordAndSubmitTaskAndPresentation({commandGraph});
                // todo: second window replaces first one at the moment
                gm->viewer->addRecordAndSubmitTaskAndPresentation({commandGraph});
                container = QWidget::createWindowContainer(window, nullptr);
                container->setGeometry(window->traits->x, window->traits->y, window->traits->width, window->traits->height);
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
            LOG_ERROR("GraphicsWidget: getCameraInterface no implemented yet!");
            return NULL;
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
                if(width != captureImage->extent.width)
                {
                    LOG_ERROR("GraphcisWidget::getImageData width doesn't fit to image of framebuffer");
                    return;
                }
                if(height != captureImage->extent.height)
                {
                    LOG_ERROR("GraphcisWidget::getImageData heigt doesn't fit to image of framebuffer");
                    return;
                }
                auto imageData = ::getImageData(gm->viewer, traits->device, captureImage);
                memcpy(buffer, imageData->dataPointer(), width*height*4);

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
                if(width != captureDepthImage->extent.width)
                {
                    LOG_ERROR("GraphcisWidget::getRTTDepthData width doesn't fit to image of framebuffer");
                    return;
                }
                if(height != captureDepthImage->extent.height)
                {
                    LOG_ERROR("GraphcisWidget::getRTTDepthData heigt doesn't fit to image of framebuffer");
                    return;
                }
                double fovy, aspectRatio, Zn, Zf;
                fovy = perspective->fieldOfViewY;
                aspectRatio = perspective->aspectRatio;
                Zn  = perspective->nearDistance;
                Zf  = perspective->farDistance;
                auto imageData = ::getImageData(gm->viewer, traits->device, captureDepthImage);
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
                        const double di2 = (float)(Zn*Zf/(Zf-dv*(Zf-Zn)));
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
                auto imageData = ::getImageData(gm->viewer, traits->device, captureImage);
                vsg::write(imageData, "color_image.vsgt");
                imageData = ::getImageData(gm->viewer, traits->device, captureDepthImage);
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

    } // end of namespace vsg_graphics
} // end of namespace mars
