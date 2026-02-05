#include "GraphicsCamera.hpp"
#include <mars_interfaces/Logging.hpp>

namespace mars
{
    namespace vsg_graphics
    {

        GraphicsCamera::GraphicsCamera(int width, int height )
        {
            VkExtent2D targetExtent{(unsigned int)width, (unsigned int)height};
            double radius = 1.0;
            lookAt = vsg::LookAt::create(vsg::dvec3(radius * 2.0, 0.0, 0.0), vsg::dvec3(0.0, 0.0, 0.0), vsg::dvec3(0.0, 0.0, 1.0));
            perspective = vsg::Perspective::create(30.0, static_cast<double>(width) / static_cast<double>(height), 0.001 * radius, radius * 100.5);
            camera = vsg::Camera::create(perspective, lookAt, vsg::ViewportState::create(targetExtent));
        }

        GraphicsCamera::~GraphicsCamera(void)
        {}

        void GraphicsCamera::setFrustum(double left, double right,
                                        double bottom, double top,
                                        double near, double far)
        {
            perspective->nearDistance = near;
            perspective->farDistance = far;
            double v = right / near;
            v = atan(v)*2.0;
            v = (v/M_PI)*180.0;
            perspective->fieldOfViewY = v;
            perspective->aspectRatio = right / top;
        }

        void GraphicsCamera::setFrustumFromRad(double horizontalOpeningAngle,
                                               double verticalOpeningAngle,
                                               double near, double far)
        {
            assert((near > 0) && (far > 0) && (far > near));
            double right = tan(horizontalOpeningAngle/2.0) * near;
            double left = -right;
            double top = tan(verticalOpeningAngle/2.0) * near;
            double bottom = -top;
            setFrustum(left, right, bottom, top, near, far);
        }

        void GraphicsCamera::getFrustum(std::vector<double>& frustum)
        {
            LOG_ERROR("GraphicsCamera::getFrustum not yet implemented!");
        }

        void GraphicsCamera::updateViewport(double rx, double ry, double tx,
                                            double ty, double tz, double rz, bool remember)
        {
            LOG_ERROR("GraphicsCamera::updateViewport not yet implemented!");
        }

        void GraphicsCamera::updateViewportQuat(double tx, double ty, double tz,
                                                double rx, double ry, double rz,
                                                double rw)
        {
            utils::Vector p(tx, ty, tz);
            utils::Vector v;
            utils::Quaternion q(rw, rx, ry, rz);
            v = p+q*utils::Vector(0.0, 0.0, -1.0);
            lookAt->eye = vsg::dvec3(p.x(), p.y(), p.z()); // position of the camera
            lookAt->center = vsg::dvec3(v.x(), v.y(), v.z());
            v = q*utils::Vector(0.0, 1.0, 0.0);
            lookAt->up = vsg::dvec3(v.x(), v.y(), v.z());
        }

        void GraphicsCamera::lookAtIso(double x, double y, double z)
        {
            LOG_ERROR("GraphicsCamera::lookAtIso not yet implemented!");
        }

        void GraphicsCamera::getViewport(double *rx, double *ry, double *tx,
                                         double *ty, double *tz, double *rz)
        {
            LOG_ERROR("GraphicsCamera::getViewport not yet implemented!");
        }

        void GraphicsCamera::getViewportQuat(double *tx, double *ty, double *tz,
                                             double *rx, double *ry, double *rz,
                                             double *rw)
        {
            LOG_ERROR("GraphicsCamera::getViewportQuat not yet implemented!");
        }

        void GraphicsCamera::setEyeSep(double value)
        {
            LOG_ERROR("GraphicsCamera::setEyeSep not yet implemented!");
        }

        double GraphicsCamera::getEyeSep(void) const
        {
            LOG_ERROR("GraphicsCamera::getEyeSep not yet implemented!");
            return 0;
        }

        /**\brief sets the camera type*/
        void GraphicsCamera::setCamera(int type)
        {
            LOG_ERROR("GraphicsCamera::setCamera not yet implemented!");
        }

        int GraphicsCamera::getCameraType(void) const
        {
            LOG_ERROR("GraphicsCamera::getCameraType not yet implemented!");
            return 0;
        }

        int GraphicsCamera::getCamera(void) const
        {
            LOG_ERROR("GraphicsCamera::getCamera not yet implemented!");
            return 0;
        }

        /**\brief sets the camera view */
        utils::Vector GraphicsCamera::getCameraPosition()
        {
            LOG_ERROR("GraphicsCamera::getCameraPosition not yet implemented!");
            return utils::Vector();
        }

        /* returns vector with current camera position */
        void GraphicsCamera::setCameraView(interfaces::cameraStruct cs)
        {
            LOG_ERROR("GraphicsCamera::setCameraView not yet implemented!");
        }

        /**\brief returns the cameraStruct */
        void GraphicsCamera::getCameraInfo(interfaces::cameraStruct *s)
        {
            LOG_ERROR("GraphicsCamera::getCameraInfo not yet implemented!");
        }

        void GraphicsCamera::update(void)
        {
            LOG_ERROR("GraphicsCamera::update not yet implemented!");
        }

        void GraphicsCamera::setViewport(int x, int y, int width, int height)
        {
            LOG_ERROR("GraphicsCamera::setViewport not yet implemented!");
        }

        void GraphicsCamera::eventStartPos(int x, int y)
        {
            LOG_ERROR("GraphicsCamera::eventStartPos not yet implemented!");
        }

        void GraphicsCamera::mouseDrag(int button, unsigned int modkey, int x, int y)
        {
            LOG_ERROR("GraphicsCamera::mouseDrag not yet implemented!");
        }
  
