#pragma once

#include "ui_element.h"

namespace recompui {

    class Container : public Element {
    protected:
        std::string_view get_type_name() override { return "Container"; }
    public:
        Container(Element* parent, FlexDirection direction, JustifyContent justify_content);
    };

} // namespace recompui
