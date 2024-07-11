#include "ElementOptionTypeCheckbox.h"

#include <string>


using json = nlohmann::json;

namespace recompui {

static const std::string checkbox_input_id = "checkbox__input"; 

ElementOptionTypeCheckbox::ElementOptionTypeCheckbox(const Rml::String& tag) : Rml::Element(tag)
{
    SetAttribute("recomp-store-element", true);
    Rml::ElementDocument *doc = GetOwnerDocument();

    SetClass("config-option__checkbox-wrapper", true);
    {
        Rml::Element *checkbox_el = AppendChild(doc->CreateElement("input"));
        checkbox_el->SetId(checkbox_input_id);
        checkbox_el->SetClass("config-option__checkbox", true);
        checkbox_el->SetAttribute("type", "checkbox");
    }
    AddEventListener(Rml::EventId::Click, this, true);
}

ElementOptionTypeCheckbox::~ElementOptionTypeCheckbox()
{
    RemoveEventListener(Rml::EventId::Click, this, true);
}

Rml::ElementFormControlInput *ElementOptionTypeCheckbox::get_input() {
    return (Rml::ElementFormControlInput *)GetElementById(checkbox_input_id);
}

void ElementOptionTypeCheckbox::set_checked(bool checked) {
    auto *input_el = get_input();

    if (checked) {
        SetAttribute("checked", true);
        input_el->SetAttribute("checked", true);
    } else {
        RemoveAttribute("checked");
        input_el->RemoveAttribute("checked");
    }
    DirtyLayout();
}

void ElementOptionTypeCheckbox::init_option(std::string& _config_key) {
    config_key = _config_key;
    const json& option_json = recomp::config::get_json_from_key(config_key);
    int value = recomp::config::get_config_store_value<int>(config_key);
    set_checked(value);
}

void ElementOptionTypeCheckbox::ProcessEvent(Rml::Event& event)
{
	// Forward clicks to the target.
	if (event == Rml::EventId::Click && !disable_click)
	{
		if (event.GetPhase() == Rml::EventPhase::Capture || event.GetPhase() == Rml::EventPhase::Target)
		{
            bool new_value = !recomp::config::get_config_store_value<int>(config_key);
            recomp::config::set_config_store_value(config_key, new_value);
            set_checked(new_value);
		}
	}
}

Rml::Element* ElementOptionTypeCheckbox::GetTarget()
{
	return get_input();
}

} // namespace Rml
