#include "ElementOptionTypeColor.h"

#include <string>
#include <math.h>


using json = nlohmann::json;

namespace recompui {

static const std::string range_input_id = "recomp-color-range:"; // + H/S/V 
static const std::string range_label_id = "recomp-color-range-label:"; // + H/S/V 
static const std::string hsv_label[] = {"H", "S", "V"};

static const std::string preview_block_id   = "recomp-color-block"; 

static const std::string cls_base = "config-option-color"; 
static const std::string cls_color_preview_wrapper = cls_base + "__preview-wrapper"; 
static const std::string cls_color_preview_block   = cls_base + "__preview-block"; 
static const std::string cls_color_hsv_wrapper = cls_base + "__hsv-wrapper"; 

// TODO: use these 3 directly from ElementOptionTypeRange
static const std::string range_cls_base = "config-option-range"; 
static const std::string range_cls_label = cls_base + "__label"; 
static const std::string range_cls_range_input = cls_base + "__range-input"; 


ElementOptionTypeColor::ElementOptionTypeColor(const Rml::String& tag) : Rml::Element(tag)
{
    SetAttribute("recomp-store-element", true);
    SetClass(cls_base, true);

    hsv.h = 0;
    hsv.s = 0;
    hsv.v = 0;

    Rml::ElementDocument *doc = GetOwnerDocument();

    {
        Rml::Element *preview_wrapper = AppendChild(doc->CreateElement("div"));
        preview_wrapper->SetClass(cls_color_preview_wrapper, true);
        {
            Rml::Element *preview_block = preview_wrapper->AppendChild(doc->CreateElement("div"));
            preview_block->SetClass(cls_color_preview_block, true);
            preview_block->SetId(preview_block_id);

            Rml::Element *hsv_wrapper = preview_wrapper->AppendChild(doc->CreateElement("div"));
            hsv_wrapper->SetClass(cls_color_hsv_wrapper, true);
            for (int i = 0; i < 3; i++) {
                const auto &label = hsv_label[i];

                Rml::Element *range_wrapper = hsv_wrapper->AppendChild(doc->CreateElement("div"));
                range_wrapper->SetClass(range_cls_base, true);
                {
                    Rml::Element *label_el = range_wrapper->AppendChild(doc->CreateElement("label"));
                    label_el->SetClass(range_cls_label, true);
                    label_el->SetId(range_label_id);
                    {
                        Rml::Element *text_node = label_el->AppendChild(doc->CreateTextNode(""));
                        text_node->SetId(range_label_id + label);
                    }

                    Rml::ElementFormControlInput *range_el = (Rml::ElementFormControlInput *)range_wrapper->AppendChild(doc->CreateElement("input"));
                    range_el->SetClass(range_cls_range_input, true);
                    range_el->SetId(range_input_id + label);
                    range_el->SetAttribute("type", "range");
                    range_el->SetAttribute("min",   0);
                    range_el->SetAttribute("hsv-index", i);
                    if (i == 0) {
                        range_el->SetValue(std::to_string((int)round(hsv[i])));
                        range_el->SetAttribute("max", 360);
                    } else {
                        range_el->SetValue(std::to_string((int)round(hsv[i] * 100.0f)));
                        range_el->SetAttribute("max", 100);
                    }
                    range_el->AddEventListener(Rml::EventId::Change, this, false);
                }
            }

        }

        // TODO: RGB hex input
    }

}

ElementOptionTypeColor::~ElementOptionTypeColor()
{
    Rml::ElementList elements;
    GetElementsByTagName(elements, "input");
    for (int i = 0; i < elements.size(); i++) {
        Rml::Element *el = elements[i];
        el->RemoveEventListener(Rml::EventId::Click, this, false);
    }
}

void ElementOptionTypeColor::set_value_label(int hsvIndex) {
    const auto value = hsv[hsvIndex];
    const auto& which = hsv_label[hsvIndex];

    Rml::ElementText *text_label = (Rml::ElementText *)GetElementById(range_label_id + which);
    if (hsvIndex == 0) {
        text_label->SetText(which + ": " + std::to_string((int)round(value)));
    } else {
        text_label->SetText(which + ": " + std::to_string((int)round(value * 100.0f)));
    }
    DirtyLayout();
}

void ElementOptionTypeColor::set_preview_block_rgb(RgbColor rgb) {
    Rml::Element *color_block = GetElementById(preview_block_id);
    char hex_buf[8]; // Enough to hold "#RRGGBB\0"
    sprintf(hex_buf, "#%02x%02x%02x", rgb.r, rgb.g, rgb.b);
    const std::string hex_val = std::string(hex_buf);

    color_block->SetProperty("background-color", hex_val);
}

void ElementOptionTypeColor::set_config_store_rgb() {
    RgbColor rgb;
    HsvFToRgb(hsv, rgb);
    recomp::config::set_config_store_value(config_key + ":r", rgb.r);
    recomp::config::set_config_store_value(config_key + ":g", rgb.g);
    recomp::config::set_config_store_value(config_key + ":b", rgb.b);
    set_preview_block_rgb(rgb);
}


void ElementOptionTypeColor::init_option(std::string& _config_key) {
    

    config_key = _config_key;

    const json& option_json = recomp::config::get_json_from_key(config_key);

    RgbColor col;
    HsvColor hsv_uc;
    col.r = recomp::config::get_config_store_value<int>(config_key + ":r");
    col.g = recomp::config::get_config_store_value<int>(config_key + ":g");
    col.b = recomp::config::get_config_store_value<int>(config_key + ":b");

    RgbToHsv(col, hsv_uc);
    hsv.h = std::clamp(hsv_uc.h * 360.0f, 0.0f, 360.0f);
    hsv.s = std::clamp((float)hsv_uc.s / 255.0f, 0.0f, 1.0f);
    hsv.v = std::clamp((float)hsv_uc.v / 255.0f, 0.0f, 1.0f);

    set_preview_block_rgb(col);

    for (int i = 0; i < 3; i++) {
        const auto &label = hsv_label[i];

        Rml::ElementFormControlInput *range = (Rml::ElementFormControlInput *)GetElementById(range_input_id + label);
        if (i == 0) {
            range->SetValue(std::to_string((int)round(hsv[i])));
        } else {
            range->SetValue(std::to_string((int)round(hsv[i] * 100.0f)));
        }
        set_value_label(i);
    }
}

void ElementOptionTypeColor::ProcessEvent(Rml::Event& event)
{
	if (event == Rml::EventId::Change)
	{
		if (event.GetPhase() == Rml::EventPhase::Bubble || event.GetPhase() == Rml::EventPhase::Target)
		{
            Rml::Element *target = event.GetTargetElement();
            auto val_variant = target->GetAttribute("value");
            int new_value = val_variant->Get<int>();

            auto idx_variant = target->GetAttribute("hsv-index");
            auto hsv_index = idx_variant->Get<int>();
            if (hsv_index == 0) {
                hsv[hsv_index] = (float)new_value;
            } else {
                hsv[hsv_index] = (float)new_value / 100.0f;
            }
            set_value_label(hsv_index);
            set_config_store_rgb();
		}
	}
}

} // namespace Rml
