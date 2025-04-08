#include "ui_container.h"

#include <cassert>

namespace recompui {

    Container::Container(Element *parent, FlexDirection direction, JustifyContent justify_content) : Element(parent) {
        set_display(Display::Flex);
        set_flex_direction(direction);
        set_justify_content(justify_content);
    }

};