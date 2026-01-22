#pragma once
#include "shader/GraphShader.hpp"
#include <vsg/all.h>
#include <vsgXchange/all.h>

#include <mars_interfaces/sim/LoadCenter.h>
#include <mars_interfaces/graphics/GraphicsManagerInterface.hpp>
#include <mars_interfaces/Logging.hpp>

namespace mars
{
    namespace vsg_graphics
    {

        struct WorldTransformUniform
        {
            vsg::mat4 projInverse;
            vsg::mat4 viewInverse;
        };
        using WorldTransformUniformValue = vsg::Value<WorldTransformUniform>;

        struct nodeFileStruct
        {
            std::string fileName;
            vsg::ref_ptr<vsg::Node> node;
        };

        struct textureFileStruct
        {
            std::string fileName;
            vsg::ref_ptr<vsg::DescriptorImage> texture;
        };

        struct imageFileStruct
        {
            std::string fileName;
            vsg::ref_ptr<vsg::Data> image;
        };

        
        // enable wireframe mode to visualize line width of the mesh
        struct SetGlobalPipelineStates : public vsg::Visitor
        {
            void apply(vsg::Object& object) override
                {
                    fprintf(stderr, "-------------- traverse %s\n", object.className());
                    //fprintf(stderr, "-------------- traverse\n");
                    object.traverse(*this);
                }

            void apply(vsg::RasterizationState& rs) override
                {
                    fprintf(stderr, "-------------- apply rasterization state\n");
                    rs.cullMode = VK_CULL_MODE_NONE;
                }

            void apply(vsg::StateGroup &v) override
                {
                    for (auto& sc : v.stateCommands)
                    {
                        if (auto bgp = sc->cast<vsg::BindGraphicsPipeline>())
                        {
                            for(auto &stage : bgp->pipeline->stages)
                            {
                                if(stage->module && stage->module->hints)
                                {
                                    fprintf(stderr, "add define two sided lighting\n");
                                    stage->module->hints->defines.insert("VSG_TWO_SIDED_LIGHTING");
                                }
                            }
                            for(auto& state : bgp->pipeline->pipelineStates)
                            {
                                if(auto rs = state->cast<vsg::RasterizationState>())
                                {
                                    //fprintf(stderr, "-------------- apply rasterization state\n");
                                    rs->cullMode = VK_CULL_MODE_NONE;
                                }
                            }
                        }
                    }
                    //dc.defines.insert("VSG_TWO_SIDED_LIGHTING");
                }
            
            void apply(vsg::BindGraphicsPipeline &v) override
                {
                    
                    fprintf(stderr, "-------------- apply %s\n", v.className());
                    //dc.defines.insert("VSG_TWO_SIDED_LIGHTING");
                }

        };

        vsg::PbrMaterialValue *extractMaterialValue(vsg::Node* node, bool makeDynamic=true);
        vsg::vec4 toOSGVec4(const mars::utils::Color &col);
        vsg::vec4 toOSGVec4(const mars::utils::Vector &v, float w);


        class GuiHelper : public interfaces::LoadMeshInterface,
                          public interfaces::LoadHeightmapInterface
        {
        public:
            GuiHelper(interfaces::GraphicsManagerInterface *gi);
            ~GuiHelper();

            /** \brief converts the mesh of an osgNode to the snmesh struct */
            static mars::interfaces::snmesh convertOsgNodeToSnMesh(vsg::Node *node, 
                                                                   double scaleX,
                                                                   double scaleY,
                                                                   double scaleZ,
                                                                   double pivotX,
                                                                   double pivotY,
                                                                   double pivotZ);

            virtual std::vector<double> getMeshSize(const std::string &filename);
            virtual void getPhysicsFromMesh(mars::interfaces::NodeData *node);
            virtual void readPixelData(mars::interfaces::terrainStruct *terrain);

            static GraphShader& readGraphShaderFromFile(std::string fileName);
            static vsg::ref_ptr<vsg::Node> readNodeFromFile(std::string fileName);
            static vsg::ref_ptr<vsg::Node> readBobjFromFile(const std::string &filename);
            static vsg::ref_ptr<vsg::Data> loadTexture(std::string filename);
            static vsg::ref_ptr<vsg::DescriptorImage> loadImage(std::string filename);
            static std::string resourcePath;
            static bool checkBobj(std::string &filename);
            static vsg::ref_ptr<vsg::StateGroup> createStateGroup(configmaps::ConfigMap material);

            static vsg::ref_ptr<WorldTransformUniformValue> worldTransformUniform;
            static vsg::ref_ptr<vsg::Group> stateGroupNodes;

        private:
            interfaces::GraphicsManagerInterface *graphicsInterface;
            static vsg::ref_ptr<vsg::Options> loadOptions;

            // map to prevent double load of shader files
            static std::map<std::string, GraphShader> graphShaderFiles;
            static std::map<std::string, vsg::ref_ptr<vsg::StateGroup>> stateGroups;

            // map to prevent double load of mesh files
            static std::map<std::string, vsg::ref_ptr<vsg::Node>> nodeFiles;
            // // vector to prevent double load of textures
            // static std::vector<textureFileStruct> textureFiles;
            // // vector to prevent double load of images
            // static std::vector<imageFileStruct> imageFiles;

            void getPhysicsFromNode(mars::interfaces::NodeData* node,
                                    vsg::ref_ptr<vsg::Node> completeNode);
        };
    }
}
