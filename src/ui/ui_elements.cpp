#include "ui_elements.h"
#include "librecomp/config.hpp"

struct RecompElementConfig {
    Rml::String tag;
    std::unique_ptr<Rml::ElementInstancer> instancer;
};

#define CUSTOM_ELEMENT(s, e) { s, std::make_unique< Rml::ElementInstancerGeneric< e > >() }

static RecompElementConfig custom_elements[] = {
    CUSTOM_ELEMENT("recomp-description", recompui::ElementDescription),
    CUSTOM_ELEMENT("recomp-config-group", recompui::ElementConfigGroup),
    CUSTOM_ELEMENT("recomp-config-option", recompui::ElementConfigOption),
    CUSTOM_ELEMENT("recomp-option-type-checkbox", recompui::ElementOptionTypeCheckbox),
    CUSTOM_ELEMENT("recomp-option-type-color", recompui::ElementOptionTypeColor),
    CUSTOM_ELEMENT("recomp-option-type-dropdown", recompui::ElementOptionTypeDropdown),
    CUSTOM_ELEMENT("recomp-option-type-radio-tabs", recompui::ElementOptionTypeRadioTabs),
    CUSTOM_ELEMENT("recomp-option-type-range", recompui::ElementOptionTypeRange),
};

void recompui::register_custom_elements() {
    recomp::config::set_config_store_value_and_default("ligma_balls", "hello!", "whats up");
    recomp::config::set_config_store_default_value("ligma_balls2", "12345");
    recomp::config::set_config_store_value("ligma_balls3", "hello!");
    for (auto& element_config : custom_elements) {
        Rml::Factory::RegisterElementInstancer(element_config.tag, element_config.instancer.get());
    }
}

Rml::ElementInstancer* recompui::get_custom_element_instancer(std::string tag) {
    for (auto& element_config : custom_elements) {
        if (tag == element_config.tag) {
            return element_config.instancer.get();
        }
    }
    return nullptr;
}

Rml::ElementPtr recompui::create_custom_element(Rml::Element* parent, std::string tag) {
    auto instancer = recompui::get_custom_element_instancer(tag);
    const Rml::XMLAttributes attributes = {};
    if (Rml::ElementPtr element = instancer->InstanceElement(parent, tag, attributes))
    {
        element->SetInstancer(instancer);
        element->SetAttributes(attributes);

        return element;
    }

    return nullptr;
}
