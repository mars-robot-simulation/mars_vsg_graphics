#include "Manipulator.hpp"
#include <mars_utils/misc.h>
#include "gui_helper_functions.hpp"
#include <mars_interfaces/Logging.hpp>

namespace mars
{
    namespace vsg_graphics
    {
        Manipulator::Manipulator(std::string resourcesPath) : Inherit()
        {
            std::string file = utils::pathJoin(resourcesPath, "resources/Objects/grabx.obj");
            auto obj = GuiHelper::readNodeFromFile(file);
            if(!obj)
            {
                LOG_ERROR("Failed to load: %s", file.c_str());
                return;
            }
            x = obj->cast<vsg::Node>();

            file = utils::pathJoin(resourcesPath, "resources/Objects/graby.obj");
            obj = GuiHelper::readNodeFromFile(file);
            if(!obj)
            {
                LOG_ERROR("Failed to load: %s", file.c_str());
                return;
            }
            y = obj->cast<vsg::Node>();

            file = utils::pathJoin(resourcesPath, "resources/Objects/grabz.obj");
            obj = GuiHelper::readNodeFromFile(file);
            if(!obj)
            {
                LOG_ERROR("Failed to load: %s", file.c_str());
                return;
            }
            z = obj->cast<vsg::Node>();

            file = utils::pathJoin(resourcesPath, "resources/Objects/rotx.obj");
            obj = GuiHelper::readNodeFromFile(file);
            if(!obj)
            {
                LOG_ERROR("Failed to load: %s", file.c_str());
                return;
            }
            rx = obj->cast<vsg::Node>();

            file = utils::pathJoin(resourcesPath, "resources/Objects/roty.obj");
            obj = GuiHelper::readNodeFromFile(file);
            if(!obj)
            {
                LOG_ERROR("Failed to load: %s", file.c_str());
                return;
            }
            ry = obj->cast<vsg::Node>();

            file = utils::pathJoin(resourcesPath, "resources/Objects/rotz.obj");
            obj = GuiHelper::readNodeFromFile(file);
            if(!obj)
            {
                LOG_ERROR("Failed to load: %s", file.c_str());
                return;
            }
            rz = obj->cast<vsg::Node>();

            poseTransform = vsg::MatrixTransform::create(vsg::translate(vsg::dvec3(0, 0, 0)));
            scale = utils::Vector(0.2, 0.2, 0.2);
            scaleTransform = vsg::MatrixTransform::create(vsg::scale(vsg::dvec3(scale.x(), scale.y(), scale.z())));
            this->addChild(poseTransform);
            poseTransform->addChild(scaleTransform);
            if(x) scaleTransform->addChild(x);
            if(y) scaleTransform->addChild(y);
            if(z) scaleTransform->addChild(z);
            if(rx) scaleTransform->addChild(rx);
            if(ry) scaleTransform->addChild(ry);
            if(rz) scaleTransform->addChild(rz);

            // setup colors
            mx = extractMaterialValue(x);
            my = extractMaterialValue(y);
            mz = extractMaterialValue(z);
            mrx = extractMaterialValue(rx);
            mry = extractMaterialValue(ry);
            mrz = extractMaterialValue(rz);
            mx->properties.dataVariance = vsg::DYNAMIC_DATA;
            mode = 0;
            axis = -1;
            active = false;
            setColors();
        }

        bool Manipulator::haveInteraction(std::vector<const vsg::Node*> &nodePath)
        {
            //LOG_ERROR("check for grab interaction");
            if(mode > 0) return true;

            move = 0;
            for (auto node : nodePath)
            {
                if(node == x)
                {
                    move = 1;
                    break;
                }
                if(node == y)
                {
                    move = 2;
                    break;
                }
                if(node == z)
                {
                    move = 3;
                    break;
                }
                if(node == rx)
                {
                    move = 4;
                    break;
                }
                if(node == ry)
                {
                    move = 5;
                    break;
                }
                if(node == rz)
                {
                    move = 6;
                    break;
                }
            }
            if(move > 0)
            {
                if(!active)
                {
                    move = 0;
                }
                rejectMove = 0;
                setColors();
                return true;
            }
            setColors();
            return false;
        }

