#include "presets.h"

namespace recompui {

static const BEM button_bem("button");
Rml::Element *add_button(Rml::ElementDocument *doc, Rml::Element *parent_el, const Rml::String contents, ButtonVariant variant, bool isLarge) {
    Rml::Element *button = parent_el->AppendChild(doc->CreateElement("button"));

    button->SetClass(button_bem.get_block(), true);

    button->SetClass(button_bem.mod(button_variants.at(variant)), true);
    if (isLarge) {
        button->SetClass(button_bem.mod("large"), true);
    }

    if (contents != "") {
        button->SetInnerRML(contents);
    }

    return button;
}

static const BEM icon_button_bem("icon-button");
Rml::Element *add_icon_button(Rml::ElementDocument *doc, Rml::Element *parent_el, const std::string &svg_src, ButtonVariant variant) {
    Rml::Element *button = parent_el->AppendChild(doc->CreateElement("button"));

    button->SetClass(icon_button_bem.get_block(), true);
    button->SetClass(icon_button_bem.mod(button_variants.at(variant)), true);

    {
        Rml::Element *icon = button->AppendChild(doc->CreateElement("svg"));
        icon->SetClass(icon_button_bem.el("icon"), true);
        icon->SetAttribute("src", svg_src);
    }

    return button;
}

} // namespace recompui
