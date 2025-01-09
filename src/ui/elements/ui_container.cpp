#include "ui_container.h"

#include <cassert>

namespace recompui {

    Container::Container(FlexDirection direction, JustifyContent justify_content, Element *parent) : Element(parent) {
        set_display(Display::Flex);
        set_flex(1.0f, 1.0f);
        set_flex_direction(direction);
        set_justify_content(justify_content);
    }

};