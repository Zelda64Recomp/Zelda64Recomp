#include "ui_scroll_container.h"

#include <cassert>

namespace recompui {
    
    ScrollContainer::ScrollContainer(Element *parent, ScrollDirection direction) : Element(parent) {
        set_flex(1.0f, 1.0f, 100.0f);
        set_width(100.0f, Unit::Percent);
        set_height(100.0f, Unit::Percent);

        switch (direction) {
        case ScrollDirection::Horizontal:
            set_max_width(100.0f, Unit::Percent);
            set_overflow_x(Overflow::Auto);
            break;
        case ScrollDirection::Vertical:
            set_max_height(100.0f, Unit::Percent);
            set_overflow_y(Overflow::Auto);
            break;
        default:
            assert(false && "Unknown scroll direction.");
            break;
        }
    }

};