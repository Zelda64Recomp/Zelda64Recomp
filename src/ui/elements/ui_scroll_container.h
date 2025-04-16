#pragma once

#include "ui_element.h"

namespace recompui {

    enum class ScrollDirection {
        Horizontal,
        Vertical
    };

    class ScrollContainer : public Element {
    protected:
        std::string_view get_type_name() override { return "ScrollContainer"; }
    public:
        ScrollContainer(Element *parent, ScrollDirection direction);
    };

} // namespace recompui
