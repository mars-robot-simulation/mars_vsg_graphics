#include "ShaderTypes.hpp"
namespace mars
{
    namespace vsg_graphics
    {
        std::ostream& operator<<(std::ostream& os, const ShaderAttributeT& a)
        {
            return os << a.type << " " << a.name;
        }

        std::ostream& operator<<(std::ostream& os, const ShaderVariableT& a)
        {
            return os << a.type << " " << a.name << " = " << a.value;
        }

        bool operator<(const ShaderAttributeT& a, const ShaderAttributeT& b)
        {
            return a.name < b.name;
        }

        bool operator<(const ShaderVariableT& a, const ShaderVariableT& b)
        {
            return a.name < b.name;
        }
    }
}
