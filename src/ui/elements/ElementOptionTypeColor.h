#ifndef RECOMPUI_ELEMENT_OPTION_TYPE_COLOR_H
#define RECOMPUI_ELEMENT_OPTION_TYPE_COLOR_H

#include "common.h"

namespace recompui {

class ElementOptionTypeColor : public Rml::Element, public Rml::EventListener {
public:
	ElementOptionTypeColor(const Rml::String& tag);
	virtual ~ElementOptionTypeColor();

    std::string config_key;
	HsvColorF hsv;

    void init_option(std::string& _config_key);
protected:
	void set_value_label(int hsvIndex);
	void set_config_store_rgb();
	void set_preview_block_rgb(RgbColor rgb);

	void ProcessEvent(Rml::Event& event) override;
};

} // namespace recompui
#endif
