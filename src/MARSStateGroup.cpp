#include "MARSStateGroup.hpp"
#include "gui_helper_functions.hpp"
#include "ImageUtils.hpp"

#include <vsg/all.h>
#include <mars_interfaces/Logging.hpp>
#include <configmaps/ConfigData.h>
#include <mars_utils/misc.h>

namespace mars
{
    namespace vsg_graphics
    {
        MARSStateGroup::MARSStateGroup(configmaps::ConfigMap materialSpec) :
            Inherit()
        {

            name = "";
            std::string loadPath = ".";
            if(materialSpec.hasKey("loadPath"))
            {
                loadPath << materialSpec["loadPath"];
            }
            if(materialSpec.hasKey("filePrefix"))
            {
                loadPath << materialSpec["filePrefix"];
            }
            if(materialSpec.hasKey("name"))
            {
                name << materialSpec["name"];
            }

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
            material.emissiveFactor[0] = (double)materialSpec["emissionColor"]["r"];
            material.emissiveFactor[1] = (double)materialSpec["emissionColor"]["g"];
            material.emissiveFactor[2] = (double)materialSpec["emissionColor"]["b"];
            material.emissiveFactor[3] = (double)materialSpec["emissionColor"]["a"];

            // binding 0 is always the material descriptor so we start with
            // 1 for textures
            unsigned short binding = 1;
            bool haveDiffuseMap = false;
            std::vector<TextureMapping> textureMapping;
            if(materialSpec.hasKey("diffuseTexture"))
            {
                TextureMapping tm;
                //fprintf(stderr, "%s\n", materialSpec.toYamlString().c_str());
                std::string filename = materialSpec["diffuseTexture"];
                filename = utils::pathJoin(loadPath, filename);

                tm.name = "diffuseMap";
                tm.image = GuiHelper::loadImage(filename);
                tm.binding = binding++;
                tm.descriptor = vsg::DescriptorImage::create(vsg::Sampler::create(), tm.image, tm.binding, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
                textureMapping.push_back(tm);
                haveDiffuseMap = true;
            }

            if(materialSpec.hasKey("textures"))
            {
                for(auto &it: materialSpec["textures"])
                {
                    TextureMapping tm;
                    tm.name << it["name"];
                    std::string filename = it["file"];
                    filename = utils::pathJoin(loadPath, filename);
                    tm.image = GuiHelper::loadImage(filename);
                    tm.binding = binding++;
                    if(tm.name == "diffuseMap")
                    {
                        haveDiffuseMap = true;
                    }

                    if(it.hasKey("dynamic") && (bool)it["dynamic"] == true)
                    {
                        tm.image->properties.dataVariance = vsg::DYNAMIC_DATA;
                    }
                    if(it.hasKey("storage") && (bool)it["storage"] == true)
                    {
                        tm.storage = true;
                    }
                    if(it.hasKey("dst") && (bool)it["dst"] == true)
                    {
                        tm.dst = true;
                    }
                    if(it.hasKey("src") && (bool)it["src"] == true)
                    {
                        tm.src = true;
                    }
                    if(tm.storage || tm.dst || tm.src)
                    {
                        // todo: figure out if when we have to use storage
                        auto image = vsg::Image::create(tm.image);
                        image->usage = 0;
                        if(tm.storage)
                        {
                            image->usage |= VK_IMAGE_USAGE_STORAGE_BIT;
                        }
                        if(tm.dst)
                        {
                            //image->usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                            image->usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                            //image->layout = VK_IMAGE_LAYOUT_GENERAL;
                            //image->tiling = VK_IMAGE_TILING_OPTIMAL;
                            image->sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                        }
                        if(tm.src)
                        {
                            image->usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
                            //image->layout = VK_IMAGE_LAYOUT_GENERAL;
                            //image->usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
                            //image->tiling = VK_IMAGE_TILING_OPTIMAL;
                            image->sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                        }

                        auto imageView = vsg::ImageView::create(image, VK_IMAGE_ASPECT_COLOR_BIT);
                        auto sii = vsg::ImageInfo::create(vsg::Sampler::create(), imageView);
                        if(tm.storage)
                        {
                            tm.descriptor = vsg::DescriptorImage::create(sii, tm.binding, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
                        }
                        else
                        {
                            tm.descriptor = vsg::DescriptorImage::create(sii, tm.binding, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
                        }

                        if(it.hasKey("createReadCommand") && (bool)it["createReadCommand"] == true)
                        {
                            LOG_ERROR("createReadCommand for %s with size: %d %d", tm.name.c_str(), image->extent.width, image->extent.height);
                            VkExtent2D targetExtent{image->extent.width, image->extent.height};
                            auto captureImage = createCaptureImage(VK_FORMAT_B8G8R8A8_SRGB, targetExtent);
                            captureImages[tm.name] = captureImage;
                            auto commands = createTransferCommands(image,
                                                                   captureImage);
                            captureCommands.push_back(commands);
                        }
                        if(it.hasKey("transfereTo"))
                        {
                            auto targetImageIt = images.find(it["transfereTo"]);
                            if(targetImageIt != images.end())
                            {
                                //LOG_ERROR("createReadCommand for %s with size: %d %d", tm.name.c_str(), image->extent.width, image->extent.height);
                                auto commands = createTransferCommandsI(image,
                                                                       targetImageIt->second);
                                captureCommands.push_back(commands);
                            }
                        }
                    } else
                    {
                        tm.descriptor = vsg::DescriptorImage::create(vsg::Sampler::create(), tm.image, tm.binding, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
                    }
                    images[tm.name] = tm.descriptor->imageInfoList[0]->imageView->image;
                    textureMapping.push_back(tm);
                }
            }

            material.roughnessFactor = 10.0/(double)materialSpec["shininess"];
            if(material.roughnessFactor > 1.0) material.roughnessFactor = 1.0;

            material.metallicFactor = material.roughnessFactor;

            // load shaders
            std::string vertexShaderFile = utils::pathJoin(GuiHelper::resourcePath, "resources/graph_shader/default_vertex_shader.yml");
            std::string fragmentShaderFile = utils::pathJoin(GuiHelper::resourcePath, "resources/graph_shader/default_fragment_shader.yml");
            if(haveDiffuseMap)
            {
                fragmentShaderFile = utils::pathJoin(GuiHelper::resourcePath, "resources/graph_shader/default_diffuseMap_fragment_shader.yml");
                //LOG_ERROR("load diffuseMap shader");
            }

            if(materialSpec.hasKey("shader"))
            {
                if(materialSpec["shader"]["provider"] == "DRockGraph")
                {
                    if(materialSpec["shader"].hasKey("vertex"))
                    {
                        vertexShaderFile << materialSpec["shader"]["vertex"];
                        vertexShaderFile = utils::pathJoin(loadPath, vertexShaderFile);
                    }
                    if(materialSpec["shader"].hasKey("fragment"))
                    {
                        fragmentShaderFile << materialSpec["shader"]["fragment"];
                        fragmentShaderFile = utils::pathJoin(loadPath, fragmentShaderFile);
                    }
                }
            }

            GraphShader &vs = GuiHelper::readGraphShaderFromFile(vertexShaderFile);
            GraphShader &fs = GuiHelper::readGraphShaderFromFile(fragmentShaderFile);

            // copy varyings from vertex shader to fragment shader
            fs.varyings = vs.varyings;

            vsg::DescriptorSetLayoutBindings descriptorBindings{
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
            };
            for(auto &tm: textureMapping)
            {
                if(tm.storage)
                {
                    descriptorBindings.push_back({tm.binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
                } else
                {
                    descriptorBindings.push_back({tm.binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
                }
                for(auto &u : fs.uniforms)
                {
                    if(u.second.name == tm.name)
                    {
                        u.second.set = 0;
                        u.second.binding = tm.binding;
                    }
                }
            }

            auto descriptorSetLayout = vsg::DescriptorSetLayout::create(descriptorBindings);

            // for testing we try to load shaders from working dir
            auto vertexShader = vsg::ShaderStage::create(VK_SHADER_STAGE_VERTEX_BIT, "main", vs.generateVertexShaderSource());
            auto fragmentShader = vsg::ShaderStage::create(VK_SHADER_STAGE_FRAGMENT_BIT, "main", fs.generateFragmentShaderSource());
            if (!vertexShader || !fragmentShader)
            {
                std::cout << "Could not create shaders." << std::endl;
            }

            if(materialSpec.hasKey("printShader") && (bool)materialSpec["printShader"])
            {
                std::string source = vs.generateVertexShaderSource();
                std::string filename = "shader_sources/" + name + "_vert.c";
                utils::createDirectory("shader_sources");
                FILE *f = fopen(filename.c_str(), "w");
                fprintf(f, "%s", source.c_str());
                fclose(f);
                source = fs.generateFragmentShaderSource();
                filename = "shader_sources/" + name + "_frag.c";
                f = fopen(filename.c_str(), "w");
                fprintf(f, "%s", source.c_str());
                fclose(f);
            }

            const vsg::ShaderStages shaders{vertexShader, fragmentShader};

            // todo: howto deal with global uniforms
            //auto worldTransformUniformDescriptor = vsg::DescriptorBuffer::create(GuiHelper::worldTransformUniform, 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            auto materialUniformDescriptor = vsg::DescriptorBuffer::create(vsg::PbrMaterialValue::create(material), 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            //auto descriptorSet = vsg::DescriptorSet::create(descriptorSetLayout, vsg::Descriptors{materialUniformDescriptor});
            vsg::Descriptors descriptors{materialUniformDescriptor};
            for(auto &tm: textureMapping)
            {
                descriptors.push_back(tm.descriptor);
            }
            auto descriptorSet = vsg::DescriptorSet::create(descriptorSetLayout, descriptors);

            vsg::PushConstantRanges pushConstantRanges{
                {VK_SHADER_STAGE_VERTEX_BIT, 0, 128} // projection, view, and model matrices, actual push constant calls automatically provided by the VSG's RecordTraversal
            };

            vsg::VertexInputState::Bindings vertexBindingsDescriptions{
                VkVertexInputBindingDescription{0, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX}, // vsg_Vertex
                VkVertexInputBindingDescription{1, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX}, // vsg_Normal
                VkVertexInputBindingDescription{2, sizeof(vsg::vec2), VK_VERTEX_INPUT_RATE_VERTEX}}; // vsg_TexCoord0

            vsg::VertexInputState::Attributes vertexAttributeDescriptions{
                VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, // vsg_Vertex
                VkVertexInputAttributeDescription{1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0}, // vsg_Normal
                VkVertexInputAttributeDescription{2, 2, VK_FORMAT_R32G32_SFLOAT, 0}}; // vsg_TexCoord0

            auto rasterState = vsg::RasterizationState::create();
            rasterState->cullMode = VK_CULL_MODE_BACK_BIT;

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
            auto pipelineLayout = vsg::PipelineLayout::create(vsg::DescriptorSetLayouts{descriptorSetLayout, viewDescriptorSetLayout}, pushConstantRanges);
            auto pipeline = vsg::GraphicsPipeline::create(pipelineLayout, shaders, pipelineStates);
            auto bindDescriptorSets = vsg::BindDescriptorSets::create(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, vsg::DescriptorSets{descriptorSet});
            auto bindGraphicsPipeline = vsg::BindGraphicsPipeline::create(pipeline);

            vsg::ref_ptr<vsg::Image> srcImage, dstImage;
            // for(auto tm: textureMapping)
            // {
            //     if(tm.name == "storeImageMap")
            //     {
            //         srcImage = tm.descriptor->imageInfoList[0]->imageView->image;
            //     }
            //     else if(tm.name == "diffuseMap")
            //     {
            //         dstImage = tm.descriptor->imageInfoList[0]->imageView->image;
            //     }
            // }

            uint32_t vds_set = 1;

            //auto root = vsg::StateGroup::create();
            add(bindGraphicsPipeline);
            add(bindDescriptorSets); // descriptor set is used for textures and uniforms etc
            add(vsg::BindViewDescriptorSets::create(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, vds_set));
            //return root;
        }
    }
}
