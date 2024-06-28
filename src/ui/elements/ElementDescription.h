#ifndef RECOMPUI_ELEMENT_DESCRIPTION_H
#define RECOMPUI_ELEMENT_DESCRIPTION_H

#include "RmlUi/Core/Element.h"
#include "../config_options/ConfigOption.h"

namespace recompui {

class ElementDescription : public Rml::Element {
public:
	ElementDescription(const Rml::String& tag);
	virtual ~ElementDescription();

    std::string config_key;
protected:
	void update_text(const std::string& text_rml);
private:
	void OnAttributeChange(const Rml::ElementAttributes& changed_attributes);
};

} // namespace recompui
#endif
