#pragma once

#include "ui_element.h"

namespace recompui {

    enum class LabelStyle {
        Annotation,
        Small,
        Normal,
        Large
    };

    class Label : public Element {
    protected:
        std::string_view get_type_name() override { return "Label"; }
    public:
        Label(Element *parent, LabelStyle label_style);
        Label(Element *parent, const std::string &text, LabelStyle label_style);
    };

} // namespace recompui