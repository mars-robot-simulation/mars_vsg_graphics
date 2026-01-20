#pragma once
#include <vsg/all.h>
#include <mars_interfaces/Logging.hpp>
#include <configmaps/ConfigData.h>

namespace mars
{
    namespace vsg_graphics
    {
        class MARSStateGroup
        {
        public:
            static vsg::ref_ptr<vsg::StateGroup> create(configmaps::ConfigMap material);
        };
    }
}
