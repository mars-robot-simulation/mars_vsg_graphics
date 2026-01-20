#pragma once
#include <vsg/all.h>
#include <mars_interfaces/Logging.hpp>

namespace mars
{
    namespace vsg_graphics
    {
        class Bobj
        {
        public:
            static vsg::ref_ptr<vsg::Node> readFromFile(const std::string &filename);
            static bool checkBobj(std::string &filename);
        };
    }
}
