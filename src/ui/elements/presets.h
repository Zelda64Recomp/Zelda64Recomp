#ifndef RECOMPUI_ELEMENTS_PRESETS
#define RECOMPUI_ELEMENTS_PRESETS

#include "common.h"

namespace recompui {
    Rml::Element *add_button(Rml::ElementDocument *doc, Rml::Element *parent_el, const Rml::String contents = "", ButtonVariant variant = ButtonVariant::Primary, bool isLarge = false);
    Rml::Element *add_icon_button(Rml::ElementDocument *doc, Rml::Element *parent_el, const std::string &svg_src, ButtonVariant variant = ButtonVariant::Tertiary);
} // namespace recompui

#endif
