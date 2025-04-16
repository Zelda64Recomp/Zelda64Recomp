#pragma once

#include "ui_element.h"

namespace recompui {

    class Image : public Element {
    protected:
        std::string_view get_type_name() override { return "ImageView"; }
    public:
        Image(Element *parent, std::string_view src);
    };

} // namespace recompui