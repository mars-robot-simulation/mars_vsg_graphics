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
            std::string generateUniforms();
            std::string generateDefinitions();
            std::string generateVertexHeader();
            std::string generateFragmentHeader();
            std::string generateVertexShaderSource();
            std::string generateFragmentShaderSource();
            void setBinding(std::string uniformName, int set, int binding);

            configmaps::ConfigMap options;
            std::string main_source;
            std::map<std::string, std::string> source_files;
            std::map<std::string, ShaderUniformT> uniforms;
            std::map<std::string, ShaderAttributeT> varyings;
            std::map<std::string, ShaderAttributeT> attributes;
            bool debugOutput;
        };
    }
}
