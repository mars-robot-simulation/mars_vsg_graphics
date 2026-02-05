#pragma once

#include <vsg/all.h>
#include <mars_interfaces/graphics/GraphicsCameraInterface.h>

#define MY_ZNEAR    0.01
#define MY_ZFAR  1000.0

namespace mars
{
    namespace vsg_graphics
    {

        class GraphicsCamera : public interfaces::GraphicsCameraInterface
        {
            //Q_OBJECT

        public:
            // note: default values for width and height come from the prior implementation
            // don't know if anyone needs it like that (jsc)
            GraphicsCamera(int width = 720, int height = 505 );
            ~GraphicsCamera(void);

            enum Direction{FORWARD, BACKWARD, LEFT, RIGHT};
            virtual void setFrustum(double left, double right,
                                    double bottom, double top,
                                    double near, double far);
            virtual void setFrustumFromRad(double horizontalOpeningAngle,
                                           double verticalOpeningAngle,
                                           double near, double far);

            virtual void getFrustum(std::vector<double>& frustum);

            virtual void updateViewport(double rx, double ry, double tx,
                                        double ty, double tz, double rz = 0, bool remember = 0);
            virtual void updateViewportQuat(double tx, double ty, double tz,
                                            double rx, double ry, double rz,
                                            double rw);
            virtual void lookAtIso(double x, double y, double z = 0);
            virtual void getViewport(double *rx, double *ry, double *tx,
                                     double *ty, double *tz, double *rz);

            virtual void getViewportQuat(double *tx, double *ty, double *tz,
                                         double *rx, double *ry, double *rz,
                                         double *rw);

            void setEyeSep(double value);
            double getEyeSep(void) const;
            /**\brief sets the camera type*/
            virtual void setCamera(int type);
            virtual int getCameraType(void) const;
            virtual int getCamera(void) const;
            /**\brief sets the camera view */
            utils::Vector getCameraPosition();
            /* returns vector with current camera position */
            void setCameraView(interfaces::cameraStruct cs);
            /**\brief returns the cameraStruct */
            virtual void getCameraInfo(interfaces::cameraStruct *s);
            void update(void);
            void setViewport(int x, int y, int width, int height);
            void eventStartPos(int x, int y);
            void mouseDrag(int button, unsigned int modkey, int x, int y);
  
            //keyboard control functions
            /**sets the camera motion state */
            void move(bool isMoving, Direction dir);
  
            /**moves camera up and donwn in iso mode*/
            void zoom(float speed, int x=0, int y=0, unsigned int modkey=0);
            void scrollX(float speed, int x=0, int y=0, unsigned int modkey=0);
  

            //protected slots:
            /**\brief set camera type by context menu */
            virtual void changeCameraTypeToPerspective();
            virtual void changeCameraTypeToOrtho();
            virtual void setOrthoH(double v);
            void openSetCamViewport();
            void context_setCamPredefLeft();
            void context_setCamPredefRight();
            void context_setCamPredefFront();
            void context_setCamPredefRear();
            void context_setCamPredefTop();
            void context_setCamPredefBottom();
            void setStereoMode(bool _stereo);
            void toggleStereoMode(void);
            void setFocalLength(double value);
            double getFocalLength(void) const;
            void setSwitchEye(bool val) {switch_eyes = val;}
            void setLeftEye(void) {left = true;}
            void setRightEye(void) {left = false;}
            void deactivateCam();
            void activateCam();
            void setPivot(int x, int y);
            void setPivot(utils::Vector p);
            void toggleTrackball();
            virtual bool isTracking();
            virtual void setTrackingLogRotation(bool b);
            virtual void getOffsetQuat(double *tx, double *ty, double *tz,
                                       double *rx, double *ry, double *rz,
                                       double *rw);
            virtual void setOffsetQuat(double tx, double ty, double tz,
                                       double rx, double ry, double rz,
                                       double rw);
            virtual double getMoveSpeed() {return (double)moveSpeed;}
            virtual void setMoveSpeed(double s) {moveSpeed = (float)s;}

            void setZNear(double v);
            void setZFar(double v);
            void setNoZCompute(bool v);

            vsg::ref_ptr<vsg::LookAt> lookAt;
            vsg::ref_ptr<vsg::Perspective> perspective;
            vsg::ref_ptr<vsg::Camera> camera;

            
        private:
            // for vibot we need some extensions
            interfaces::local_settings *l_settings;

            bool logTrackingRotation;
            //int camera;
            int camType, previousType;
            double actOrtH;
            int width, height;
            //ODE camera control global variables
            int xpos, ypos, xrot, yrot;
            double d_xp, d_yp, d_zp, d_xr, d_yr, d_zr;
            double f_nearPlane, f_farPlane, f_aperture, f_focal;
            double f_left[2], f_right[2], f_top, f_bottom;
            double f_ratio;
            double f_win_ratio;
            double separation;
            double eyeSep; // separation of the eyes (displacement of one eye)
            float moveSpeed;
            bool stereo;
            bool switch_eyes;
            short left;
            int nodeMask;
            double scrollSpeed;
            int haveScrollEvent;
            /*flags that are set to true when corresponing key is pressed 
             * and to false when it is release*/
            bool isMovingForward, isMovingBack, isMovingLeft, isMovingRight;
  
            double isoMinHeight,isoMaxHeight;

            /**moves the camera forward with positive speed and backwards with negative speed*/
            void moveForward(float speed);
            /**moves te camera left with positive speed and right with negative speed */
            void moveRight(float speed);

        }; // end of class GraphicsCamera

    } // end of namespace vsg_graphics
} // end of namespace mars
