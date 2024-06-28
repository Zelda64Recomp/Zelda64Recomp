#ifndef RECOMPUI_ELEMENTS_CONFIG_OPTION_H
#define RECOMPUI_ELEMENTS_CONFIG_OPTION_H

#include "RmlUi/Core/Element.h"
#include "../config_options/ConfigOption.h"
#include "RmlUi/Core/EventListener.h"

namespace recompui {

/**
    Base custom element representing a single config option.
    Maps to other custom elements.
 */

class ElementConfigOption : public Rml::Element, public Rml::EventListener {
public:
	ElementConfigOption(const Rml::String& tag);
	virtual ~ElementConfigOption();

    ConfigOptionType option_type;
    std::string config_key;
    bool in_checkbox_group = false;
    
protected:
    void OnAttributeChange(const Rml::ElementAttributes& changed_attributes);
    void SetTextLabel(const std::string& s);
    void AddOptionTypeElement();
    Rml::Element *GetOptionTypeWrapper();
    void ProcessEvent(Rml::Event& event) override;
};

}
#endif
