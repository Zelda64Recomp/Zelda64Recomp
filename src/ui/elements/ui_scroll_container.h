#pragma once

#include "ui_element.h"

namespace recompui {

    enum class ScrollDirection {
        Horizontal,
        Vertical
    };

    class ScrollContainer : public Element {
    public:
        ScrollContainer(Element *parent, ScrollDirection direction);
    };

} // namespace recompui
