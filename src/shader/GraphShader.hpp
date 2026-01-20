#pragma once
#include "ShaderTypes.hpp"

#include <mars_utils/Vector.h>
#include <mars_utils/Quaternion.h>
#include <configmaps/ConfigData.h>

#include <vsg/all.h>

#include <ostream>

namespace mars
{
    namespace vsg_graphics
    {
        class GraphShader
        {
         public:
            GraphShader();
            ~GraphShader();

            void loadShader(configmaps::ConfigMap &vertexShader);
            void parseFunctionInfo(std::string functionName,
                                   configmaps::ConfigMap functionInfo);
            std::string generateDefinitions();
            std::string generateVertexHeader();
            std::string generateFragmentHeader();
            std::string generateVertexShaderSource();
            std::string generateFragmentShaderSource();

            configmaps::ConfigMap options;
            std::string main_source;
            std::map<std::string, std::string> source_files;
            std::set<ShaderUniformT> uniforms;
            std::set<ShaderAttributeT> varyings;
            std::set<ShaderAttributeT> attributes;
        };
    }
}
