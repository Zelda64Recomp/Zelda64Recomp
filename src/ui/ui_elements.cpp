#include "ui_elements.h"

struct RecompCustomElement {
    Rml::String tag;
    std::unique_ptr<Rml::ElementInstancer> instancer;
};

#define CUSTOM_ELEMENT(s, e) { s, std::make_unique< Rml::ElementInstancerGeneric< e > >() }

static RecompCustomElement custom_elements[] = {
    CUSTOM_ELEMENT("recomp-mod-menu", recompui::ElementModMenu),
    CUSTOM_ELEMENT("recomp-config-sub-menu", recompui::ElementConfigSubMenu),
};

void recompui::register_custom_elements() {
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
