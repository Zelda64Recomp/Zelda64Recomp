#include "ElementConfigGroup.h"
#include "ElementConfigOption.h"

#include <string>
#include <cassert>

using json = nlohmann::json;
using ConfigOptionType = recomp::config::ConfigOptionType;
using ConfigOption = recomp::config::ConfigOption;

namespace recompui {

static const std::string config_group_base_class            = "config-group"; 
static const std::string config_group_base_class_scrollable = config_group_base_class  + "--scrollable"; 
static const std::string config_group_title_class           = config_group_base_class  + "__title"; 
static const std::string config_group_title_class_hidden    = config_group_title_class + "--hidden"; 
static const std::string config_group_wrapper_class         = config_group_base_class  + "__wrapper"; 

ElementConfigGroup::ElementConfigGroup(const Rml::String& tag) : Rml::Element(tag)
{
    SetAttribute("recomp-store-element", true);
    SetClass(config_group_base_class, true);

    Rml::ElementDocument *doc = GetOwnerDocument();

    {
        Rml::Element *title_el = AppendChild(doc->CreateElement("div"));
        title_el->SetClass(config_group_title_class, true);
        {
            Rml::Element *text_el = title_el->AppendChild(doc->CreateTextNode("replace me"));
            text_el->SetId("config-group-label");
        } // title_el

        Rml::Element *div_el = AppendChild(doc->CreateElement("div"));
        div_el->SetClass(config_group_wrapper_class, true);
    }
}

ElementConfigGroup::~ElementConfigGroup()
{

}

void ElementConfigGroup::SetTextLabel(const std::string& s) {
    Rml::Element *title_container = GetChild(0);
    Rml::ElementText *label = (Rml::ElementText *)title_container->GetElementById("config-group-label");
    label->SetText(s);
    DirtyLayout();
}

void ElementConfigGroup::ToggleTextLabelVisibility(bool show) {
    Rml::Element *title_container = GetChild(0);
    title_container->SetClass(config_group_title_class_hidden, !show);
}

void ElementConfigGroup::AddConfigOptionElement(const json& option_json) {
    ConfigOptionType el_option_type = ConfigOptionType::Label;
    from_json(option_json["type"], el_option_type);

    const std::string key = recomp::config::get_string_in_json(option_json, ConfigOption::schema::key);

    Rml::Element *option_container = GetChild(1);
    Rml::Element *child_opt = nullptr;

    if (config_option_is_group(el_option_type)) {
        child_opt = (ElementConfigGroup *)option_container->AppendChild(
            recompui::create_custom_element(GetOwnerDocument(), "recomp-config-group")
        );
        // TODO: Base class with option type + config_key
        ((ElementConfigGroup *)child_opt)->option_type = el_option_type;
    } else {
        child_opt = (ElementConfigOption *)option_container->AppendChild(
            recompui::create_custom_element(GetOwnerDocument(), "recomp-config-option")
        );
        // TODO: Base class with option type + config_key
        ElementConfigOption *opt_as_conf_opt = (ElementConfigOption *)child_opt;
        opt_as_conf_opt->option_type = el_option_type;
        opt_as_conf_opt->in_checkbox_group = option_type == ConfigOptionType::CheckboxGroup;
    }
    std::string full_key = config_key + "/" + key;
    child_opt->SetAttribute("recomp-data", full_key);
}

static nlohmann::json get_options(std::string& config_key) {
    if (recomp::config::config_key_is_base_group(config_key)) {
        return recomp::config::get_group_json(config_key);
    }

    const json& group_json = recomp::config::get_json_from_key(config_key);
    return group_json["options"];
}

void ElementConfigGroup::OnAttributeChange(const Rml::ElementAttributes& changed_attributes)
{
	// Call through to the base element's OnAttributeChange().
	Rml::Element::OnAttributeChange(changed_attributes);

    auto config_store_key_attr = changed_attributes.find("recomp-data");
    if (config_store_key_attr != changed_attributes.end() && config_store_key_attr->second.GetType() == Rml::Variant::STRING) {
        config_key = config_store_key_attr->second.Get<Rml::String>();
        bool is_base_group = recomp::config::config_key_is_base_group(config_key);
    
        option_type = ConfigOptionType::Label;

        if (is_base_group) {
            SetClass(config_group_base_class_scrollable, true);
            option_type = ConfigOptionType::Group;
            ToggleTextLabelVisibility(false);
        } else {
            try {
                auto value = recomp::config::get_config_store_value<std::string>("translations/" + config_key);
                SetTextLabel(value);
            } catch (const std::runtime_error& e) {
                SetTextLabel(e.what());
            }

            const json& group_json = recomp::config::get_json_from_key(config_key);

            from_json(group_json["type"], option_type);
            assert(
                option_type == ConfigOptionType::Group || option_type == ConfigOptionType::CheckboxGroup);
        }

        const nlohmann::json& options = get_options(config_key);

        for (int i = 0; i < options.size(); i++) {
            const auto &el = options[i];
            AddConfigOptionElement(el);
        }
		DirtyLayout();
    }
}

} // namespace Rml
