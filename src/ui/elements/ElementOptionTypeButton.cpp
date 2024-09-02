
#include "ElementOptionTypeButton.h"

#include <string>

using json = nlohmann::json;

namespace recompui {

static const std::string button_id = "recomp-button"; 

static const std::string cls_base = "config-option-button"; 
static const std::string cls_button = "button"; 

ElementOptionTypeButton::ElementOptionTypeButton(const Rml::String& tag) : ElementOptionType(tag, cls_base)
{
    Rml::Element *button = AppendChild(GetOwnerDocument()->CreateElement("button"));
    button->SetClass(cls_button, true);
    button->SetId(button_id);
    button->AddEventListener(Rml::EventId::Click, this, false);
}

ElementOptionTypeButton::~ElementOptionTypeButton()
{
    auto button_el = get_button();
    button_el->RemoveEventListener(Rml::EventId::Click, this, false);
}


Rml::Element *ElementOptionTypeButton::get_button()
{
    return GetElementById(button_id);
}

void ElementOptionTypeButton::init_option(std::string& _config_key) {
    config_key = _config_key;

    const json& option_json = recomp::config::get_json_from_key(config_key);

    auto button_el = get_button();

    button_el->SetInnerRML(
        recomp::config::get_config_store_value<std::string>("translations/" + config_key)
    );

    std::string variantClass = recomp::config::get_value_in_json_with_default<std::string>(option_json, "variant", "primary");
    button_el->SetClass(cls_button + "--" + variantClass, true);
}

void ElementOptionTypeButton::ProcessEvent(Rml::Event& event)
{
	if (event == Rml::EventId::Click)
	{
		if (event.GetPhase() == Rml::EventPhase::Bubble || event.GetPhase() == Rml::EventPhase::Target)
		{
            printf("Button clicked\n");
		}
	}
}

} // namespace Rml
