
#include "ElementOptionTypeTextField.h"

#include <string>

using json = nlohmann::json;

namespace recompui {

static const std::string input_id = "recomp-textfield"; 

static const std::string cls_base = "config-option-textfield"; 
static const std::string cls_wrapper = cls_base + "__wrapper"; 
static const std::string cls_input = cls_base + "__input"; 

ElementOptionTypeTextField::ElementOptionTypeTextField(const Rml::String& tag) : ElementOptionType(tag, cls_base)
{
    Rml::Element *wrapper = AppendChild(GetOwnerDocument()->CreateElement("label"));
    wrapper->SetClass(cls_wrapper, true);
    Rml::ElementFormControlSelect *input_el = (Rml::ElementFormControlSelect *)wrapper->AppendChild(GetOwnerDocument()->CreateElement("input"));
    input_el->SetId(input_id);
    input_el->SetValue("");
    input_el->AddEventListener(Rml::EventId::Change, this, false);
    input_el->SetClass(cls_input, true);
}

Rml::ElementFormControlInput *ElementOptionTypeTextField::get_input()
{
    return (Rml::ElementFormControlInput *)GetElementById(input_id);
}

ElementOptionTypeTextField::~ElementOptionTypeTextField()
{
    auto input_el = get_input();
    RemoveEventListener(Rml::EventId::Change, this, false);
}

void ElementOptionTypeTextField::init_option(std::string& _config_key) {
    config_key = _config_key;

    const json& option_json = recomp::config::get_json_from_key(config_key);;
    auto val = recomp::config::get_config_store_value<std::string>(config_key);

    auto input_el = get_input();
    input_el->SetValue(val);

    const auto maxlength = recomp::config::get_value_in_json<int>(option_json, "maxlength");
    input_el->SetAttribute("maxlength", std::to_string(maxlength));

    // RMLUI sadly doesn't support placeholders yet, so this will be a _placeholder_ until thats ready.
    input_el->SetAttribute("placeholder", recomp::config::get_config_store_value_with_default<std::string>("translations/" + config_key + ":placeholder", ""));
}

void ElementOptionTypeTextField::ProcessEvent(Rml::Event& event)
{
	if (event == Rml::EventId::Change)
	{
		if (event.GetPhase() == Rml::EventPhase::Bubble || event.GetPhase() == Rml::EventPhase::Target)
		{
            Rml::Element *target = event.GetTargetElement();
            auto val_variant = target->GetAttribute("value");
            recomp::config::set_config_store_value(config_key, val_variant->Get<std::string>());
		}
	}
}

} // namespace Rml
