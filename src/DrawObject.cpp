#include "DrawObject.hpp"
#include "gui_helper_functions.hpp"


const auto skybox_vert = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstants {
    mat4 projection;
    mat4 modelView;
} pc;

layout(binding = 0) uniform mat4 ;
layout(location = 0) in vec3 vsg_Vertex;
layout(location = 0) out vec3 UVW;

out gl_PerVertex{ vec4 gl_Position; };

void main()
{
    UVW = vsg_Vertex;

    // Remove translation
    mat4 modelView = pc.modelView;
    //rmodelView[3] = vec4(0.0, 0.0, 0.0, 1.0);

    vec4 pos = pc.projection * modelView * vec4(vsg_Vertex, 1.0);
    gl_Position = pos;
    //UVW = pos.xyz;
}
)";

const auto skybox_frag = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout(binding = 0) uniform samplerCube envMap;
layout(location = 0) in vec3 UVW;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(UVW.z, 0.0, 0.0, 1.0);
}
)";


namespace mars
{
    namespace vsg_graphics
    {

        DrawObject::DrawObject() : visible(true)
        {
            poseTransform = vsg::MatrixTransform::create(vsg::translate(vsg::dvec3(0, 0, 0)));
        }

        DrawObject::~DrawObject()
        {

        }

        void DrawObject::createObject(configmaps::ConfigMap spec, vsg::ref_ptr<vsg::Group> parent, vsg::ref_ptr<WorldTransformUniformValue> worldTransformUniform)
        {
            if(parent) setParent(parent);
            vsg::GeometryInfo geomInfo;
            vsg::StateInfo stateInfo;

            // todo: check how culling is done in vsg
            //geomInfo.cullNode = settings.insertCullNode;
            geomInfo.color.set(1.0f, 1.0f, 1.5f, 1.0f);
            //fprintf(stderr, "createObject spec:\n%s\n", spec.toYamlString().c_str());


            // for testing we try to load shaders from working dir
            auto vertexShader = vsg::ShaderStage::create(VK_SHADER_STAGE_VERTEX_BIT, "main", skybox_vert);
            auto fragmentShader = vsg::ShaderStage::create(VK_SHADER_STAGE_FRAGMENT_BIT, "main", skybox_frag);
            if (!vertexShader || !fragmentShader)
            {
                std::cout << "Could not create shaders." << std::endl;
            }
            const vsg::ShaderStages shaders{vertexShader, fragmentShader};

            vsg::DescriptorSetLayoutBindings descriptorBindings{
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},            // { binding, descriptorType, descriptorCount, stageFlags, pImmutableSamplers}
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
            };
            auto descriptorSetLayout = vsg::DescriptorSetLayout::create(descriptorBindings);

            auto worldTransformUniformDescriptor = vsg::DescriptorBuffer::create(worldTransformUniform, 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            auto descriptorSet = vsg::DescriptorSet::create(descriptorSetLayout, vsg::Descriptors{worldTransformUniformDescriptor});

            vsg::PushConstantRanges pushConstantRanges{
                {VK_SHADER_STAGE_VERTEX_BIT, 0, 128} // projection, view, and model matrices, actual push constant calls automatically provided by the VSG's RecordTraversal
            };

            vsg::VertexInputState::Bindings vertexBindingsDescriptions{
                VkVertexInputBindingDescription{0, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX}};

            vsg::VertexInputState::Attributes vertexAttributeDescriptions{
                VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}};

            auto rasterState = vsg::RasterizationState::create();
            rasterState->cullMode = VK_CULL_MODE_FRONT_BIT;

            auto depthState = vsg::DepthStencilState::create();
            depthState->depthTestEnable = VK_TRUE;
            depthState->depthWriteEnable = VK_FALSE;
            depthState->depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;

            vsg::GraphicsPipelineStates pipelineStates{
                vsg::VertexInputState::create(vertexBindingsDescriptions, vertexAttributeDescriptions),
                vsg::InputAssemblyState::create(),
                rasterState,
                vsg::MultisampleState::create(),
                vsg::ColorBlendState::create(),
                depthState};

            auto pipelineLayout = vsg::PipelineLayout::create(vsg::DescriptorSetLayouts{descriptorSetLayout}, pushConstantRanges);
            auto pipeline = vsg::GraphicsPipeline::create(pipelineLayout, shaders, pipelineStates);
            auto bindDescriptorSets = vsg::BindDescriptorSets::create(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, vsg::DescriptorSets{descriptorSet});
            auto bindGraphicsPipeline = vsg::BindGraphicsPipeline::create(pipeline);

