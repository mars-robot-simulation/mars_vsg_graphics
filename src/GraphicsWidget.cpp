#include <QWidget>

#include "GraphicsWidget.hpp"
#include "GraphicsManager.hpp"
#include "ViewDependentState.hpp"

#include <mars_utils/Color.h>

#include <iostream>
#include <string>

#define CULL_LAYER (1 << (widgetID-1))

namespace mars
{
    namespace vsg_graphics
    {

        using namespace std;
        using namespace interfaces;
        using std::cout;
        using std::cerr;
        using std::endl;

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

            if(isRTTWidget)
            {
                LOG_ERROR("GraphicsWidget create RTT window not yet implemented");
                return;
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

                auto clearColor = vsg::vec4(0.2f, 0.2f, 0.7f, 1.0f);

                auto view = vsg::View::create(camera);
                // try to override view dependent state implementation
                view->viewDependentState = ViewDependentState::create(view);
                view->addChild(gm->rootNode);

                // set up the render graph
                auto renderGraph = vsg::RenderGraph::create(*window, view);
                renderGraph->contents = VK_SUBPASS_CONTENTS_INLINE;

                renderGraph->setClearValues(vsg::sRGB_to_linear(clearColor));
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
            LOG_ERROR("GraphicsWidget: setClearColor no implemented yet!");
        }

        const mars::utils::Color& GraphicsWidget::getClearColor() const
        {
            return clearColor;
        }

        void GraphicsWidget::getImageData(char* buffer, int& width, int& height)
        {
            (void) buffer;
            (void) width;
            (void) height;
            LOG_ERROR("GraphicsWidget: getImageData no implemented yet!");
        }

        void GraphicsWidget::getImageData(void **data, int &width, int &height)
        {
            (void) data;
            (void) width;
            (void) height;
            LOG_ERROR("GraphicsWidget: getImageData no implemented yet!");
        }

        void GraphicsWidget::getRTTDepthData(float* buffer, int& width, int& height)
        {
            (void) buffer;
            (void) width;
            (void) height;
            LOG_ERROR("GraphicsWidget: getRTTDepthData no implemented yet!");
        }

        void GraphicsWidget::getRTTDepthData(float **data, int &width, int &height) {
            (void) data;
            (void) width;
            (void) height;
            LOG_ERROR("GraphicsWidget: getRTTDepthData no implemented yet!");
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

    } // end of namespace vsg_graphics
} // end of namespace mars
