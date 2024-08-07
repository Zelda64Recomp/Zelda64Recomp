#ifndef RECOMPUI_ELEMENT_OPTION_TYPE_TEXTFIELD_H
#define RECOMPUI_ELEMENT_OPTION_TYPE_TEXTFIELD_H

#include "common.h"
#include "ElementOptionType.h"

namespace recompui {

class ElementOptionTypeTextField : public ElementOptionType {
public:
	ElementOptionTypeTextField(const Rml::String& tag);
	virtual ~ElementOptionTypeTextField();

    void init_option(std::string& _config_key);
protected:
	void ProcessEvent(Rml::Event& event) override;
private:
	Rml::ElementFormControlInput* get_input();
};

} // namespace recompui
#endif
