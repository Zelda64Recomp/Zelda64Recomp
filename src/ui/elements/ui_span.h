#pragma once

#include "ui_element.h"
#include "ui_label.h"

namespace recompui {

    class Span : public Element {
    protected:
        std::string_view get_type_name() override { return "Span"; }
    public:
        Span(Element *parent);
        Span(Element *parent, const std::string &text);
    };

} // namespace recompui