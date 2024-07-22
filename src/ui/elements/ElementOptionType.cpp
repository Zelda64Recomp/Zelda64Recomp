#include "ElementOptionType.h"

namespace recompui {

ElementOptionType::ElementOptionType(const Rml::String& tag, const std::string& base_class) : Rml::Element(tag)
{
    SetAttribute("recomp-store-element", true);
    SetClass(base_class, true);
}

ElementOptionType::~ElementOptionType()
{

}

} // namespace Rml