        //keyboard control functions
        /**sets the camera motion state */
        void GraphicsCamera::move(bool isMoving, Direction dir)
        {
            LOG_ERROR("GraphicsCamera::move not yet implemented!");
        }
  
        /**moves camera up and donwn in iso mode*/
        void GraphicsCamera::zoom(float speed, int x, int y, unsigned int modkey)
        {
            LOG_ERROR("GraphicsCamera::zoom not yet implemented!");
        }

        void GraphicsCamera::scrollX(float speed, int x, int y, unsigned int modkey)
        {
            LOG_ERROR("GraphicsCamera::scrollX not yet implemented!");
        }
  

        //protected slots:
        /**\brief set camera type by context menu */
        void GraphicsCamera::changeCameraTypeToPerspective()
        {
            LOG_ERROR("GraphicsCamera::changeCameraTypeToPerspective not yet implemented!");
        }

        void GraphicsCamera::changeCameraTypeToOrtho()
        {
            LOG_ERROR("GraphicsCamera::changeCameraTypeToOrtho not yet implemented!");
        }

        void GraphicsCamera::setOrthoH(double v)
        {
            LOG_ERROR("GraphicsCamera::setOrthoH not yet implemented!");
        }

        void GraphicsCamera::openSetCamViewport()
        {
            LOG_ERROR("GraphicsCamera::openSetCamViewport not yet implemented!");
        }

        void GraphicsCamera::context_setCamPredefLeft()
        {
            LOG_ERROR("GraphicsCamera::context_setCamPredefLeft not yet implemented!");
        }

        void GraphicsCamera::context_setCamPredefRight()
        {
            LOG_ERROR("GraphicsCamera::context_setCamPredefRight not yet implemented!");
        }

        void GraphicsCamera::context_setCamPredefFront()
        {
            LOG_ERROR("GraphicsCamera::context_setCamPredefFront not yet implemented!");
        }

        void GraphicsCamera::context_setCamPredefRear()
        {
            LOG_ERROR("GraphicsCamera::context_setCamPredefRear not yet implemented!");
        }

        void GraphicsCamera::context_setCamPredefTop()
        {
            LOG_ERROR("GraphicsCamera::context_setCamPredefTop not yet implemented!");
        }

        void GraphicsCamera::context_setCamPredefBottom()
        {
            LOG_ERROR("GraphicsCamera::context_setCamPredefBottom not yet implemented!");
        }

        void GraphicsCamera::setStereoMode(bool _stereo)
        {
            LOG_ERROR("GraphicsCamera::setStereoMode not yet implemented!");
        }

        void GraphicsCamera::toggleStereoMode(void)
        {
            LOG_ERROR("GraphicsCamera::toggleStereoMode not yet implemented!");
        }

        void GraphicsCamera::setFocalLength(double value)
        {
            LOG_ERROR("GraphicsCamera::setFocalLength not yet implemented!");
        }

        double GraphicsCamera::getFocalLength(void) const
        {
            LOG_ERROR("GraphicsCamera::getFocalLength not yet implemented!");
            return 0;
        }

        void GraphicsCamera::deactivateCam()
        {
            LOG_ERROR("GraphicsCamera::deactivateCam not yet implemented!");
        }

        void GraphicsCamera::activateCam()
        {
            LOG_ERROR("GraphicsCamera::activateCam not yet implemented!");
        }

        void GraphicsCamera::setPivot(int x, int y)
        {
            LOG_ERROR("GraphicsCamera::setPivot not yet implemented!");
        }

        void GraphicsCamera::setPivot(utils::Vector p)
        {
            LOG_ERROR("GraphicsCamera::setPivot not yet implemented!");
        }

        void GraphicsCamera::toggleTrackball()
        {
            LOG_ERROR("GraphicsCamera::toggleTrackball not yet implemented!");
        }

        bool GraphicsCamera::isTracking()
        {
            LOG_ERROR("GraphicsCamera::isTracking not yet implemented!");
            return false;
        }

        void GraphicsCamera::setTrackingLogRotation(bool b)
        {
            LOG_ERROR("GraphicsCamera::setTrackingLogRotation not yet implemented!");
        }

        void GraphicsCamera::getOffsetQuat(double *tx, double *ty, double *tz,
                                           double *rx, double *ry, double *rz,
                                           double *rw)
        {
            LOG_ERROR("GraphicsCamera::getOffsetQuat not yet implemented!");
        }

        void GraphicsCamera::setOffsetQuat(double tx, double ty, double tz,
                                           double rx, double ry, double rz,
                                           double rw)
        {
            LOG_ERROR("GraphicsCamera::setOffsetQuat not yet implemented!");
        }

        void GraphicsCamera::setZNear(double v)
        {
            LOG_ERROR("GraphicsCamera::setZNear not yet implemented!");
        }

        void GraphicsCamera::setZFar(double v)
        {
            LOG_ERROR("GraphicsCamera::setZFar not yet implemented!");
        }

        void GraphicsCamera::setNoZCompute(bool v)
        {
            LOG_ERROR("GraphicsCamera::setNoZCompute deprecated since vsg never computes zfar automatically!");
        }

        /**moves the camera forward with positive speed and backwards with negative speed*/
        void GraphicsCamera::moveForward(float speed)
        {
            LOG_ERROR("GraphicsCamera::moveForward not yet implemented!");
        }

        /**moves te camera left with positive speed and right with negative speed */
        void GraphicsCamera::moveRight(float speed)
        {
            LOG_ERROR("GraphicsCamera::moveRight not yet implemented!");
        }

    } // end of namespace vsg_graphics
} // end of namespace mars
