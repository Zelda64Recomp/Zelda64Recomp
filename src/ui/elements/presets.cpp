#include "presets.h"

#define BUTTON_BEM "button"
#define ICON_BUTTON_BEM "icon-button"

namespace recompui {

Rml::Element *add_button(Rml::ElementDocument *doc, Rml::Element *parent_el, const Rml::String contents, ButtonVariant variant, bool isLarge) {
    Rml::Element *button = parent_el->AppendChild(doc->CreateElement("button"));

    button->SetClass(BLOCK(BUTTON_BEM), true);

    button->SetClass(MOD_DYN(BUTTON_BEM, button_variants.at(variant)), true);
    if (isLarge) {
        button->SetClass(MOD(BUTTON_BEM, "large"), true);
    }

    if (contents != "") {
        button->SetInnerRML(contents);
    }

    return button;
}

Rml::Element *add_icon_button(Rml::ElementDocument *doc, Rml::Element *parent_el, const std::string &svg_src, ButtonVariant variant) {
    Rml::Element *button = parent_el->AppendChild(doc->CreateElement("button"));

    button->SetClass(BLOCK(ICON_BUTTON_BEM), true);
    button->SetClass(MOD_DYN(ICON_BUTTON_BEM, button_variants.at(variant)), true);

    {
        Rml::Element *icon = button->AppendChild(doc->CreateElement("svg"));
        icon->SetClass(EL(ICON_BUTTON_BEM, "icon"), true);
        icon->SetAttribute("src", svg_src);
    }

    return button;
}

} // namespace recompui
