// most of the following non class code copied from vsgoffscreenshot of vsgExamples

// check if we can export into desired rgba format
bool supportsBlit(vsg::ref_ptr<vsg::Device> device, VkFormat format)
{
    auto physicalDevice = device->getPhysicalDevice();
    VkFormatProperties srcFormatProperties;
    vkGetPhysicalDeviceFormatProperties(*(physicalDevice), format, &srcFormatProperties);
    VkFormatProperties destFormatProperties;
    vkGetPhysicalDeviceFormatProperties(*(physicalDevice), VK_FORMAT_R8G8B8A8_SRGB, &destFormatProperties);
    return ((srcFormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) != 0) &&
        ((destFormatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) != 0);
}

// the capture image is the one we copy / write to
vsg::ref_ptr<vsg::Image> createCaptureImage(
    vsg::ref_ptr<vsg::Device> device,
    VkFormat sourceFormat,
    const VkExtent2D& extent)
{
    // blit to RGBA if supported
    auto targetFormat = supportsBlit(device, sourceFormat) ? VK_FORMAT_R8G8B8A8_SRGB : sourceFormat;

    // create image to write to
    auto image = vsg::Image::create();
    image->format = targetFormat;
    image->extent = {extent.width, extent.height, 1};
    image->arrayLayers = 1;
    image->mipLevels = 1;
    image->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image->samples = VK_SAMPLE_COUNT_1_BIT;
    image->tiling = VK_IMAGE_TILING_LINEAR;
    image->usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    switch (sourceFormat)
    {
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
        image->format = VK_FORMAT_R32_SFLOAT;
        break;
    default:
        break;
    }

    image->compile(device);

    auto memReqs = image->getMemoryRequirements(device->deviceID);
    auto memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    auto deviceMemory = vsg::DeviceMemory::create(device, memReqs, memFlags);
    image->bind(deviceMemory, 0);

    return image;
}

VkImageUsageFlags computeUsageFlagsForFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
        return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    default:
        return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
}

vsg::ref_ptr<vsg::ImageView> createTransferImageView(
    vsg::ref_ptr<vsg::Device> device,
    VkFormat format,
    const VkExtent2D& extent,
    VkSampleCountFlagBits samples)
{
    auto image = vsg::Image::create();
    image->format = format;
    image->extent = VkExtent3D{extent.width, extent.height, 1};
    image->mipLevels = 1;
    image->arrayLayers = 1;
    image->samples = samples;
    image->usage = computeUsageFlagsForFormat(format) | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    return vsg::createImageView(device, image, vsg::computeAspectFlagsForFormat(format));
}

vsg::ref_ptr<vsg::Commands> createTransferCommands(
    vsg::ref_ptr<vsg::Device> device,
    vsg::ref_ptr<vsg::Image> sourceImage,
    vsg::ref_ptr<vsg::Image> destinationImage)
{
    auto commands = vsg::Commands::create();

    // transition destinationImage to transfer destination initialLayout
    auto transitionDestinationImageToDestinationLayoutBarrier = vsg::ImageMemoryBarrier::create(
        0,                                                             // srcAccessMask
        VK_ACCESS_TRANSFER_WRITE_BIT,                                  // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,                                     // oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,                          // newLayout
        VK_QUEUE_FAMILY_IGNORED,                                       // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                                       // dstQueueFamilyIndex
        destinationImage,                                              // image
        VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1} // subresourceRange
        );

    auto cmd_transitionForTransferBarrier = vsg::PipelineBarrier::create(
        VK_PIPELINE_STAGE_TRANSFER_BIT,                      // srcStageMask
        VK_PIPELINE_STAGE_TRANSFER_BIT,                      // dstStageMask
        0,                                                   // dependencyFlags
        transitionDestinationImageToDestinationLayoutBarrier // barrier
        );

    commands->addChild(cmd_transitionForTransferBarrier);

    if (
        sourceImage->format == destinationImage->format && sourceImage->extent.width == destinationImage->extent.width && sourceImage->extent.height == destinationImage->extent.height)
    {
        // use vkCmdCopyImage
        VkImageCopy region{};
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.layerCount = 1;
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.layerCount = 1;
        region.extent = destinationImage->extent;

        auto copyImage = vsg::CopyImage::create();
        copyImage->srcImage = sourceImage;
        copyImage->srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        copyImage->dstImage = destinationImage;
        copyImage->dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        copyImage->regions.push_back(region);

        commands->addChild(copyImage);
    }
    else if (supportsBlit(device, destinationImage->format))
    {
        // blit using vkCmdBlitImage
        VkImageBlit region{};
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.layerCount = 1;
        region.srcOffsets[0] = VkOffset3D{0, 0, 0};
        region.srcOffsets[1] = VkOffset3D{
            static_cast<int32_t>(sourceImage->extent.width),
            static_cast<int32_t>(sourceImage->extent.height),
            1};
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.layerCount = 1;
        region.dstOffsets[0] = VkOffset3D{0, 0, 0};
        region.dstOffsets[1] = VkOffset3D{
            static_cast<int32_t>(destinationImage->extent.width),
            static_cast<int32_t>(destinationImage->extent.height),
            1};

        auto blitImage = vsg::BlitImage::create();
        blitImage->srcImage = sourceImage;
        blitImage->srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        blitImage->dstImage = destinationImage;
        blitImage->dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        blitImage->regions.push_back(region);
        blitImage->filter = VK_FILTER_NEAREST;

        commands->addChild(blitImage);
    }
    else
    {
        /// If the source and target extents and/or format are different
        /// we would need to blit, however this device does not support it.
        /// Options at this point include resizing or reformatting on the
        /// CPU (using STBI for example) or using a sampler and rendering to
        /// a texture in an additional pass.
        throw std::runtime_error{"GPU does not support blit."};
    }

    // transition destination image from transfer destination layout
    // to general layout to enable mapping to image DeviceMemory
    auto transitionDestinationImageToMemoryReadBarrier = vsg::ImageMemoryBarrier::create(
        VK_ACCESS_TRANSFER_WRITE_BIT,                                  // srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,                                     // dstAccessMask
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,                          // oldLayout
        VK_IMAGE_LAYOUT_GENERAL,                                       // newLayout
        VK_QUEUE_FAMILY_IGNORED,                                       // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                                       // dstQueueFamilyIndex
        destinationImage,                                              // image
        VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1} // subresourceRange
        );

    auto cmd_transitionFromTransferBarrier = vsg::PipelineBarrier::create(
        VK_PIPELINE_STAGE_TRANSFER_BIT,               // srcStageMask
        VK_PIPELINE_STAGE_TRANSFER_BIT,               // dstStageMask
        0,                                            // dependencyFlags
        transitionDestinationImageToMemoryReadBarrier // barrier
        );

    commands->addChild(cmd_transitionFromTransferBarrier);

    return commands;
}

