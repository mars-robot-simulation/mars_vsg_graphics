#pragma once

#include "GraphicsWidget.hpp"
#include <mars_interfaces/graphics/ManipulatorClient.hpp>

namespace mars
{
    namespace vsg_graphics
    {
        class Manipulator :  public vsg::Inherit<InteractionHandler, Manipulator> 
        {
        public:
            Manipulator(std::string resourcesPath);
            interfaces::ManipulatorClient *eventClient;
            int mask;
            std::string name;
            int mode, axis;

            virtual bool haveInteraction(std::vector<const vsg::Node*> &nodePath) override;
            virtual void keyPressEvent(vsg::KeyPressEvent& keyPress, bool &active_);
            virtual bool pointerClickEvent(int x, int y);
            virtual bool pointerReleaseEvent(int x, int y);
            virtual bool pointerMoveEvent(int x, int y);
            virtual void setActive(bool v);
            void setPose(const utils::Vector &v,
                         const utils::Quaternion &q);
            void setScale(const utils::Vector &s);

        private:
            utils::Vector pos, scale;
            utils::Quaternion rot;
            int move;
            bool active;
            int rejectMove;

            vsg::ref_ptr<vsg::Node> x, y, z, rx, ry, rz;
            vsg::ref_ptr<vsg::MatrixTransform> poseTransform;
            vsg::ref_ptr<vsg::MatrixTransform> scaleTransform;
            vsg::ref_ptr<vsg::PbrMaterialValue> mx, my, mz, mrx, mry, mrz;
            void applyReject();
            void setColors();
            void setColor(vsg::PbrMaterialValue *v, int c);
        };
    }
}
