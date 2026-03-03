#pragma once
#include <vsg/all.h>
#include <mars_interfaces/Logging.hpp>
#include <configmaps/ConfigData.h>

namespace mars
{
    namespace vsg_graphics
    {

        struct TextureMapping
        {
            std::string name;
            vsg::ref_ptr<vsg::Data> image;
            vsg::ref_ptr<vsg::DescriptorImage> descriptor;
            unsigned short binding = -1;
            int usage = 0;
            bool storage = false;
            bool dst = false;
            bool src = false;
        };

        class MARSStateGroup : public vsg::Inherit<vsg::StateGroup, MARSStateGroup>
        {
        public:
            explicit MARSStateGroup(configmaps::ConfigMap material);
            std::vector<vsg::ref_ptr<vsg::Commands>> captureCommands;
            std::map<std::string, vsg::ref_ptr<vsg::Image>> captureImages;
            std::map<std::string, vsg::ref_ptr<vsg::Image>> images;
            std::string name;
        };
    }
}
