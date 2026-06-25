#pragma once

#ifndef Q_MOC_RUN

//#include "gui_helper_functions.h"
//#include "GraphicsCamera.h"
//#include "PostDrawCallback.h"

#include <mars_interfaces/MARSDefs.h>
#include <mars_utils/Vector.h>
#include <mars_interfaces/graphics/GraphicsWindowInterface.h>
#include <mars_interfaces/graphics/GraphicsEventInterface.h>
#include <mars_interfaces/graphics/GraphicsGuiInterface.h>
#include <mars_interfaces/Logging.hpp>

#include <vsg/all.h>
#include <vsgQt/Window.h>

namespace mars
{
    namespace vsg_graphics
    {

        class GraphicsManager;
        class GraphicsCamera;
        class GraphicsWidget;
        const unsigned int MASK_2D = 0xF0000000;

        class InteractionHandler : public vsg::Inherit<vsg::Group, InteractionHandler>
        {
        public:
            InteractionHandler() : Inherit() {}
            virtual bool haveInteraction(std::vector<const vsg::Node*> &nodePath) {return false;}
            virtual void keyPressEvent(vsg::KeyPressEvent& keyPress, bool &active) {}
            virtual bool pointerClickEvent(int x, int y) {return false;}
            virtual bool pointerReleaseEvent(int x, int y) {return false;}
            virtual bool pointerMoveEvent(int x, int y) {return false;}
            virtual void setActive(bool v) {}
        };

        class EventHandler : public vsg::Inherit<vsg::Visitor, EventHandler>
        {
        public:
            GraphicsWidget *gw;
            std::vector<InteractionHandler*> interactionHandlers;
            InteractionHandler* activeHandler;

            void apply(vsg::KeyPressEvent& keyPress) override;
            void apply(vsg::ButtonPressEvent& buttonPressEvent) override;
            void apply(vsg::ButtonReleaseEvent& ButtonReleaseEvent) override;
            void apply(vsg::MoveEvent& moveEvent) override;
        };

        /**
         * Widget with OpenGL context and event handling.
         */
        class GraphicsWidget : public interfaces::GraphicsWindowInterface
        {
        public:
            GraphicsWidget(void *parent,
                           vsg::ref_ptr<vsg::Group> scene,
                           unsigned long id, bool hasRTTWidget = 0,
                           GraphicsManager *gm=0);
            ~GraphicsWidget();

            void initialize(void* data = nullptr, GraphicsWidget* shared = nullptr,
                            int width = 0, int height = 0, bool vsync = false);

            unsigned long getID(void);

            /**\brief returns actual mouse position */
            //mars::utils::Vector getMousePos();

            // virtual void setWGeometry(int top, int left, int width, int height) {};
            // virtual void getWGeometry(int* top, int* left,
            //                           int* width, int* height) const {};
            virtual void setFullscreen(bool val, int display = 1) override;

            //osgViewer::View* getView(void);

            virtual interfaces::GraphicsCameraInterface* getCameraInterface(void) const override;
            //osg::ref_ptr<osg::Camera> getMainCamera();

            //osgViewer::GraphicsWindow* getGraphicsWindow();
            //const osgViewer::GraphicsWindow* getGraphicsWindow() const;

            //osg::Texture2D* getRTTTexture(void);
            //osg::Texture2D* getRTTDepthTexture(void);
            //osg::Image* getRTTImage(void);
            //osg::Image* getRTTDepthImage(void);

            virtual void writeRTTImages(void) override;

            std::vector<const vsg::MatrixTransform*> getPickedObjects();
            void clearSelectionVectors(void);

            virtual void addGraphicsEventHandler(interfaces::GraphicsEventInterface* _graphicsEventHandler) override;

            //virtual osgWidget::WindowManager* getOrCreateWindowManager();
            //void setHUD(HUD* theHUD);
            //void addHUDElement(HUDElement* elem);
            //void removeHUDElement(HUDElement* elem);
            virtual void switchHudElemtVis(int num_element) override;

            /**\brief sets the clear color */
            virtual void setClearColor(mars::utils::Color color) override;
            virtual const mars::utils::Color& getClearColor() const override;

            virtual void setGrabFrames(bool grab) override;
            virtual void setSaveFrames(bool grab) override;

            //virtual void* getWidget() {return nullptr;}
            //virtual void showWidget() {};

            //virtual void updateView();

            void setName(const std::string &_name)
            {
                this->name = _name;
            }

            const std::string getName() const override
            {
                return name;
            }

