#include "gui_helper_functions.hpp"
#include "Bobj.hpp"
#include "MARSStateGroup.hpp"
#include <mars_utils/misc.h>

using namespace std;

namespace mars
{
    using namespace utils;
    using namespace interfaces;

    namespace vsg_graphics
    {

        vsg::ref_ptr<WorldTransformUniformValue> GuiHelper::worldTransformUniform = 0;
        vsg::ref_ptr<vsg::Options> GuiHelper::loadOptions = nullptr;
        std::map<std::string, GraphShader> GuiHelper::graphShaderFiles;
        std::map<std::string, vsg::ref_ptr<vsg::Node>> GuiHelper::nodeFiles;
        std::map<std::string, vsg::ref_ptr<vsg::StateGroup>> GuiHelper::stateGroups;
        vsg::ref_ptr<vsg::Group> GuiHelper::stateGroupNodes = vsg::StateGroup::create();
        std::string GuiHelper::resourcePath = "";

        // Extract a pointer to the materialValue
        vsg::PbrMaterialValue *extractMaterialValue(vsg::Node* node, bool makeDynamic)
        {
            struct ExtractMaterialVisitor : public vsg::Visitor
            {
                ExtractMaterialVisitor() {}

                void apply(vsg::Object& object) override
                    {
                        object.traverse(*this);
                    }

                // Find and return the PbrMaterial value
                void apply(vsg::DescriptorBuffer& db) override
                    {
                        for (auto& bfi : db.bufferInfoList)
                        {
                            auto data = bfi->data;
                            //print("Found bfi of type {}\n", data->type_info().name());
                            if (data->is_compatible(typeid(vsg::PbrMaterialValue))) {
                                if (makeDynamic)
                                    data->properties.dataVariance = vsg::DYNAMIC_DATA;

                                // Store the materialValue so that it can be returned to the user
                                this->materialValue = data->cast<vsg::PbrMaterialValue>();
                            }
                        }
                    }

                bool makeDynamic = true;
                vsg::PbrMaterialValue *materialValue = nullptr;
            };

            ExtractMaterialVisitor emv;
            emv.makeDynamic = makeDynamic;
            node->accept(emv);
            if (!emv.materialValue)
                throw std::runtime_error("Failed finding the material value!");
            return emv.materialValue;
        }

        vsg::vec4 toOVGVec4(const Color &col)
        {
            return vsg::vec4(col.r, col.g, col.b, col.a);
        }
        vsg::vec4 toVSGVec4(const Vector &v, float w)
        {
            return vsg::vec4(v.x(), v.y(), v.z(), w);
        }

        GuiHelper::GuiHelper(interfaces::GraphicsManagerInterface *gi) : graphicsInterface(gi)
        {
            (void)this->graphicsInterface;
        }

        GuiHelper::~GuiHelper()
        {
            GuiHelper::worldTransformUniform = 0;
            GuiHelper::stateGroupNodes = 0;
            GuiHelper::loadOptions = 0;
            graphShaderFiles.clear();
            stateGroups.clear();
            nodeFiles.clear();
        }

        /** \brief converts the mesh of an osgNode to the snmesh struct */
        mars::interfaces::snmesh GuiHelper::convertOsgNodeToSnMesh(vsg::Node *node,
                                                                   double scaleX,
                                                                   double scaleY,
                                                                   double scaleZ,
                                                                   double pivotX,
                                                                   double pivotY,
                                                                   double pivotZ)
        {
            (void)node;
            (void)scaleX;
            (void)scaleY;
            (void)scaleZ;
            (void)pivotX;
            (void)pivotY;
            (void)pivotZ;
            return mars::interfaces::snmesh();
        }

        std::vector<double> GuiHelper::getMeshSize(const std::string &filename)
        {
            (void)filename;
            return std::vector<double>();
        }

        void GuiHelper::getPhysicsFromMesh(mars::interfaces::NodeData *node)
        {
            (void)node;
        }

        void GuiHelper::readPixelData(mars::interfaces::terrainStruct *terrain)
        {
            (void)terrain;
        }

        GraphShader& GuiHelper::readGraphShaderFromFile(std::string fileName)
        {
            auto it = graphShaderFiles.find(fileName);
            if(it != graphShaderFiles.end()) {
                return it->second;
            }
            GraphShader gs;
            configmaps::ConfigMap graphShaderMap = configmaps::ConfigMap::fromYamlFile(fileName);
            gs.loadShader(graphShaderMap);
            graphShaderFiles[fileName] = gs;
            return graphShaderFiles[fileName];
        }

        vsg::ref_ptr<vsg::Node> GuiHelper::readNodeFromFile(std::string fileName)
        {
            vsg::ref_ptr<vsg::Node> node;
            auto it = nodeFiles.find(fileName);
            if(it != nodeFiles.end()) {
                node = it->second;
                return node;
            }

            if(!loadOptions)
            {
                loadOptions = vsg::Options::create(vsgXchange::all::create());
                // todo: learn the shared concept of vsg, it looks like it provides
                //       the same functionality as our nodeFiles from MARS1
                loadOptions->sharedObjects = vsg::SharedObjects::create();
                loadOptions->add(vsgXchange::all::create());
                loadOptions->setValue("two_sided", true);
            }
            node = vsg::read_cast<vsg::Node>(fileName, loadOptions);
            nodeFiles[fileName] = node;

            return node;
        }

        vsg::ref_ptr<vsg::Node> GuiHelper::readBobjFromFile(const std::string &filename)
        {
            { // check if we loaded the file already into memory
                auto it = nodeFiles.find(filename);
                if(it != nodeFiles.end()) {
                    return it->second;
                }
            }
            auto node = Bobj::readFromFile(filename);
            nodeFiles[filename] = node;

            return node;
        }

        vsg::ref_ptr<vsg::Data> GuiHelper::loadTexture(std::string filename)
        {
            (void)filename;
            return vsg::ref_ptr<vsg::Data>();
        }

        vsg::ref_ptr<vsg::DescriptorImage> GuiHelper::loadImage(std::string filename)
        {
            (void)filename;
            return vsg::ref_ptr<vsg::DescriptorImage>();
        }

        void GuiHelper::getPhysicsFromNode(mars::interfaces::NodeData* node,
                                           vsg::ref_ptr<vsg::Node> completeNode)
        {
            (void)node;
            (void)completeNode;
        }

        bool GuiHelper::checkBobj(std::string &filename)
        {
            return Bobj::checkBobj(filename);
        }

        vsg::ref_ptr<vsg::StateGroup> GuiHelper::createStateGroup(configmaps::ConfigMap materialSpec)
        {
            std::string materialName = materialSpec["name"];
            { // check if we loaded the file already into memory
                auto it = stateGroups.find(materialName);
                if(it != stateGroups.end()) {
                    return it->second;
                }
            }
            auto stateGroup = MARSStateGroup::create(materialSpec);
            stateGroupNodes->addChild(stateGroup);
            stateGroups[materialName] = stateGroup;
            return stateGroup;
        }

    } // end of namespace vsg_graphics
} // end of namespace mars
