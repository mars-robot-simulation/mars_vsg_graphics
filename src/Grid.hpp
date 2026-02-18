#pragma once
#include <vsg/all.h>
#include <mars_interfaces/Logging.hpp>

namespace mars
{
    namespace vsg_graphics
    {
        class Grid
        {
        public:
            static vsg::ref_ptr<vsg::Group> create();
        };
    }
}
