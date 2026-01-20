#include "MARSStateGroup.hpp"
#include "gui_helper_functions.hpp"
#include <vsg/all.h>
#include <mars_interfaces/Logging.hpp>
#include <configmaps/ConfigData.h>
#include <mars_utils/misc.h>

namespace mars
{
    namespace vsg_graphics
    {
        vsg::ref_ptr<vsg::StateGroup> MARSStateGroup::create(configmaps::ConfigMap materialSpec)
        {

            // create material info for shader
            vsg::PbrMaterial material;
            material.baseColorFactor[0] = (double)materialSpec["ambientColor"]["r"];
            material.baseColorFactor[1] = (double)materialSpec["ambientColor"]["g"];
            material.baseColorFactor[2] = (double)materialSpec["ambientColor"]["b"];
            material.baseColorFactor[3] = (double)materialSpec["ambientColor"]["a"];
            material.diffuseFactor[0] = (double)materialSpec["diffuseColor"]["r"];
            material.diffuseFactor[1] = (double)materialSpec["diffuseColor"]["g"];
            material.diffuseFactor[2] = (double)materialSpec["diffuseColor"]["b"];
            material.diffuseFactor[3] = (double)materialSpec["diffuseColor"]["a"];
            material.specularFactor[0] = (double)materialSpec["specularColor"]["r"];
            material.specularFactor[1] = (double)materialSpec["specularColor"]["g"];
            material.specularFactor[2] = (double)materialSpec["specularColor"]["b"];
            material.specularFactor[3] = (double)materialSpec["specularColor"]["a"];

            material.roughnessFactor = 10.0/(double)materialSpec["shininess"];
            if(material.roughnessFactor > 1.0) material.roughnessFactor = 1.0;

            material.metallicFactor = material.roughnessFactor;

            // load shaders
            std::string vertexShaderFile = utils::pathJoin(GuiHelper::resourcePath, "resources/graph_shader/default_vertex_shader.yml");
            std::string fragmentShaderFile = utils::pathJoin(GuiHelper::resourcePath, "resources/graph_shader/default_fragment_shader.yml");
            GraphShader &vs = GuiHelper::readGraphShaderFromFile(vertexShaderFile);
            GraphShader &fs = GuiHelper::readGraphShaderFromFile(fragmentShaderFile);
            // copy varyings from vertex shader to fragment shader
            fs.varyings = vs.varyings;

            // for testing we try to load shaders from working dir
            auto vertexShader = vsg::ShaderStage::create(VK_SHADER_STAGE_VERTEX_BIT, "main", vs.generateVertexShaderSource());
            auto fragmentShader = vsg::ShaderStage::create(VK_SHADER_STAGE_FRAGMENT_BIT, "main", fs.generateFragmentShaderSource());
            if (!vertexShader || !fragmentShader)
            {
                std::cout << "Could not create shaders." << std::endl;
            }
            const vsg::ShaderStages shaders{vertexShader, fragmentShader};

            vsg::DescriptorSetLayoutBindings descriptorBindings{
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},            // { binding, descriptorType, descriptorCount, stageFlags, pImmutableSamplers}
                //{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
            };
            auto descriptorSetLayout = vsg::DescriptorSetLayout::create(descriptorBindings);

            // todo: howto deal with global uniforms
            auto worldTransformUniformDescriptor = vsg::DescriptorBuffer::create(GuiHelper::worldTransformUniform, 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            auto materialUniformDescriptor = vsg::DescriptorBuffer::create(vsg::PbrMaterialValue::create(material), 1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            auto descriptorSet = vsg::DescriptorSet::create(descriptorSetLayout, vsg::Descriptors{worldTransformUniformDescriptor, materialUniformDescriptor});


            vsg::PushConstantRanges pushConstantRanges{
                {VK_SHADER_STAGE_VERTEX_BIT, 0, 128} // projection, view, and model matrices, actual push constant calls automatically provided by the VSG's RecordTraversal
            };

            vsg::VertexInputState::Bindings vertexBindingsDescriptions{
                VkVertexInputBindingDescription{0, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX},
                VkVertexInputBindingDescription{1, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX}};

            vsg::VertexInputState::Attributes vertexAttributeDescriptions{
                VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
                VkVertexInputAttributeDescription{1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0}};

            auto rasterState = vsg::RasterizationState::create();
            rasterState->cullMode = VK_CULL_MODE_NONE;//VK_CULL_MODE_BACK_BIT;

            auto depthState = vsg::DepthStencilState::create();
            depthState->depthTestEnable = VK_TRUE;
            depthState->depthWriteEnable = VK_TRUE;
            depthState->depthCompareOp = VK_COMPARE_OP_GREATER;

            vsg::GraphicsPipelineStates pipelineStates{
                vsg::VertexInputState::create(vertexBindingsDescriptions, vertexAttributeDescriptions),
                vsg::InputAssemblyState::create(),
                rasterState,
                vsg::MultisampleState::create(),
                vsg::ColorBlendState::create(),
                depthState};

            auto viewDescriptorSetLayout = vsg::ViewDescriptorSetLayout::create();
            auto pipelineLayout = vsg::PipelineLayout::create(vsg::DescriptorSetLayouts{viewDescriptorSetLayout, descriptorSetLayout}, pushConstantRanges);
            auto pipeline = vsg::GraphicsPipeline::create(pipelineLayout, shaders, pipelineStates);
            auto bindDescriptorSets = vsg::BindDescriptorSets::create(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, vsg::DescriptorSets{descriptorSet});
            auto bindGraphicsPipeline = vsg::BindGraphicsPipeline::create(pipeline);


            // bind light data
            vsg::DescriptorSetLayoutBindings viewDescriptorBindings{
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}, // binding for light data
            };
            //auto viewLayout = vsg::PipelineLayout::create(vsg::DescriptorSetLayouts{viewDescriptorSetLayout}, pushConstantRanges);
            uint32_t vds_set = 1;

            auto root = vsg::StateGroup::create();
            root->add(bindGraphicsPipeline);
            root->add(bindDescriptorSets); // descriptor set is used for textures and uniforms etc
            root->add(vsg::BindViewDescriptorSets::create(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, vds_set));
            return root;
        }
    }
}
