#pragma once
#include <vsg/all.h>
#include <mars_interfaces/Logging.hpp>

namespace mars
{
    namespace vsg_graphics
    {
        bool supportsBlit(VkFormat format);
        vsg::ref_ptr<vsg::Image> createCaptureImage(VkFormat sourceFormat,
                                                    const VkExtent2D& extent);
        VkImageUsageFlags computeUsageFlagsForFormat(VkFormat format);
        vsg::ref_ptr<vsg::ImageView> createTransferImageView(VkFormat format,
                                                             const VkExtent2D& extent,
                                                             VkSampleCountFlagBits samples);
        vsg::ref_ptr<vsg::Commands> createTransferCommands(vsg::ref_ptr<vsg::Image> sourceImage,
                                                           vsg::ref_ptr<vsg::Image> destinationImage);
        vsg::ref_ptr<vsg::Commands> createTransferCommandsI(vsg::ref_ptr<vsg::Image> sourceImage,
                                                            vsg::ref_ptr<vsg::Image> destinationImage);
        vsg::ref_ptr<vsg::RenderPass> createTransferRenderPass(VkFormat imageFormat,
                                                               VkFormat depthFormat,
                                                               bool requiresDepthRead);
        vsg::ref_ptr<vsg::RenderPass> createTransferRenderPass(VkFormat imageFormat,
                                                               VkFormat depthFormat,
                                                               VkSampleCountFlagBits samples,
                                                               bool requiresDepthRead);
        vsg::ref_ptr<vsg::Framebuffer> createOffscreenFramebuffer(
            vsg::ref_ptr<vsg::ImageView> transferImageView,
            vsg::ref_ptr<vsg::ImageView> transferDepthImageView,
            VkSampleCountFlagBits const samples);
        vsg::ref_ptr<vsg::Data> getImageData(vsg::ref_ptr<vsg::Viewer> viewer,
                                             vsg::ref_ptr<vsg::Image> captureImage);

    }
}
