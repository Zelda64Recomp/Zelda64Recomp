#include "ui_span.h"

#include <cassert>

namespace recompui {

    Span::Span(Element *parent) : Element(parent, 0, "span", true) {
        set_font_style(FontStyle::Normal);
    }

    Span::Span(Element *parent, const std::string &text) : Span(parent) {
        set_text(text);
    }

};
