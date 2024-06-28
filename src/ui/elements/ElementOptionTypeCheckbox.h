#ifndef RECOMPUI_ELEMENT_OPTION_TYPE_CHECKBOX_H
#define RECOMPUI_ELEMENT_OPTION_TYPE_CHECKBOX_H

#include "RmlUi/Core/Element.h"
#include "RmlUi/Core/Elements/ElementFormControlInput.h"
#include "RmlUi/Core/EventListener.h"
#include "../config_options/ConfigOption.h"

namespace recompui {

class ElementOptionTypeCheckbox : public Rml::Element, public Rml::EventListener {
public:
	ElementOptionTypeCheckbox(const Rml::String& tag);
	virtual ~ElementOptionTypeCheckbox();

    void init_option(std::string& _config_key);

    std::string config_key;

	const ConfigOptionType option_type = ConfigOptionType::Checkbox;
protected:
    Rml::ElementFormControlInput *get_input();

	void ProcessEvent(Rml::Event& event) override;

private:
	Element* GetTarget();
	void set_checked(bool checked);

    bool disable_click = false;
};

} // namespace recompui
#endif
