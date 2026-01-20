#pragma once

#include "gui_helper_functions.hpp"

#include <mars_utils/Vector.h>
#include <mars_utils/Quaternion.h>
#include <configmaps/ConfigData.h>

#include <vsg/all.h>

namespace mars
{
    namespace vsg_graphics
    {

        class DrawObject
        {
         public:
            DrawObject();
            ~DrawObject();
            void createObject(configmaps::ConfigMap spec, vsg::ref_ptr<vsg::Group> parent_=nullptr);
            void setParent(vsg::ref_ptr<vsg::Group> parent);
            void setPosition(const utils::Vector &pos);
            void setQuaternion(const utils::Quaternion &q);
            inline const utils::Vector& getPosition()
                { return position; }
            inline const utils::Quaternion& getQuaternion()
                { return quaternion; }
            void setVisible(bool v);

        private:
            vsg::ref_ptr<vsg::MatrixTransform> poseTransform;
            vsg::ref_ptr<vsg::MatrixTransform> scaleTransform;
            vsg::ref_ptr<vsg::Node> drawObject;
            vsg::ref_ptr<vsg::Group> parent;
            vsg::ref_ptr<vsg::StateGroup> materialStateGroup;
            utils::Vector position;
            utils::Quaternion quaternion;
            bool visible;

            void applyTransform();
        };
    }
}
