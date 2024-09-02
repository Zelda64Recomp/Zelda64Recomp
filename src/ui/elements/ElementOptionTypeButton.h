#ifndef RECOMPUI_ELEMENT_OPTION_TYPE_BUTTON_H
#define RECOMPUI_ELEMENT_OPTION_TYPE_BUTTON_H

#include "common.h"
#include "ElementOptionType.h"

namespace recompui {

class ElementOptionTypeButton : public ElementOptionType {
public:
	ElementOptionTypeButton(const Rml::String& tag);
	virtual ~ElementOptionTypeButton();

    void init_option(std::string& _config_key);
protected:
	void ProcessEvent(Rml::Event& event) override;
private:
	Rml::Element* get_button();
};

} // namespace recompui
#endif
