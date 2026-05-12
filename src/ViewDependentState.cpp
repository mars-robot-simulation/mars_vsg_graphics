#include "ViewDependentState.hpp"

namespace mars
{
    namespace vsg_graphics
    {

        ViewDependentState::ViewDependentState(vsg::View* in_view) :
            Inherit(in_view)
        {
            //fprintf(stdout, "create own view dependent state .................................... \n");
        }

        void ViewDependentState::init(vsg::ResourceRequirements& requirements)
        {
            if (lightData) return;

            worldTransformUniform = WorldTransformUniformValue::create();
            worldTransformUniform->setValue("name", "worldTransform");
            worldTransformUniform->properties.dataVariance = vsg::DataVariance::DYNAMIC_DATA_TRANSFER_AFTER_RECORD;

            vsg::ViewDependentState::init(requirements);

            unsigned int binding = descriptorSetLayout->bindings.size();
            auto worldTransformUniformDescriptor = vsg::DescriptorBuffer::create(worldTransformUniform, binding, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            descriptorSetLayout->bindings.push_back({binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
            descriptorSet->descriptors.push_back(worldTransformUniformDescriptor);
        }

        void ViewDependentState::traverse(vsg::RecordTraversal& rt) const
        {
            worldTransformUniform->value().projInverse = view->camera->projectionMatrix->inverse();
            worldTransformUniform->value().viewInverse = view->camera->viewMatrix->inverse();
            VkViewport& viewport = view->camera->viewportState->viewports[0];
            worldTransformUniform->value().viewport = vsg::vec4(viewport.x, viewport.y, viewport.width, viewport.height);
            worldTransformUniform->dirty();
            vsg::ViewDependentState::traverse(rt);
        }
    }
}
