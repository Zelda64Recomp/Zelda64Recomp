#pragma once

#include "ui_element.h"

namespace recompui {

    class Image : public Element {
    public:
        Image(Element *parent, std::string_view src);
    };

} // namespace recompui