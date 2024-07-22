#ifndef RECOMPUI_ELEMENT_OPTION_TYPE_DROPDOWN_H
#define RECOMPUI_ELEMENT_OPTION_TYPE_DROPDOWN_H

#include "common.h"
#include "ElementOptionType.h"

namespace recompui {

class ElementOptionTypeDropdown : public ElementOptionType {
public:
	ElementOptionTypeDropdown(const Rml::String& tag);
	virtual ~ElementOptionTypeDropdown();

    void init_option(std::string& _config_key);
protected:
	void ProcessEvent(Rml::Event& event) override;

private:
	void set_cur_option(int opt);
	Rml::ElementFormControlSelect *get_select();
};

} // namespace recompui
#endif