        void Manipulator::keyPressEvent(vsg::KeyPressEvent& keyPress, bool &active_)
        {
            //fprintf(stderr, "key press: %d", keyPress.keyBase);
            if(keyPress.keyBase == 'g')
            {
                if(mode == 2 && move > 0)
                {
                    applyReject();
                    move -= 3;
                }
                mode = 1;
                keyPress.handled = true;
                setColors();
                return;
            }
            if(keyPress.keyBase == 'q')
            {
                if(mode == 1 && move > 0)
                {
                    applyReject();
                    move += 3;
                }
                mode = 2;
                keyPress.handled = true;
                setColors();
                return;
            }
            if(mode > 0)
            {
                if(keyPress.keyBase == vsg::KEY_Escape)
                {
                    applyReject();
                    move = 0;
                    mode = 0;
                    keyPress.handled = true;
                    setColors();
                    return;
                }
                if(keyPress.keyBase == 'x')
                {
                    if(move != 1+(mode-1)*3)
                    {
                        applyReject();
                    }
                    move = 1+(mode-1)*3;
                    keyPress.handled = true;
                    setColors();
                    return;
                }
                if(keyPress.keyBase == 'y')
                {
                    if(move != 2+(mode-1)*3)
                    {
                        applyReject();
                    }
                    move = 2+(mode-1)*3;
                    keyPress.handled = true;
                    setColors();
                    return;
                }
                if(keyPress.keyBase == 'z')
                {
                    if(move != 3+(mode-1)*3)
                    {
                        applyReject();
                    }
                    move = 3+(mode-1)*3;
                    keyPress.handled = true;
                    setColors();
                    return;
                }
            }
            else if(keyPress.keyBase == vsg::KEY_Escape)
            {
                if(move > 0)
                {
                    applyReject();
                    move = 0;
                    keyPress.handled = true;
                    setColors();
                    return;
                } else
                {
                    active_ = false;
                    keyPress.handled = true;
                    setColors();
                    return;
                }
            }
            if(eventClient->keyPress(keyPress.keyBase))
            {
                keyPress.handled = true;
                return;
            }
        }

        bool Manipulator::pointerClickEvent(int x, int y)
        {
            //if(mode == 0) return false;
            if(mode > 0)
            {
                mode = 0;
                move = 0;
                setColors();
            }
            return true;
        }

        bool Manipulator::pointerMoveEvent(int x, int y)
        {
            if(move == 1)
            {
                rejectMove += -x;
                eventClient->moveX(x);
            } else if(move == 2)
            {
                rejectMove += -x;
                eventClient->moveY(x);
            } else if(move == 3)
            {
                rejectMove += -x;
                eventClient->moveZ(x);
            } else if(move == 4)
            {
                rejectMove += -x;
                eventClient->rotX(x);
            } else if(move == 5)
            {
                rejectMove += -x;
                eventClient->rotY(x);
            } else if(move == 6)
            {
                rejectMove += -x;
                eventClient->rotZ(x);
            }
            return move != 0;
            //LOG_ERROR("pickMoveEvent %d: %4d %4d", move, x, y);
        }

        bool Manipulator::pointerReleaseEvent(int x, int y)
        {
            if(mode == 0)
            {
                move = 0;
                rejectMove = 0;
                setColors();
            }
        }

        void Manipulator::setPose(const utils::Vector &v_,
                           const utils::Quaternion &q_)
        {
            pos = v_;
            rot = q_;
            vsg::dvec3 p(pos.x(), pos.y(), pos.z());
            vsg::dquat q;
            q.x = rot.x();
            q.y = rot.y();
            q.z = rot.z();
            q.w = rot.w();
            poseTransform->matrix = vsg::translate(p) * vsg::rotate(q);
        }