            auto root = vsg::StateGroup::create();
            root->add(bindGraphicsPipeline);
            root->add(bindDescriptorSets);

            //root->add(bindDescriptorSet); // descriptor set is used for textures and uniforms etc
            
            if(spec.hasKey("filename"))
            {
                if(spec["filename"] == "PRIMITIVE")
                {
                    // todo: use options from GuiHelper
                    auto options = vsg::Options::create();
                    options->paths = vsg::getEnvPaths("VSG_FILE_PATH");
                    options->sharedObjects = vsg::SharedObjects::create();
                    auto builder = vsg::Builder::create();
                    builder->options = options;
                    std::string type = spec["origname"];
                    if(type == "box")
                    {
                        geomInfo.dx.set((double)spec["extend"]["x"], 0, 0);
                        geomInfo.dy.set(0, (double)spec["extend"]["y"], 0);
                        geomInfo.dz.set(0, 0, (double)spec["extend"]["z"]);
                        drawObject = builder->createBox(geomInfo, stateInfo);
                    }
                    else if(type == "plane")
                    {
                        geomInfo.dx.set((double)spec["extend"]["x"], 0, 0);
                        geomInfo.dy.set(0, (double)spec["extend"]["y"], 0);
                        drawObject = builder->createQuad(geomInfo, stateInfo);
                    }
                    else if(type == "sphere")
                    {
                        double d = (double)spec["extend"]["x"]*2;
                        geomInfo.dx.set(d, 0, 0);
                        geomInfo.dy.set(0, d, 0);
                        geomInfo.dz.set(0, 0, d);
                        drawObject = builder->createSphere(geomInfo, stateInfo);
                    }
                    // if(drawObject.cast<vsg::StateGroup>())
                    // {
                    //     LOG_ERROR("have stateGroup");
                    //     root->addChild(drawObject.cast<vsg::StateGroup>()->children[0]);
                    // }
                    // else if(drawObject.cast<vsg::CullNode>())
                    // {
                    //     LOG_ERROR("have cullNode");
                    //     root->addChild(drawObject.cast<vsg::CullNode>()->child);
                    // }
                    // else
                    // {
                    //     LOG_ERROR("have no stateGroup nor cullNode");
                    //     root->addChild(drawObject);
                    // }
                    // drawObject = root;
                }
                else
                {
                    drawObject = GuiHelper::readNodeFromFile((std::string)(spec["filename"]));
                    vsg::ref_ptr<vsg::PbrMaterialValue> materialValue(extractMaterialValue(drawObject));
                    auto material = (vsg::PbrMaterial*)(materialValue->dataPointer(0));
                    // material->diffuseFactor = vsg::vec4{0.9,0.9,1.0,1.0};
                    // material->defines.insert("VSG_TWO_SIDED_LIGHTING");
                    vsg::ref_ptr<vsg::DescriptorConfigurator> materialConfig = vsg::DescriptorConfigurator::create();
                    materialConfig->defines.insert("VSG_TWO_SIDED_LIGHTING");
                    //materialConfig->assignDescriptor("material", materialValue);
                }
                if(drawObject)
                {
                    poseTransform->addChild(drawObject);
                    if(parent)
                    {
                        parent->addChild(poseTransform);
                    }
                }
            }
        }

        void DrawObject::setParent(vsg::ref_ptr<vsg::Group> parent)
        {
            this->parent = parent;
        }

        void DrawObject::setPosition(const utils::Vector &pos)
        {
            position = pos;
            applyTransform();
        }

        void DrawObject::setQuaternion(const utils::Quaternion &q)
        {
            quaternion = q;
            applyTransform();
        }

        void DrawObject::applyTransform()
        {
            vsg::dvec3 p(position.x(), position.y(), position.z());
            vsg::dquat q;
            q.x = quaternion.x();
            q.y = quaternion.y();
            q.z = quaternion.z();
            q.w = quaternion.w();
            poseTransform->matrix = vsg::translate(p) * vsg::rotate(q);
        }

        void DrawObject::setVisible(bool v)
        {
            if(v != visible)
            {
                visible = v;
                if(drawObject && parent)
                {
                    if(!visible)
                    {
                        auto it = std::find(parent->children.begin(), parent->children.end(), poseTransform);
                        parent->children.erase(it);
                    }
                    else
                    {
                        parent->addChild(poseTransform);
                    }
                }
            }
        }
    }
}
