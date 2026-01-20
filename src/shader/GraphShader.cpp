#include "GraphShader.hpp"
#include "tsort/tsort.h"
#include "../gui_helper_functions.hpp"
#include <mars_utils/misc.h>

using namespace std;
using namespace configmaps;
using namespace mars::utils;

namespace mars
{
    namespace vsg_graphics
    {

        const auto default_vert = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstants {
    mat4 projection;
    mat4 modelView;
} pc;

layout(set = 0, binding = 0) uniform WorldTransform{
    mat4 projectionInverse;
    mat4 viewInverse;
} wt;

layout(location = 0) in vec3 vsg_Vertex;
layout(location = 1) in vec3 vsg_Normal;

out gl_PerVertex{ vec4 gl_Position; };
)";

        const auto default_frag = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstants {
    mat4 projection;
    mat4 modelView;
} pc;

layout(set = 0, binding = 0) uniform WorldTransform{
    mat4 projectionInverse;
    mat4 viewInverse;
} wt;

layout(set = 0, binding = 1) uniform PbrMaterial
{
    vec4 baseColorFactor;
    vec4 emissiveFactor;
    vec4 diffuseFactor;
    vec4 specularFactor;
    float metallicFactor;
    float roughnessFactor;
    float alphaMask;
    float alphaMaskCutoff;
} pbr;

// ViewDependentState
layout(constant_id = 3) const int lightDataSize = 256;
layout(set = 1, binding = 0) uniform LightData
{
    vec4 values[lightDataSize];
} lightData;

)";

        GraphShader::GraphShader() {}
        GraphShader::~GraphShader() {}

        void GraphShader::loadShader(ConfigMap &shaderConfig)
        {
            stringstream code;
            std::map<unsigned long, ConfigMap> nodeMap;
            std::vector<ConfigMap *> sortedNodes;
            std::map<std::string, unsigned long> nodeNameId;
            std::map<unsigned long, ConfigMap>::iterator nodeIt;
            std::vector<ConfigMap *>::iterator sNodeIt;
            std::vector<std::string> add; // lines to add after function call generation
            std::vector<ShaderAttributeT> vars; //definitions of main variables
            std::vector<ShaderVariableT> defaultVars;
            std::vector<std::string> function_calls;
            ConfigItem graph = shaderConfig["versions"][0]["components"];
            ConfigMap nodeConfig;
            ConfigMap iOuts; //Map containing already created edge variables for output interfaces
            ConfigMap filterMap;
            filterMap["int"] = 1;
            filterMap["float"] = 1;
            filterMap["vec2"] = 1;
            filterMap["vec3"] = 1;
            filterMap["vec4"] = 1;
            filterMap["sampler2D"] = 1;
            filterMap["samplerCube"] = 1;
            filterMap["outColor"] = 1;

            ConfigVector::iterator it;
            ConfigMap::iterator mit;

            // Making default values of nodes easily accessible
            for (it = graph["configuration"]["nodes"].begin(); it != graph["configuration"]["nodes"].end(); ++it)
            {
                std::string name = replaceString((std::string)(*it)["name"], "::", "_");
                ConfigMap data;
                if((*it)["data"].isMap())
                {
                    data = (*it)["data"];
                }
                else
                {
                    data = ConfigMap::fromYamlString((*it)["data"].getString());
                }
                nodeConfig[name] = data["data"];
            }

            // create node ids for tsort
            unsigned long id = 1;
            for(it = graph["nodes"].begin(); it != graph["nodes"].end(); ++it)
            {
                string function = (*it)["model"]["name"];
                std::string name = replaceString((std::string)(*it)["name"], "::", "_");
                // for backwards compatibility replace gl_Vertex by new vsg_Vertex
                // todo: check if we need other replacements and document them all
                if(name == "gl_Vertex") name = "vsg_Vertex";
                (*it)["name"] = name;
                nodeMap[id] = (*it);
                nodeNameId[name] = id++;
                if(nodeConfig.hasKey(name) && nodeConfig[name].hasKey("type"))
                {
                    string type = nodeConfig[name]["type"].getString();
                    if(nodeConfig[name].hasKey("loadName"))
                    {
                        function << nodeConfig[name]["loadName"];
                    }
                    //todo: check how to deal with uniforms
                    //      if we want to be generic we might need a uniform-manager
                    if (type == "uniform")
                    {
                        if(!options.hasKey(name))
                        {
                            uniforms.insert((ShaderUniformT) {function, name});
                        }
                    } else if (type == "varying")
                    {
                        varyings.insert((ShaderAttributeT) {function, name});
                    }
                }
            }

            for (it = graph["edges"].begin(); it != graph["edges"].end(); ++it)
            {
                string fromNodeName = replaceString((std::string)(*it)["from"]["name"], "::", "_");
                string toNodeName = replaceString((std::string)(*it)["to"]["name"], "::", "_");
                string fromInterfaceName = (*it)["from"]["interface"];
                string toInterface = (*it)["to"]["interface"];
                string fromVar = "";
                // for backwards compatibility replace gl_Vertex by new vsg_Vertex
                if(fromNodeName == "gl_Vertex") fromNodeName = "vsg_Vertex";

                // create relations for tsort
                if(nodeNameId.count(fromNodeName) > 0 && nodeNameId.count(toNodeName) > 0)
                {
                    add_relation(nodeNameId[fromNodeName], nodeNameId[toNodeName]);
                }
                ConfigMap fromNode = nodeMap[nodeNameId[fromNodeName]];
                string name = fromNode["model"]["name"];
                string function = name;
                if(nodeConfig[fromNodeName].hasKey("loadName"))
                {
                    function << nodeConfig[name]["loadName"];
                }
                if (filterMap.hasKey(function))
                {
                    fromVar = fromNodeName;
                } else
                {
                    fromVar = fromNodeName + "_at_" + fromInterfaceName;
                }
                // Create Mappings from mainVar names to node input interfaces
                nodeConfig[toNodeName]["toParams"][toInterface] = fromVar;
                ConfigMap toNode = nodeMap[nodeNameId[toNodeName]];
                if (filterMap.hasKey(toNode["model"]["name"]) && nodeConfig[toNodeName].hasKey("type"))
                {
                    if ( nodeConfig[toNodeName]["type"].getString() == "varying")
                    {
                        // We have to output into a varying
                        add.push_back(toNodeName + " = " + fromVar + ";\n");
                    }
                }
            }

            tsort();
            unsigned long *ids = get_sorted_ids();
            while (*ids != 0)
            {
                sortedNodes.push_back(&(nodeMap[*ids]));
                ids++;
            }
            for (nodeIt = nodeMap.begin(); nodeIt != nodeMap.end(); ++nodeIt)
            {
                if (find(sortedNodes.begin(), sortedNodes.end(), &(nodeIt->second)) == sortedNodes.end())
                {
                    sortedNodes.push_back(&(nodeIt->second));
                }
            }

            for (sNodeIt = sortedNodes.begin(); sNodeIt != sortedNodes.end(); ++sNodeIt)
            {
                ConfigMap &node = **sNodeIt;
                string nodeFunction = node["model"]["name"];
                string nodeName = node["name"];
                priority_queue<PrioritizedLine> incoming, outgoing;
                bool first = true;
                if (nodeConfig.hasKey(nodeName))
                {
                    if(nodeConfig[nodeName].hasKey("loadName"))
                    {
                        nodeFunction << nodeConfig[nodeName]["loadName"];
                    }
                }
                if (!filterMap.hasKey(nodeFunction))
                {
                    ConfigMap functionInfo;
                    try
                    {
                        functionInfo = ConfigMap::fromYamlFile(GuiHelper::resourcePath + "/resources/graph_shader/" + nodeFunction + ".yaml");
                    }
                    catch (...)
                    {
                        fprintf(stderr, "ERROR: \n%s\n", node.toYamlString().c_str());
                        throw std::runtime_error("load shader error");
                    }
                    // could handle shadow_map samples here

                    parseFunctionInfo(nodeFunction, functionInfo);
                    stringstream call;
                    call.clear();
                    call << nodeFunction << "(";
                    if (functionInfo["params"].hasKey("out"))
                    {
                        for (mit = functionInfo["params"]["out"].beginMap(); mit != functionInfo["params"]["out"].endMap(); mit++)
                        {
                            string varName = nodeName + "_at_" + mit->first;
                            string varType = (mit->second)["type"];
                            outgoing.push((PrioritizedLine) {varName, (int) functionInfo["params"]["out"][mit->first]["index"], 0});
                            vars.push_back((ShaderAttributeT) {varType, varName}); // Add main var
                        }
                    }
                    if (functionInfo["params"].hasKey("in"))
                    {
                        for (mit = functionInfo["params"]["in"].beginMap(); mit != functionInfo["params"]["in"].endMap(); mit++)
                        {
                            string varName = "";
                            if (nodeConfig[nodeName]["toParams"].hasKey(mit->first))
                            {
                                varName = nodeConfig[nodeName]["toParams"][mit->first].getString();
                            } else if (nodeConfig[nodeName].hasKey("inputs") && nodeConfig[nodeName]["inputs"].hasKey(mit->first))
                            {
                                varName = "default_" + mit->first + "_for_" + nodeName;
                                string varType = (mit->second)["type"];
                                defaultVars.push_back(ShaderVariableT {varType, varName, nodeConfig[nodeName]["inputs"][mit->first]});
                            }
                            else
                            {
                                varName = "default_" + mit->first + "_for_" + nodeName;
                                string varType = (mit->second)["type"].toString();
                                string value = "1.0";
                                if(varType == "vec2")
                                {
                                    value = "vec2(0, 0)";
                                }
                                if(varType == "vec3")
                                {
                                    value = "vec2(0, 0, 0)";
                                }
                                if(varType == "vec4")
                                {
                                    value = "vec4(0, 0, 0, 1)";
                                }
                                else
                                {
                                    fprintf(stderr, "Shader ERROR: input not connected: node %s input %lu\n", nodeName.c_str(), incoming.size());
                                }
                                defaultVars.push_back(ShaderVariableT {varType, varName, value});
                            }
                            incoming.push((PrioritizedLine) {varName, (int) functionInfo["params"]["in"][mit->first]["index"], 0});
                        }
                    }
                    while (!incoming.empty())
                    {
                        if (!first)
                        {
                            call << ", ";
                        }
                        first = false;
                        call << incoming.top().line;
                        incoming.pop();
                    }
                    while (!outgoing.empty())
                    {
                        if (!first)
                        {
                            call << ", ";
                        }
                        first = false;
                        call << outgoing.top().line;
                        outgoing.pop();
                    }
                    call << ");" << endl;
                    function_calls.push_back(call.str());
                }
            }

            // Compose code
            code << "void main() {" << endl;
            for (size_t i = 0; i < vars.size(); ++i)
            {
                code << "  " << vars[i] << ";" << endl;
            }
            code << endl;
            for (size_t i = 0; i < defaultVars.size(); ++i)
            {
                code << "  " << defaultVars[i] << ";" << endl;
            }
            code << endl;
            for (size_t i = 0; i < function_calls.size(); ++i)
            {
                code << "  " << function_calls[i] << endl;
            }
            for (size_t i = 0; i < add.size(); ++i)
            {
                code << "  " << add[i];
            }
            code << "}" << endl;
            main_source = code.str();
        }

        void GraphShader::parseFunctionInfo(string functionName, ConfigMap functionInfo)
        {
            ConfigMap::iterator mit;
            ConfigVector::iterator vit;
            if (functionInfo.hasKey("source") && source_files.count(functionName) == 0)
            {
                source_files[functionName] = functionInfo["source"].getString();
            }
            // todo: check minVersion and extension handling for Vulkan
            // if (functionInfo.hasKey("minVersion"))
            // {
            //     int version = functionInfo["minVersion"];
            //     if (version > minVersion)
            //     {
            //         minVersion = version;
            //     }
            // }

            // if (functionInfo.hasKey("extensions"))
            // {
            //     ConfigItem list = functionInfo["extensions"];
            //     if (list.hasKey("enabled"))
            //     {
            //         std::cout << "Having enabled extensions" << std::endl;
            //         for (vit=list["enabled"].begin();vit!=list["enabled"].end();vit++)
            //         {
            //             enabledExtensions.insert(vit.base()->getString());
            //         }
            //     }
            //     if (list.hasKey("disabled"))
            //     {
            //         for (vit=list["disabled"].begin();vit!=list["disabled"].end();vit++)
            //         {
            //             disabledExtensions.insert(vit.base()->getString());
            //         }
            //     }
            // }

            if (functionInfo.hasKey("uniforms"))
            {
                ConfigItem list = functionInfo["uniforms"];
                for (mit = list.beginMap(); mit != list.endMap(); ++mit)
                {
                    string type = mit->first;
                    string type_name = "";
                    std::size_t a_pos = type.find("[]");
                    bool is_array = false;
                    if (a_pos != string::npos)
                    {
                        type_name = type.substr(0, a_pos);
                        is_array = true;
                    } else
                    {
                        type_name = type;
                    }
                    for (vit = list[type].begin(); vit != list[type].end(); ++vit)
                    {
                        ConfigItem &item = *vit;
                        string name = item["name"].getString();
                        if (is_array)
                        {
                            stringstream s;
                            int num = 1;
                            // if (item.hasKey("arraySize"))
                            // {
                            //     num = (int) options[item["arraySize"].getString()];
                            // }
                            s << "[" << num << "]";
                            name.append(s.str());
                        }
                        if(!options.hasKey(name))
                        {
                            uniforms.insert((ShaderUniformT) {type_name, name});
                        }
                    }
                }
            }

            if (functionInfo.hasKey("varyings"))
            {
                ConfigItem list = functionInfo["varyings"];
                for (mit = list.beginMap(); mit != list.endMap(); ++mit)
                {
                    string type = mit->first;
                    string type_name = "";
                    std::size_t a_pos = type.find("[]");
                    bool is_array = false;
                    if (a_pos != string::npos)
                    {
                        type_name = type.substr(0, a_pos);
                        is_array = true;
                    } else
                    {
                        type_name = type;
                    }
                    for (vit = list[type].begin(); vit != list[type].end(); ++vit)
                    {
                        ConfigItem &item = *vit;
                        string name = item["name"].getString();
                        if (is_array)
                        {
                            stringstream s;
                            int num = 1;
                            // if (item.hasKey("arraySize"))
                            // {
                            //     num = (int) options[item["arraySize"].getString()];
                            // }
                            s << "[" << num << "]";
                            name.append(s.str());
                        }
                        varyings.insert((ShaderAttributeT) {type_name, name});
                    }
                }
            }

            if (functionInfo.hasKey("attributes"))
            {
                ConfigItem list = functionInfo["attributes"];
                for (mit = list.beginMap(); mit != list.endMap(); ++mit)
                {
                    string type = mit->first;
                    string type_name = "";
                    std::size_t a_pos = type.find("[]");
                    bool is_array = false;
                    if (a_pos != string::npos)
                    {
                        type_name = type.substr(0, a_pos);
                        is_array = true;
                    } else
                    {
                        type_name = type;
                    }
                    for (vit = list[type].begin(); vit != list[type].end(); ++vit)
                    {
                        ConfigItem &item = *vit;
                        string name = item["name"].getString();
                        if (is_array)
                        {
                            stringstream s;
                            int num = 1;
                            // if (item.hasKey("arraySize"))
                            // {
                            //     num = (int) options[item["arraySize"].getString()];
                            // }
                            s << "[" << num << "]";
                            name.append(s.str());
                        }
                        attributes.insert((ShaderAttributeT) {type_name, name});
                    }
                }
            }
        }

        std::string GraphShader::generateDefinitions()
        {
            stringstream code;
            map<string, string>::iterator mit;
            string resPath = utils::pathJoin(GuiHelper::resourcePath, "resources");
            for (mit = source_files.begin(); mit != source_files.end(); ++mit)
            {
                string path = utils::pathJoin(resPath, (string) mit->second);
                //fprintf(stderr, "path: %s\n", path.c_str());
                ifstream t(path);
                stringstream buffer;
                buffer << t.rdbuf();
                code << endl << buffer.str() << endl;
            }
            return code.str();
        }

        std::string GraphShader::generateVertexHeader()
        {
            stringstream code;
            code << default_vert;
            int outIndex = 0;
            // varyings is the data transfered from one shader to the other
            // e. g. from vertex to fragment
            // todo: check if we have to distinguish between in and out varyings
            for(auto v: varyings)
            {
                code << "layout(location = " << outIndex++ << ") out " << v << ";" << std::endl;
            }
            return code.str();
        }

        std::string GraphShader::generateFragmentHeader()
        {
            stringstream code;
            code << default_frag;
            int inIndex = 0;
            // varyings is the data transfered from one shader to the other
            // e. g. from vertex to fragment
            // todo: check if we have to distinguish between in and out varyings
            for(auto v: varyings)
            {
                code << "layout(location = " << inIndex++ << ") in " << v << ";" << std::endl;
            }
            code << "layout(location = 0) out vec4 outColor;" << endl;
            return code.str();
        }

        std::string GraphShader::generateVertexShaderSource()
        {
            stringstream code;
            code << generateVertexHeader() << endl;
            code << generateDefinitions() << endl;
            code << main_source << endl;
            return code.str();
        }

        std::string GraphShader::generateFragmentShaderSource()
        {
            stringstream code;
            code << generateFragmentHeader() << endl;
            code << generateDefinitions() << endl;
            code << main_source << endl;
            return code.str();
        }
    }
}
