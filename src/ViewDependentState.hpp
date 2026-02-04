#pragma once

#include "gui_helper_functions.hpp"
#include <vsg/all.h>

namespace mars
{
    namespace vsg_graphics
    {
        class ViewDependentState :  public vsg::Inherit<vsg::ViewDependentState, ViewDependentState>
        {
        public:
            explicit ViewDependentState(vsg::View* in_view);
            void init(vsg::ResourceRequirements& requirements) override;
            void traverse(vsg::RecordTraversal& rt) const override;
            vsg::ref_ptr<WorldTransformUniformValue> worldTransformUniform;
        };
    }
}
