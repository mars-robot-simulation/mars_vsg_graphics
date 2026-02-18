#include "Grid.hpp"
#include "gui_helper_functions.hpp"

namespace mars
{
    namespace vsg_graphics
    {

        const auto grid_vert = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstants {
    mat4 projection;
    mat4 modelView;
} pc;

layout(set = 1, binding = 5) uniform WorldTransform{
    mat4 projectionInverse;
    mat4 viewInverse;
} wt;

layout(location = 0) in vec3 vsg_Vertex;

layout(location = 0) out vec3 nearPoint;
layout(location = 1) out vec3 farPoint;
layout(location = 2) out mat4 view;
layout(location = 6) out mat4 projection;

out gl_PerVertex{
 vec4 gl_Position;
};

// todo: worldTransform is not correct

vec3 unprojectPoint(float x, float y, float z, float w, mat4 projectionInverse, mat4 viewInverse) {
    vec4 clipSpacePos = vec4(x, y, z, w);
    vec4 viewPos =  projectionInverse * clipSpacePos;
    viewPos = vec4(viewPos.xyz / viewPos.w, 1);
    vec4 worldPos = viewInverse * viewPos;
    return worldPos.xyz;
}

void main()
{
    mat4 projectionInverse = wt.projectionInverse;//inverse(pc.projection);
    mat4 viewInverse = wt.viewInverse;//inverse(pc.modelView);
    vec3 pos = vsg_Vertex;

    nearPoint = unprojectPoint(pos.x, pos.y, 0.99999999, 1.0, projectionInverse, viewInverse);
    farPoint = unprojectPoint(pos.x, pos.y, 0.000000001, 1.0, projectionInverse, viewInverse);
    float t = -nearPoint.z / (farPoint.z - nearPoint.z);

    // write outputs
    view = inverse(wt.viewInverse);//pc.modelView;
    projection = pc.projection;
    gl_Position = vec4(pos.x, pos.y, 1.0, 1.0);
}
)";

