#include "DrawObject.hpp"
#include "shader/GraphShader.hpp"
#include "gui_helper_functions.hpp"
#include <mars_utils/misc.h>

namespace mars
{
    namespace vsg_graphics
    {

        struct RemoveShader : public vsg::Visitor
        {
            void apply(vsg::Object& object) override
                {
                    //fprintf(stderr, "-------------- traverse %s\n", object.className());
                    object.traverse(*this);
                }

            void apply(vsg::Group &v) override
                {
                    //fprintf(stderr, "-------------- traverse Group %p\n", &v);
                    vsg::ref_ptr<vsg::Group> p(&v);

                    // if(auto rs = p->cast<vsg::MatrixTransform>())
                    // {
                    //     fprintf(stderr, "------------------ Group is MatrisTransform %p\n", &v);
                    // }

                    v.traverse(*this);
                    for(auto &c: children)
                    {
                        p->addChild(c);
                    }
                    for(auto &s: stateGroups)
                    {
                        p->children.erase(std::find(p->children.begin(), p->children.end(), s));
                    }
                    stateGroups.clear();
                    children.clear();
                }

            void apply(vsg::StateGroup &v) override
                {
                    //fprintf(stderr, "-------------- traverse StateGroup %p\n", &v);
                    children.push_back(v.children[0]);
                    stateGroups.push_back(&v);
                }

            std::vector<vsg::ref_ptr<vsg::Node>> children;
            std::vector<vsg::StateGroup*> stateGroups;

        };

        DrawObject::DrawObject() : visible(true)
        {
            poseTransform = vsg::MatrixTransform::create(vsg::translate(vsg::dvec3(0, 0, 0)));
        }

        DrawObject::~DrawObject()
        {

        }

        void DrawObject::createObject(configmaps::ConfigMap spec, vsg::ref_ptr<vsg::Group> parent_)
        {
            if(parent_) setParent(parent_);

            //fprintf(stderr, "createObject spec:\n%s\n", spec.toYamlString().c_str());

            // todo: prefix material names by worlds?
            auto stateGroup = GuiHelper::createStateGroup(spec["material"]);

            if(spec.hasKey("filename"))
            {
                if(spec["filename"] == "PRIMITIVE")
                {
                    vsg::GeometryInfo geomInfo;
                    vsg::StateInfo stateInfo;
                    // todo: check how culling is done in vsg
                    //geomInfo.cullNode = settings.insertCullNode;
                    geomInfo.color.set(1.0f, 1.0f, 1.5f, 1.0f);

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
                }
                else
                {
                    std::string filename = spec["filename"];
                    if(GuiHelper::checkBobj(filename))
                    {
                        drawObject = GuiHelper::readBobjFromFile(filename);
                    } else
                    {
                        drawObject = GuiHelper::readNodeFromFile(filename);
                        // we have to remove the render pipeline which was created by the loader
                        vsg::visit<RemoveShader>(drawObject);
                    }
/*
                    vsg::ref_ptr<vsg::PbrMaterialValue> materialValue(extractMaterialValue(drawObject));
                    auto material = (vsg::PbrMaterial*)(materialValue->dataPointer(0));
material->diffuseFactor = vsg::vec4{0.0,0.9,1.0,1.0};
*/
                    // material->defines.insert("VSG_TWO_SIDED_LIGHTING");
                    //vsg::ref_ptr<vsg::DescriptorConfigurator> materialConfig = vsg::DescriptorConfigurator::create();
                    //materialConfig->defines.insert("VSG_TWO_SIDED_LIGHTING");
                    //materialConfig->assignDescriptor("material", materialValue);
                }
                if(drawObject)
                {

                    // apply own shader pipeline
                    // if(drawObject.cast<vsg::StateGroup>())
                    // {
                    //     //LOG_ERROR("have stateGroup");
                    //     stateGroup->addChild(drawObject.cast<vsg::StateGroup>()->children[0]);
                    // }
                    // else if(drawObject.cast<vsg::CullNode>())
                    // {
                    //     //LOG_ERROR("have cullNode");
                    //     stateGroup->addChild(drawObject);
                    // }
                    // else
                    // {
                    //     //LOG_ERROR("have no stateGroup nor cullNode");
                    //     stateGroup->addChild(drawObject);
                    // }
                    // drawObject = stateGroup;

                    materialStateGroup = stateGroup;

                    // if(parent)
                    // {
                    //     parent->addChild(poseTransform);
                    // }

                    // vs. have the stategroup only once in the graph and have the drawobjects as children of the stategroup
                    poseTransform->addChild(drawObject);
                    stateGroup->addChild(poseTransform);
                    // todo: how to deal with parents?
                    // if(parent)
                    // {
                    //     parent->addChild(poseTransform);
                    // }
                    //vsg::write(root, "debug.vsgt");
                }
            }
        }

        void DrawObject::setParent(vsg::ref_ptr<vsg::Group> parent_)
        {
            this->parent = parent_;
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
                if(drawObject)
                {
                    // if(!visible)
                    // {
                    //     auto it = std::find(parent->children.begin(), parent->children.end(), poseTransform);
                    //     parent->children.erase(it);
                    // }
                    // else
                    // {
                    //     parent->addChild(poseTransform);
                    // }
                    if(!visible)
                    {
                        auto it = std::find(materialStateGroup->children.begin(),
                                            materialStateGroup->children.end(),
                                            poseTransform);
                        materialStateGroup->children.erase(it);
                    }
                    else
                    {
                        materialStateGroup->addChild(poseTransform);
                    }
                }
            }
        }
    }
}
