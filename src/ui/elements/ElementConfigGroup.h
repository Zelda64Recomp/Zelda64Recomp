#ifndef RECOMPUI_ELEMENTS_CONFIG_GROUP_H
#define RECOMPUI_ELEMENTS_CONFIG_GROUP_H

#include "common.h"

namespace recompui {

/**
    Base custom element representing a single config option.
    Maps to other custom elements.
 */

class ElementConfigGroup : public Rml::Element {
public:
	ElementConfigGroup(const Rml::String& tag);
	virtual ~ElementConfigGroup();

    recomp::config::ConfigOptionType option_type;
    std::string config_key;
protected:
    void OnAttributeChange(const Rml::ElementAttributes& changed_attributes);
    void AddConfigOptionElement(const nlohmann::json& option_json);
    void SetTextLabel(const std::string& s);
    void ToggleTextLabelVisibility(bool show);
};

}
#endif
