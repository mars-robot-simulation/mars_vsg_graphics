#include "gui_helper_functions.hpp"

namespace mars
{
    using namespace utils;
    using namespace interfaces;


    namespace vsg_graphics
    {


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

        vsg::ref_ptr<vsg::Options> GuiHelper::loadOptions = nullptr;

        GuiHelper::GuiHelper(interfaces::GraphicsManagerInterface *gi) : gi(gi)
        {
        }

        /** \brief converts the mesh of an osgNode to the snmesh struct */
        mars::interfaces::snmesh GuiHelper::convertOsgNodeToSnMesh(vsg::Node *node, 
                                                                   double scaleX,
                                                                   double scaleY,
                                                                   double scaleZ,
                                                                   double pivotX,
                                                                   double pivotY,
                                                                   double pivotZ)
        {}

        std::vector<double> GuiHelper::getMeshSize(const std::string &filename) {}
        void GuiHelper::getPhysicsFromMesh(mars::interfaces::NodeData *node) {}
        void GuiHelper::readPixelData(mars::interfaces::terrainStruct *terrain) {}

        vsg::ref_ptr<vsg::Node> GuiHelper::readNodeFromFile(std::string fileName)
        {
            if(!loadOptions)
            {
                loadOptions = vsg::Options::create(vsgXchange::all::create());
                loadOptions->sharedObjects = vsg::SharedObjects::create();
                loadOptions->add(vsgXchange::all::create());
                loadOptions->setValue("two_sided", true);
            }
            return vsg::read_cast<vsg::Node>(fileName, loadOptions);
        }

        vsg::ref_ptr<vsg::Node> GuiHelper::readBobjFromFile(const std::string &filename) {}
        vsg::ref_ptr<vsg::Data> GuiHelper::loadTexture(std::string filename) {}
        vsg::ref_ptr<vsg::DescriptorImage> GuiHelper::loadImage(std::string filename) {}
        void GuiHelper::getPhysicsFromNode(mars::interfaces::NodeData* node,
                                           vsg::ref_ptr<vsg::Node> completeNode) {}

    } // end of namespace vsg_graphics
} // end of namespace mars

