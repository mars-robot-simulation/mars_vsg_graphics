#include <vsgvr/xr/Instance.h>
#include <vsgvr/xr/GraphicsBindingVulkan.h>
#include <vsgvr/xr/ViewMatrix.h>

#include <vsgvr/app/Viewer.h>
#include <vsgvr/app/CompositionLayerProjection.h>
#include <vsgvr/actions/ActionSet.h>
#include <vsgvr/actions/ActionPoseBinding.h>
#include <vsgvr/app/UserOrigin.h>

// most of the following non class code copied from vsgvs generic example

XrViewConfigurationType selectViewConfigurationType(vsg::ref_ptr<vsgvr::Instance> instance);

XrEnvironmentBlendMode selectEnvironmentBlendMode(vsg::ref_ptr<vsgvr::Instance> instance, XrViewConfigurationType viewConfigurationType);

void configureXrVulkanRequirements(vsg::ref_ptr<vsg::WindowTraits> windowTraits, vsgvr::VulkanRequirements& xrVulkanReqs);

VkFormat selectSwapchainFormat(vsg::ref_ptr<vsgvr::Session> vrSession);

uint32_t selectSwapchainSampleCount(vsg::ref_ptr<vsgvr::Session> vrSession, VkSampleCountFlags samples);

XrReferenceSpaceType selectReferenceSpaceType(vsg::ref_ptr<vsgvr::Session> vrSession);
