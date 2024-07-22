
#include "ElementOptionTypeDropdown.h"

#include <string>

using json = nlohmann::json;

namespace recompui {

static const std::string select_id = "recomp-select"; 
static const std::string select_option_id = "recomp-select-option__"; 

static const std::string cls_base = "config-option-dropdown"; 
static const std::string cls_select = cls_base + "__select"; 

ElementOptionTypeDropdown::ElementOptionTypeDropdown(const Rml::String& tag) : ElementOptionType(tag, cls_base)
{
    Rml::ElementFormControlSelect *select_el = (Rml::ElementFormControlSelect *)AppendChild(GetOwnerDocument()->CreateElement("select"));
    select_el->SetId(select_id);
    select_el->SetValue("0");
    select_el->AddEventListener(Rml::EventId::Change, this, false);
    select_el->SetClass(cls_select, true);
}

ElementOptionTypeDropdown::~ElementOptionTypeDropdown()
{
    auto select_el = get_select();
    RemoveEventListener(Rml::EventId::Change, this, false);
}

Rml::ElementFormControlSelect *ElementOptionTypeDropdown::get_select()
{
    return (Rml::ElementFormControlSelect *)GetElementById(select_id);
}

void ElementOptionTypeDropdown::set_cur_option(int opt) {
    auto select_el = get_select();
    select_el->SetValue(std::to_string(opt));
    DirtyLayout();
}

void ElementOptionTypeDropdown::init_option(std::string& _config_key) {
    config_key = _config_key;

    const json& option_json = recomp::config::get_json_from_key(config_key);;
    int opt = recomp::config::get_config_store_value<int>(config_key);
    const json& opt_array = option_json["values"];

    auto select_el = get_select();

    for (int i = 0; i < opt_array.size(); i++) {
        const auto &j_opt = opt_array[i];
        const std::string opt_val = j_opt.get<std::string>();
        const std::string opt_id = select_option_id + config_key + "--" + opt_val;
        
        const std::string translation_key = "translations/" + config_key + "/values/" + opt_val;
        const std::string& opt_text = recomp::config::get_config_store_value<std::string>(translation_key);

        select_el->Add(opt_text, std::to_string(i));
    }

    set_cur_option(opt);
}

void ElementOptionTypeDropdown::ProcessEvent(Rml::Event& event)
{
	if (event == Rml::EventId::Change)
	{
		if (event.GetPhase() == Rml::EventPhase::Bubble || event.GetPhase() == Rml::EventPhase::Target)
		{
            Rml::Element *target = event.GetTargetElement();
            auto val_variant = target->GetAttribute("value");
            int new_value = val_variant->Get<int>();
            recomp::config::set_config_store_value(config_key, new_value);
		}
	}
}

} // namespace Rml
