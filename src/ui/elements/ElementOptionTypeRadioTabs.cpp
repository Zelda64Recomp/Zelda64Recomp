
#include "ElementOptionTypeRadioTabs.h"

#include <string>

using json = nlohmann::json;

namespace recompui {

static const std::string radio_input_id = "recomp-radio__"; 

ElementOptionTypeRadioTabs::ElementOptionTypeRadioTabs(const Rml::String& tag) : Rml::Element(tag)
{
    SetAttribute("recomp-store-element", true);
    SetClass("config-option__radio-tabs", true);
}

ElementOptionTypeRadioTabs::~ElementOptionTypeRadioTabs()
{
    Rml::ElementList elements;
    GetElementsByTagName(elements, "input");
    for (int i = 0; i < elements.size(); i++) {
        Rml::Element *el = elements[i];
        el->RemoveEventListener(Rml::EventId::Click, this, false);
    }
}

void ElementOptionTypeRadioTabs::set_cur_option(int opt) {
    Rml::ElementList elements;
    GetElementsByTagName(elements, "input");
    for (int i = 0; i < elements.size(); i++) {
        Rml::Element *el = elements[i];
        if (i == opt) {
            SetAttribute("checked", true);
            el->SetAttribute("checked", true);
        } else {
            RemoveAttribute("checked");
            el->RemoveAttribute("checked");
        }
    }

    DirtyLayout();
}

void ElementOptionTypeRadioTabs::init_option(std::string& _config_key) {
    config_key = _config_key;
    const json& option_json = recomp::config::get_json_from_key(config_key);
    int opt = recomp::config::get_config_store_value<int>(config_key);
    const json& opt_array = option_json["values"];

    for (int i = 0; i < opt_array.size(); i++) {
        const auto &j_opt = opt_array[i];
        const std::string opt_val = j_opt.get<std::string>();
        const std::string opt_id = radio_input_id + config_key + "--" + opt_val;
        
        const std::string translation_key = "translations/" + config_key + "/values/" + opt_val;
        const std::string& opt_text = recomp::config::get_config_store_value<std::string>(translation_key);

        Rml::Element *radio_el = AppendChild(GetOwnerDocument()->CreateElement("input"));

        radio_el->SetId(opt_id);
        radio_el->SetAttribute("type", "radio");
        radio_el->SetAttribute("value", i);
        radio_el->AddEventListener(Rml::EventId::Click, this, false);
        // TODO: focus event set description
        // TODO: blur event clear description
        Rml::Element *label_el = AppendChild(GetOwnerDocument()->CreateElement("label"));
        label_el->SetAttribute("for", opt_id);
        label_el->SetClass("config-option__tab-label", true);
        {
            label_el->AppendChild(GetOwnerDocument()->CreateTextNode(opt_text));
        }
    }

    set_cur_option(opt);
}

void ElementOptionTypeRadioTabs::ProcessEvent(Rml::Event& event)
{
	// Forward clicks to the target.
	if (event == Rml::EventId::Click)
	{
		if (event.GetPhase() == Rml::EventPhase::Bubble || event.GetPhase() == Rml::EventPhase::Target)
		{
            Rml::Element *target = event.GetTargetElement();
            auto val_variant = target->GetAttribute("value");
            int new_value = val_variant->Get<int>();
            recomp::config::set_config_store_value(config_key, new_value);
            set_cur_option(new_value);
		}
	}
}

} // namespace Rml
