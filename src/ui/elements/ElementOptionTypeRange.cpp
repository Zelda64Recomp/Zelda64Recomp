
#include "ElementOptionTypeRange.h"

#include <string>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementText.h>

using json = nlohmann::json;

namespace recompui {

static const std::string range_label_id = "recomp-range__label";
static const std::string range_label_text_id = "recomp-range__label-text";
static const std::string range_input_id = "recomp-range__input";

static const std::string cls_base = "config-option-range"; 
static const std::string cls_label = cls_base + "__label"; 
static const std::string cls_range_input = cls_base + "__range-input"; 

ElementOptionTypeRange::ElementOptionTypeRange(const Rml::String& tag) : Rml::Element(tag)
{
    SetAttribute("recomp-store-element", true);
    SetClass(cls_base, true);

    Rml::ElementDocument *doc = GetOwnerDocument();

    {
        Rml::Element *label_el = AppendChild(doc->CreateElement("label"));
        label_el->SetClass(cls_label, true);
        label_el->SetId(range_label_id);
        {
            Rml::Element *text_node = label_el->AppendChild(doc->CreateTextNode(""));
            text_node->SetId(range_label_text_id);
        }

        Rml::Element *range_el = AppendChild(doc->CreateElement("input"));
        range_el->SetClass(cls_range_input, true);
        range_el->SetId(range_input_id);
        range_el->SetAttribute("type", "range");
        range_el->AddEventListener(Rml::EventId::Change, this, false);
        // TODO: focus event set description
        // TODO: blur event clear description
    }
}

ElementOptionTypeRange::~ElementOptionTypeRange()
{
    Rml::Element *range = GetElementById(range_input_id);
    range->RemoveEventListener(Rml::EventId::Change, this, false);
}

void ElementOptionTypeRange::set_value_label(int value) {
    Rml::ElementText *text_label = (Rml::ElementText *)GetElementById(range_label_text_id);
    text_label->SetText(std::to_string(value) + suffix);
    DirtyLayout();
}

void ElementOptionTypeRange::init_option(std::string& _config_key) {
    config_key = _config_key;
    const json& option_json = recomp::config::get_json_from_key(config_key);

    const int value = recomp::config::get_config_store_value<int>(config_key);
    suffix = recomp::config::get_string_in_json_with_default(option_json, "suffix", "");
    const int min = recomp::config::get_value_in_json<int>(option_json, "min");
    const int max = recomp::config::get_value_in_json<int>(option_json, "max");
    const int step = recomp::config::get_value_in_json_with_default<int>(option_json, "step", 1);

    Rml::ElementFormControlInput *range = (Rml::ElementFormControlInput *)GetElementById(range_input_id);
    range->SetAttribute("min", min);
    range->SetAttribute("max", max);
    range->SetAttribute("step", step);
    range->SetValue(std::to_string(value));

    set_value_label(value);
}

void ElementOptionTypeRange::ProcessEvent(Rml::Event& event)
{
	if (event == Rml::EventId::Change)
	{
		if (event.GetPhase() == Rml::EventPhase::Bubble || event.GetPhase() == Rml::EventPhase::Target)
		{
            Rml::ElementFormControlInput *target = (Rml::ElementFormControlInput *)event.GetTargetElement();
            auto val_s = target->GetValue();
            int new_value = std::stoi(val_s);
            recomp::config::set_config_store_value(config_key, new_value);
            set_value_label(new_value);
		}
	}
}

} // namespace Rml