vsg::ref_ptr<vsg::RenderPass> createTransferRenderPass(
    vsg::ref_ptr<vsg::Device> device,
    VkFormat imageFormat,
    VkFormat depthFormat,
    bool requiresDepthRead)
{
    auto colorAttachment = vsg::defaultColorAttachment(imageFormat);
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; // <-- difference from vsg::createRenderPass()

    auto depthAttachment = vsg::defaultDepthAttachment(depthFormat);

    if (requiresDepthRead)
    {
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    }

    vsg::RenderPass::Attachments attachments{colorAttachment, depthAttachment};

    vsg::AttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    vsg::AttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    vsg::SubpassDescription subpass = {};
    subpass.colorAttachments.emplace_back(colorAttachmentRef);
    subpass.depthStencilAttachments.emplace_back(depthAttachmentRef);

    vsg::RenderPass::Subpasses subpasses{subpass};

    // image layout transition
    vsg::SubpassDependency colorDependency = {};
    colorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    colorDependency.dstSubpass = 0;
    colorDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    colorDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    colorDependency.srcAccessMask = 0;
    colorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    colorDependency.dependencyFlags = 0;

    // depth buffer is shared between swap chain images
    vsg::SubpassDependency depthDependency = {};
    depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    depthDependency.dstSubpass = 0;
    depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    depthDependency.dependencyFlags = 0;

    vsg::RenderPass::Dependencies dependencies{colorDependency, depthDependency};

    return vsg::RenderPass::create(device, attachments, subpasses, dependencies);
}

vsg::ref_ptr<vsg::RenderPass> createTransferRenderPass(
    vsg::ref_ptr<vsg::Device> device,
    VkFormat imageFormat,
    VkFormat depthFormat,
    VkSampleCountFlagBits samples,
    bool requiresDepthRead)
{
    vsg::ref_ptr<vsg::RenderPass> renderPass;

    if (samples == VK_SAMPLE_COUNT_1_BIT)
    {
        return createTransferRenderPass(device, imageFormat, depthFormat, requiresDepthRead);
    }

    // First attachment is multisampled target.
    vsg::AttachmentDescription colorAttachment = {};
    colorAttachment.format = imageFormat;
    colorAttachment.samples = samples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Second attachment is the resolved image which will be presented.
    vsg::AttachmentDescription resolveAttachment = {};
    resolveAttachment.format = imageFormat;
    resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; // <-- difference from vsg::createMultisampledRenderPass()

    // multisampled depth attachment. Resolved if requiresDepthRead is true.
    vsg::AttachmentDescription depthAttachment = {};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = samples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    vsg::RenderPass::Attachments attachments{colorAttachment, resolveAttachment, depthAttachment};

    if (requiresDepthRead)
    {
        vsg::AttachmentDescription depthResolveAttachment = {};
        depthResolveAttachment.format = depthFormat;
        depthResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthResolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthResolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthResolveAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.push_back(depthResolveAttachment);
    }

    vsg::AttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    vsg::AttachmentReference resolveAttachmentRef = {};
    resolveAttachmentRef.attachment = 1;
    resolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    vsg::AttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 2;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    vsg::SubpassDescription subpass;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachments.emplace_back(colorAttachmentRef);
    subpass.resolveAttachments.emplace_back(resolveAttachmentRef);
    subpass.depthStencilAttachments.emplace_back(depthAttachmentRef);

    if (requiresDepthRead)
    {
        vsg::AttachmentReference depthResolveAttachmentRef = {};
        depthResolveAttachmentRef.attachment = 3;
        depthResolveAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        subpass.depthResolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
        subpass.stencilResolveMode = VK_RESOLVE_MODE_NONE;
        subpass.depthStencilResolveAttachments.emplace_back(depthResolveAttachmentRef);
    }

    vsg::RenderPass::Subpasses subpasses{subpass};

    vsg::SubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    vsg::SubpassDependency dependency2 = {};
    dependency2.srcSubpass = 0;
    dependency2.dstSubpass = VK_SUBPASS_EXTERNAL;
    dependency2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency2.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency2.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependency2.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependency2.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    vsg::RenderPass::Dependencies dependencies{dependency, dependency2};

    return vsg::RenderPass::create(device, attachments, subpasses, dependencies);
}

