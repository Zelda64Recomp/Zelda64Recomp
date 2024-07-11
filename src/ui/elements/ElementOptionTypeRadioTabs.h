#ifndef RECOMPUI_ELEMENT_OPTION_TYPE_RADIO_TABS_H
#define RECOMPUI_ELEMENT_OPTION_TYPE_RADIO_TABS_H

#include "common.h"

namespace recompui {

class ElementOptionTypeRadioTabs : public Rml::Element, public Rml::EventListener {
public:
	ElementOptionTypeRadioTabs(const Rml::String& tag);
	virtual ~ElementOptionTypeRadioTabs();

    std::string config_key;
    void init_option(std::string& _config_key);
protected:
	void ProcessEvent(Rml::Event& event) override;

private:
	void set_cur_option(int opt);
};

} // namespace recompui
#endif
