#ifndef RECOMPUI_ELEMENT_OPTION_TYPE_RANGE_H
#define RECOMPUI_ELEMENT_OPTION_TYPE_RANGE_H

#include "common.h"

namespace recompui {

class ElementOptionTypeRange : public Rml::Element, public Rml::EventListener {
public:
	ElementOptionTypeRange(const Rml::String& tag);
	virtual ~ElementOptionTypeRange();

    std::string config_key;
    std::string suffix;

    void init_option(std::string& _config_key);
protected:
	void ProcessEvent(Rml::Event& event) override;

private:
	void set_cur_option(int opt);
	void set_value_label(int value);
};

} // namespace recompui
#endif
