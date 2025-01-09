#pragma once

#include "ui_element.h"

namespace recompui {

    enum class LabelStyle {
        Normal,
        Large
    };

    class Label : public Element {
    public:
        Label(LabelStyle label_style, Element *parent);
        Label(const std::string &text, LabelStyle label_style, Element *parent);
    };

} // namespace recompui