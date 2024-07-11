#ifndef RECOMPUI_ELEMENTS_CONFIG_OPTION_H
#define RECOMPUI_ELEMENTS_CONFIG_OPTION_H

#include "common.h"

namespace recompui {

/**
    Base custom element representing a single config option.
    Maps to other custom elements.
 */

class ElementConfigOption : public Rml::Element, public Rml::EventListener {
public:
	ElementConfigOption(const Rml::String& tag);
	virtual ~ElementConfigOption();

    recomp::config::ConfigOptionType option_type;
    std::string config_key;
    bool in_checkbox_group = false;
    
protected:
    void OnAttributeChange(const Rml::ElementAttributes& changed_attributes);
    Rml::Element *GetLabel(void);
    void SetTextLabel(const std::string& s);
    void AddOptionTypeElement();
    Rml::Element *GetOptionTypeWrapper();
    void ProcessEvent(Rml::Event& event) override;
};

}
#endif