vsg::ref_ptr<vsg::Framebuffer> createOffscreenFramebuffer(
    vsg::ref_ptr<vsg::Device> device,
    vsg::ref_ptr<vsg::ImageView> transferImageView,
    vsg::ref_ptr<vsg::ImageView> transferDepthImageView,
    VkSampleCountFlagBits const samples)
{
    VkFormat imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    bool requiresDepthRead = true;

    VkExtent2D const extent{
        transferImageView->image->extent.width,
        transferImageView->image->extent.height};

    vsg::ImageViews imageViews;
    if (samples == VK_SAMPLE_COUNT_1_BIT)
    {
        imageViews.emplace_back(transferImageView);
        imageViews.emplace_back(transferDepthImageView);
    }
    else
    {
        // MSAA
        imageViews.emplace_back(createTransferImageView(device, imageFormat, extent, samples));
        imageViews.emplace_back(transferImageView);
        imageViews.emplace_back(createTransferImageView(device, depthFormat, extent, samples));
        if (requiresDepthRead)
        {
            imageViews.emplace_back(createTransferImageView(device, depthFormat, extent, VK_SAMPLE_COUNT_1_BIT));
        }
    }

    auto renderPass = createTransferRenderPass(device, imageFormat, depthFormat, samples, requiresDepthRead);
    auto framebuffer = vsg::Framebuffer::create(renderPass, imageViews, extent.width, extent.height, 1);

    return framebuffer;
}
        

vsg::ref_ptr<vsg::Data> getImageData(vsg::ref_ptr<vsg::Viewer> viewer, vsg::ref_ptr<vsg::Device> device, vsg::ref_ptr<vsg::Image> captureImage)
{
    constexpr uint64_t waitTimeout = 1000000000; // 1 second
    viewer->waitForFences(0, waitTimeout);

    VkImageSubresource subResource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
    VkSubresourceLayout subResourceLayout;
    vkGetImageSubresourceLayout(*device, captureImage->vk(device->deviceID), &subResource, &subResourceLayout);

    auto deviceMemory = captureImage->getDeviceMemory(device->deviceID);

    size_t destRowWidth = captureImage->extent.width * sizeof(vsg::ubvec4);
    vsg::ref_ptr<vsg::Data> imageData;
    if (destRowWidth == subResourceLayout.rowPitch)
    {
        /// Map the buffer memory and assign as a vec4Array2D that will automatically
        /// unmap itself on destruction.
        imageData = vsg::MappedData<vsg::ubvec4Array2D>::create(
            deviceMemory,
            subResourceLayout.offset,
            0,
            vsg::Data::Properties{captureImage->format},
            captureImage->extent.width,
            captureImage->extent.height);
    }
    else
    {
        /// Map the buffer memory and assign as a ubyteArray that will automatically
        /// unmap itself on destruction. A ubyteArray is used as the graphics buffer
        /// memory is not contiguous like vsg::Array2D, so map to a flat buffer first
        /// then copy to Array2D.
        auto mappedData = vsg::MappedData<vsg::ubyteArray>::create(
            deviceMemory,
            subResourceLayout.offset,
            0,
            vsg::Data::Properties{captureImage->format},
            subResourceLayout.rowPitch * captureImage->extent.height);
        imageData = vsg::ubvec4Array2D::create(captureImage->extent.width, captureImage->extent.height, vsg::Data::Properties{captureImage->format});
        for (uint32_t row = 0; row < captureImage->extent.height; ++row)
        {
            std::memcpy(imageData->dataPointer(row * captureImage->extent.width), mappedData->dataPointer(row * subResourceLayout.rowPitch), destRowWidth);
        }
    }
    return imageData;
}