        void Manipulator::setScale(const utils::Vector &s)
        {
            scale = s;
            if(active) scale *= 3.0;
            scaleTransform->matrix = vsg::scale(vsg::dvec3(scale.x(), scale.y(), scale.z()));
        }

        void Manipulator::setActive(bool v)
        {
            if(v && !active)
            {
                active = true;
                setScale(scale);
            } else if(!v && active)
            {
                active = false;
                scale /= 3.0;
                setScale(scale);
            }
            setColors();
        }

        void Manipulator::applyReject()
        {
            if(rejectMove != 0 && move > 0)
            {
                int x = rejectMove;
                if(move == 1)
                {
                    eventClient->moveX(x);
                } else if(move == 2)
                {
                    eventClient->moveY(x);

                } else if(move == 3)
                {
                    eventClient->moveZ(x);
                } else if(move == 4)
                {
                    eventClient->rotX(x);
                } else if(move == 5)
                {
                    eventClient->rotY(x);
                } else if(move == 6)
                {
                    eventClient->rotZ(x);
                }
            }
            rejectMove = 0;
        }

        void Manipulator::setColors()
        {
            if(!active)
            {
                setColor(mx.get(), 1);
                setColor(my.get(), 1);
                setColor(mz.get(), 1);
                setColor(mrx.get(), 1);
                setColor(mry.get(), 1);
                setColor(mrz.get(), 1);
                return;
            }
            mx->value().diffuseFactor = vsg::vec4{1.0, 0.0, 0.0, 1.0};
            mx->value().emissiveFactor = vsg::vec4{0.3, 0.0, 0.0, 1.0};
            my->value().diffuseFactor = vsg::vec4{0.0, 1.0, 0.0, 1.0};
            my->value().emissiveFactor = vsg::vec4{0.0, 0.3, 0.0, 1.0};
            mz->value().diffuseFactor = vsg::vec4{0.0, 0.0, 1.0, 1.0};
            mz->value().emissiveFactor = vsg::vec4{0.0, 0.0, 0.3, 1.0};
            mrx->value().diffuseFactor = vsg::vec4{1.0, 0.0, 0.0, 1.0};
            mrx->value().emissiveFactor = vsg::vec4{0.3, 0.0, 0.0, 1.0};
            mry->value().diffuseFactor = vsg::vec4{0.0, 1.0, 0.0, 1.0};
            mry->value().emissiveFactor = vsg::vec4{0.0, 0.3, 0.0, 1.0};
            mrz->value().diffuseFactor = vsg::vec4{0.0, 0.0, 1.0, 1.0};
            mrz->value().emissiveFactor = vsg::vec4{0.0, 0.0, 0.3, 1.0};
            if(move == 1)
            {
                setColor(mx.get(), 2);
            }
            else if(move == 2)
            {
                setColor(my.get(), 2);
            }
            else if(move == 3)
            {
                setColor(mz.get(), 2);
            }
            else if(move == 4)
            {
                setColor(mrx.get(), 2);
            }
            else if(move == 5)
            {
                setColor(mry.get(), 2);
            }
            else if(move == 6)
            {
                setColor(mrz.get(), 2);
            }
            mx->dirty();
            my->dirty();
            mz->dirty();
            mrx->dirty();
            mry->dirty();
            mrz->dirty();
        }

        void Manipulator::setColor(vsg::PbrMaterialValue *v, int c)
        {
            if(c==1)
            {
                v->value().diffuseFactor = vsg::vec4{0.1, 0.1, 0.1, 1.0};
                v->value().emissiveFactor = vsg::vec4{0.1, 0.1, 0.1, 1.0};
                v->dirty();
                return;
            }
            if(c==2)
            {
                v->value().diffuseFactor = vsg::vec4{1.0, 0.3, 0.0, 1.0};
                v->value().emissiveFactor = vsg::vec4{0.3, 0.1, 0.0, 1.0};
                v->dirty();
                return;
            }
        }
    }
}
