#pragma once

#include "ui_element.h"

namespace recompui {

    enum class ScrollDirection {
        Horizontal,
        Vertical
    };

    class ScrollContainer : public Element {
    public:
        ScrollContainer(ScrollDirection direction, Element *parent);
    };

} // namespace recompui
