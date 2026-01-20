#pragma once

#include <ostream>

namespace mars
{
    namespace vsg_graphics
    {

        struct ShaderAttributeT
        {
            std::string type, name;
        };

        typedef ShaderAttributeT ShaderVaryingT;
        typedef ShaderAttributeT ShaderUniformT;
        typedef ShaderAttributeT ShaderSuffixT;

        struct ShaderVariableT
        {
            std::string type, name, value;
        };

        typedef ShaderVariableT ShaderConstantT;

        struct PrioritizedValue
        {
            int priority;
            int s_priority; // used to preserve order in case of same priority
            bool operator<(const PrioritizedValue& other) const
                {
                    if (priority == other.priority)
                    {
                        return s_priority > other.s_priority;
                    }
                    return priority > other.priority;
                };
        };

        struct FunctionCall : PrioritizedValue
        {
            std::string name;
            std::vector<std::string> arguments;
            FunctionCall(std::string name, std::vector<std::string> args, int prio, int s_prio) : name(name), arguments(args)
                {
                    (void)s_prio;
                    priority = prio;
                }
            std::string toString() const
                {
                    std::string call = name + "( ";
                    unsigned long numArgs = arguments.size();

                    if(numArgs > 0)
                    {
                        call += arguments[0];
                        for(unsigned long i=1; i<numArgs; ++i)
                        {
                            call += ", " + arguments[i];
                        }
                    }
                    call += " )";
                    return call;
                }
        };

        typedef struct MainVar : PrioritizedValue, ShaderVariableT
        {
            MainVar(std::string p_name, std::string p_type, std::string p_value, int p_priority, int p_s_priority)
                {
                    priority = p_priority;
                    s_priority = p_s_priority;
                    name = p_name;
                    value = p_value;
                    type = p_type;
                }
            std::string toString() const
                {
                    return name + " = " + value;
                }
        } MainVar;

        struct PrioritizedLine : PrioritizedValue
        {
            std::string line;
            PrioritizedLine(std::string p_line, int p_priority, int p_s_priority) : line(p_line)
                {
                    priority = p_priority;
                    s_priority = p_s_priority;
                }
        };

        std::ostream& operator<<(std::ostream& os, const ShaderAttributeT& a);
        //std::ostream& operator<<(std::ostream& os, const ShaderExportT& a);
        std::ostream& operator<<(std::ostream& os, const ShaderVariableT& a);
        bool operator<(const ShaderAttributeT& a, const ShaderAttributeT& b);
        //bool operator<(const ShaderExportT& a, const ShaderExporTt& b);
        bool operator<(const ShaderVariableT& a, const ShaderVariableT& b);
    }
}