    const auto grid_frag = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 nearPoint;
layout(location = 1) in vec3 farPoint;
layout(location = 2) in mat4 view;
layout(location = 6) in mat4 projection;
layout(location = 0) out vec4 outColor;

vec4 grid(vec3 pos) {
    vec2 coord = pos.xy;
    vec2 derivative = fwidth(coord);
    //derivative = vec2(1.0);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    grid *= 1.2;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1);
    float minimumx = min(derivative.x, 1);
    vec4 color = vec4(0.5, 0.5, 0.5, 1.0 - min(line, 1.0));

    if(abs(pos.x) < minimumx)
    {
        color.x = 1.0;
        color.y = 0.0;
        color.z = 0.0;
    }
    if(abs(pos.y) < minimumz)
    {
        color.x = 0.0;
        color.y = 1.0;
        color.z = 0.0;
    }
    return color;
}

void main() {
     float t = -nearPoint.z / (farPoint.z - nearPoint.z);
     vec3 pos = nearPoint + t * (farPoint - nearPoint);
     float fade = (1.0-t*2.2);
     vec4 viewPos = view * vec4(pos.xyz, 1);
     vec4 clipPos = projection * viewPos;
     float depth = clipPos.z / clipPos.w;
     //pos.z = max(-viewPos.z*0.1, 1.0);

     // Display only the lower plane
     if(t < 1 && t > 0) {
         vec4 gridColor = grid(pos);
         outColor = vec4(gridColor.x, gridColor.y, gridColor.z, gridColor.w * fade);
         if(gridColor.w*fade < 0.25)
         {
           depth = 0;
         }
     }
     else
     {
         outColor = vec4(0.0, 0.0, 0.0, 0.);
         depth = 0;
     } 

     gl_FragDepth = depth;
 }
 
)";

        
        vsg::ref_ptr<vsg::Group> Grid::create()
        {
            vsg::ref_ptr<vsg::Group> node = vsg::Group::create();

            auto vertices = vsg::vec3Array::create({
                    {1.0f, 1.0f, 0.0f},
                    {-1.0f, -1.0f, 0.0f},
                    {-1.0f, 1.0f, 0.0f},
                    {-1.0f, -1.0f, 0.0f},
                    {1.0f, 1.0f, 0.0f},
                    {1.0f, -1.0f, 0.0f},
                });
            auto indices = vsg::ushortArray::create({0, 1, 2, 3, 4, 5});

            auto options = vsg::Options::create();
            options->fileCache = vsg::getEnv("VSG_FILE_CACHE");
            options->paths = vsg::getEnvPaths("VSG_FILE_PATH");

            auto vertexShader = vsg::ShaderStage::create(VK_SHADER_STAGE_VERTEX_BIT, "main", grid_vert);
            auto fragmentShader = vsg::ShaderStage::create(VK_SHADER_STAGE_FRAGMENT_BIT, "main", grid_frag);

            // no clue how te get the set numbers right for using shadersets
#define VIEW_DESCRIPTOR_SET 1
#define MATERIAL_DESCRIPTOR_SET 0

            auto shaderSet = vsg::ShaderSet::create(vsg::ShaderStages{vertexShader, fragmentShader});

            shaderSet->addAttributeBinding("vsg_Vertex", "", 0, VK_FORMAT_R32G32B32_SFLOAT, vsg::vec3Array::create(1));
            shaderSet->addDescriptorBinding("material", "", MATERIAL_DESCRIPTOR_SET, 10, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, vsg::PbrMaterialValue::create(), vsg::CoordinateSpace::LINEAR);
            shaderSet->addDescriptorBinding("lightData", "", VIEW_DESCRIPTOR_SET, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, vsg::vec4Array::create(64));
            shaderSet->addDescriptorBinding("worldTransform", "", VIEW_DESCRIPTOR_SET, 5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, WorldTransformUniformValue::create());

            shaderSet->addPushConstantRange("pc", "", VK_SHADER_STAGE_ALL, 0, 128);
            shaderSet->customDescriptorSetBindings.push_back(vsg::ViewDependentStateBinding::create(VIEW_DESCRIPTOR_SET));

            VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
            colorBlendAttachment.blendEnable = VK_TRUE;
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;

            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

            auto colorBlendState = vsg::ColorBlendState::create(vsg::ColorBlendState::ColorBlendAttachments{colorBlendAttachment});
            shaderSet->defaultGraphicsPipelineStates.push_back(colorBlendState);

            auto depthState = vsg::DepthStencilState::create();
            depthState->depthTestEnable = VK_TRUE;
            depthState->depthWriteEnable = VK_TRUE;
            depthState->depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
            shaderSet->defaultGraphicsPipelineStates.push_back(depthState);
    
            auto graphicsPipelineConfig = vsg::GraphicsPipelineConfigurator::create(shaderSet);

            vsg::DataList vertexArrays;
            graphicsPipelineConfig->assignArray(vertexArrays, "vsg_Vertex", VK_VERTEX_INPUT_RATE_VERTEX, vertices);
            graphicsPipelineConfig->enableDescriptor("material");
            graphicsPipelineConfig->enableDescriptor("worldTransform");

            auto vertexDraw = vsg::VertexIndexDraw::create();
            vertexDraw->assignArrays(vertexArrays);
            vertexDraw->assignIndices(indices);
            vertexDraw->indexCount = static_cast<uint32_t>(indices->size());
            vertexDraw->instanceCount = 1;
            vertexDraw->firstBinding = graphicsPipelineConfig->baseAttributeBinding;

            graphicsPipelineConfig->init();

            auto stateGroup = vsg::StateGroup::create();

            graphicsPipelineConfig->copyTo(stateGroup);

            stateGroup->addChild(vertexDraw);

            node->addChild(stateGroup);

            return node;
        }
    }
}
