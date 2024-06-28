
#include "ElementConfigOption.h"
#include "../config_options/ConfigOption.h"
#include "../config_options/ConfigRegistry.h"
#include "ElementOptionTypeCheckbox.h"
#include "ElementOptionTypeRadioTabs.h"
#include "ElementOptionTypeRange.h"
#include "librecomp/config_store.hpp"
#include <string>
#include "recomp_ui.h"
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementText.h>

using json = nlohmann::json;

namespace recompui {

static const std::string option_type_wrapper_id = "checkbox__input"; 

static const std::string config_option_base_class    = "config-option"; 
static const std::string config_option_base_class_hz = config_option_base_class + "--hz"; 
static const std::string config_option_title_class   = config_option_base_class + "__title"; 
static const std::string config_option_wrapper       = config_option_base_class + "__list"; // TODO: move hz listing to radio tabs, make this container dumber 

ElementConfigOption::ElementConfigOption(const Rml::String& tag) : Rml::Element(tag)
{
    Rml::ElementDocument *doc = GetOwnerDocument();

    SetAttribute("recomp-store-element", true);
    SetClass(config_option_base_class, true);

    {
        Rml::Element *label_el = AppendChild(doc->CreateElement("label"));
        label_el->SetClass(config_option_title_class, true);
        {
            Rml::Element *label_text_el = label_el->AppendChild(doc->CreateTextNode("Unknown"));
            label_text_el->SetAttribute("id", "config-opt-label");
        } // label_el

        Rml::Element *option_type_wrapper = AppendChild(doc->CreateElement("div"));
        option_type_wrapper->SetId(option_type_wrapper_id);
        option_type_wrapper->SetClass(config_option_wrapper, true);
    } // base element

    AddEventListener(Rml::EventId::Mouseover, this, true);
}

ElementConfigOption::~ElementConfigOption()
{
    RemoveEventListener(Rml::EventId::Mouseover, this, true);
}

void ElementConfigOption::SetTextLabel(const std::string& s) {
    Rml::ElementText *label = (Rml::ElementText *)GetElementById("config-opt-label");
    label->SetText(s);
    DirtyLayout();
}

Rml::Element *ElementConfigOption::GetOptionTypeWrapper() {
    return GetElementById(option_type_wrapper_id);
}

template <typename T>
static void add_option_el(Rml::ElementDocument *doc, Rml::Element *wrapper, const std::string& tag, std::string& config_key) {
    T *opt = (T *)wrapper->AppendChild(recompui::create_custom_element(doc, tag));
    opt->init_option(config_key);
}

void ElementConfigOption::AddOptionTypeElement() {
    ConfigOptionType el_option_type = ConfigOptionType::Label;
    const json& option_json = get_json_from_key(config_key);
    from_json(option_json["type"], el_option_type);

    Rml::Element *wrapper = GetOptionTypeWrapper();
    Rml::ElementDocument *doc = GetOwnerDocument();

    switch (el_option_type) {
        default:
            printf("No option type element exists for type '%d'\n", el_option_type);
            return;
        case ConfigOptionType::Checkbox: {
            add_option_el<ElementOptionTypeCheckbox>(doc, wrapper, "recomp-option-type-checkbox", config_key);
            break;
        }
        case ConfigOptionType::RadioTabs: {
            add_option_el<ElementOptionTypeRadioTabs>(doc, wrapper, "recomp-option-type-radio-tabs", config_key);
            break;
        }
        case ConfigOptionType::Range: {
            add_option_el<ElementOptionTypeRange>(doc, wrapper, "recomp-option-type-range", config_key);
            break;
        }
    }
}

void ElementConfigOption::OnAttributeChange(const Rml::ElementAttributes& changed_attributes)
{
	// Call through to the base element's OnAttributeChange().
	Rml::Element::OnAttributeChange(changed_attributes);

	bool dirty_layout = false;

    auto config_store_key_attr = changed_attributes.find("recomp-data");
    if (config_store_key_attr != changed_attributes.end() && config_store_key_attr->second.GetType() == Rml::Variant::STRING) {
        config_key = config_store_key_attr->second.Get<Rml::String>();
        if (in_checkbox_group) {
            SetClass(config_option_base_class_hz, true);
        }

        try {
            auto value = recomp::get_config_store_value<std::string>("translations/" + config_key);
            SetTextLabel(value);
            printf("found type and translation\n");
            AddOptionTypeElement();
        } catch (const std::runtime_error& e) {
            SetTextLabel(e.what());
        }
        dirty_layout = true;
    }

	if (dirty_layout) {
		DirtyLayout();
    }
}

void ElementConfigOption::ProcessEvent(Rml::Event& event)
{
	// Set description key
	if (event == Rml::EventId::Mouseover)
	{
		if (event.GetPhase() == Rml::EventPhase::Capture || event.GetPhase() == Rml::EventPhase::Target)
		{
            Rml::ElementList elements;
            Rml::ElementDocument *doc = GetOwnerDocument();
            GetElementsByTagName(elements, "input");
            doc->GetElementsByTagName(elements, "recomp-description");
            for (size_t i = 0; i < elements.size(); i++) {
                Rml::Element *el = elements[i];
                el->SetAttribute("recomp-data", config_key);
            }
		}
	}
}

} // namespace Rml
