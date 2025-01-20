#pragma once

#include "ui_element.h"

namespace recompui {

    class Container : public Element {
    public:
        Container(Element* parent, FlexDirection direction, JustifyContent justify_content);
    };

} // namespace recompui
