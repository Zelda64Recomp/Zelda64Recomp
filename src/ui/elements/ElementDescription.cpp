#include "ElementDescription.h"

#include <string>

using json = nlohmann::json;

namespace recompui {

static const std::string cls_base = "config-description"; 
static const std::string cls_contents = cls_base + "__contents"; 

static const std::string contents_id = cls_base + "__contents-text";

static const std::string default_contents = ""; 

ElementDescription::ElementDescription(const Rml::String& tag) : Rml::Element(tag)
{
    SetAttribute("recomp-store-element", true);
    Rml::ElementDocument *doc = GetOwnerDocument();

    SetClass(cls_base, true);
    {
        Rml::Element *p_el = AppendChild(doc->CreateElement("p"));
        p_el->SetClass(cls_contents, true);
        p_el->SetId(contents_id);
    }
}

ElementDescription::~ElementDescription()
{
}

void ElementDescription::update_text(const std::string& text) {
    auto *p_el = GetElementById(contents_id);
    p_el->SetInnerRML(text);
    DirtyLayout();
}

void ElementDescription::OnAttributeChange(const Rml::ElementAttributes& changed_attributes)
{
	// Call through to the base element's OnAttributeChange().
	Rml::Element::OnAttributeChange(changed_attributes);

    auto config_store_key_attr = changed_attributes.find("recomp-data");
    if (config_store_key_attr != changed_attributes.end() && config_store_key_attr->second.GetType() == Rml::Variant::STRING) {
        config_key = config_store_key_attr->second.Get<Rml::String>();

        try {
            auto value = recomp::config::get_config_store_value<std::string>("translations/" + config_key + ":description");
            update_text(value);
        } catch (const std::runtime_error& e) {
            update_text(default_contents);
        }
    }
}

} // namespace Rml
