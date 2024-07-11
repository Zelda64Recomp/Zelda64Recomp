#ifndef RECOMPUI_ELEMENT_OPTION_TYPE_CHECKBOX_H
#define RECOMPUI_ELEMENT_OPTION_TYPE_CHECKBOX_H

#include "common.h"

namespace recompui {

class ElementOptionTypeCheckbox : public Rml::Element, public Rml::EventListener {
public:
	ElementOptionTypeCheckbox(const Rml::String& tag);
	virtual ~ElementOptionTypeCheckbox();

    void init_option(std::string& _config_key);

    std::string config_key;

	const recomp::config::ConfigOptionType option_type = recomp::config::ConfigOptionType::Checkbox;
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
