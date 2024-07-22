#ifndef RECOMPUI_ELEMENT_OPTION_TYPE_H
#define RECOMPUI_ELEMENT_OPTION_TYPE_H

#include "common.h"

// base class for ElementOptionTypes. Already set up as an event listener,
// and contains required data. Initialization sets an important flag.

namespace recompui {

class ElementOptionType : public Rml::Element, public Rml::EventListener {
public:
	ElementOptionType(const Rml::String& tag, const std::string& base_class);
	virtual ~ElementOptionType();

    std::string config_key;
};

} // namespace recompui
#endif