            /**
             * This function copies the image data in the given buffer.
             * It assumes that the buffer ist correctly initalized
             * with a char array of the size width * height * 4
             *
             * @param buffer buffer in which the image gets copied
             * @param width returns the width of the image
             * @param height returns the height of the image
             * */
            virtual void getImageData(char* buffer, int& width, int& height) override;
            virtual void getImageData(void** data, int& width, int& height) override;

            /**
             * This function copies the depth image in the given buffer.
             * It assumes that the buffer ist correctly initalized
             * with a double array of the size width * height
             *
             * @param buffer buffer in which the image gets copied
             * @param width returns the width of the image
             * @param height returns the height of the image
             * */
            virtual void getRTTDepthData(float* buffer, int& width, int& height) override;
            virtual void getRTTDepthData(float** data, int& width, int& height) override;

            osg::Group* getScene() override
            {
                LOG_ERROR("GraphicsWidget: getScene is not implemented and deprecated");
                return nullptr;
            }

            vsg::ref_ptr<vsg::Group> getVSGScene()
            {
                return scene;
            }

            void setVSGScene(vsg::ref_ptr<vsg::Group> s)
            {
                scene = s;
            }

            virtual void setScene(osg::Group* s) override
            {
                (void) s;
                LOG_ERROR("GraphicsWidget: setScene is not implemented and deprecated");
            }

            virtual void setHUDViewOffsets(double x1, double y1,
                                           double x2, double y2) override;

            virtual void setupDistortion(double factor) override;
            virtual void grabFocus() override {}
            //void unsetFocus();

            bool pick(const double x, const double y);

            vsg::ref_ptr<vsg::WindowTraits> traits;
            vsgQt::Window *window;
            QWidget *container;
            vsg::ref_ptr<vsg::View> overlayView;
            vsg::ref_ptr<vsg::CommandGraph> commandGraph;
            vsg::ref_ptr<vsg::ImageInfo> colorImageInfo, depthImageInfo;
            vsg::ref_ptr<vsg::Image> colorImage, depthImage, captureImage, captureDepthImage;
            vsg::ref_ptr<vsg::RenderGraph> renderGraph;
            vsg::ref_ptr<vsg::Group> overlayGroup;
            vsg::ref_ptr<vsg::Group> contentGroup;
            vsg::ref_ptr<vsg::LineSegmentIntersector::Intersection> intersection;
            GraphicsCamera *graphicsCamera;
            std::vector<const vsg::Node*> pickNodePath;
            vsg::ref_ptr<EventHandler> eventHandler;
            GraphicsManager* gm;

            // todo: implement setName which applies the name to the window if available
            std::string name;
            // toggles for the mouse state
            bool isMouseButtonDown, isMouseMoving;
            // last mouse position from event queue
            int mouseX, mouseY;

        protected:
            // the widget size
            int widgetWidth;
            int widgetHeight;
            int widgetX;
            int widgetY;

            // protected for osg reference counter

            //bool manageClickEvent(osgWidget::Event& event);

            // holds a single view on a scene, this view may be composed of one or more slave cameras
            //vsg::ref_ptr<osgViewer::View> view;
            // root of the scene
            vsg::ref_ptr<vsg::Group> scene;

            // toggle for render to texture
            bool isRTTWidget;

            // the OpenGL/OSG camera

            // handles some events
            std::vector<interfaces::GraphicsEventInterface *> graphicsEventHandler;

            // called post drawing
            //PostDrawCallback* postDrawCallback;

            // the widget id
            unsigned long widgetID;

            bool hasFocus;

            //void applyResize();

        private:
            utils::Color clearColor;
            // toggle for fullscreen display
            bool isFullscreen;
            //int mouseMask;
            // toggle for stereo display
            bool isStereoDisplay;
            // eye separation for stereo display
            float cameraEyeSeparation;

            // toggle for HUD display
            bool isHUDShown;


            // destination texture if isRTTWidget==true
            //osg::ref_ptr<osg::Texture2D> rttTexture;
            // destination image if isRTTWidget==true
            //osg::ref_ptr<osg::Image> rttImage;

            // destination texture if isRTTWidget==true
            //osg::ref_ptr<osg::Texture2D> rttDepthTexture;
            // destination image if isRTTWidget==true
            //osg::ref_ptr<osg::Image> rttDepthImage;

            // list of picked objects
            std::vector<const vsg::MatrixTransform*> pickedObjects;
            enum PickMode { DISABLED, STANDARD, FORCE_ADD, FORCE_REMOVE, SINGLE };
            PickMode pickmode;
            // bool brushmode;
            vsg::ref_ptr<vsg::RenderGraph> createOffscreenRendergraph(vsg::Context& context,
                                                                      const VkExtent2D& extent);

            //virtual void initialize() {};

        }; // end of class GraphicsWidget

    } // end of namespace vsg_graphics
} // end of namespace mars

#endif
