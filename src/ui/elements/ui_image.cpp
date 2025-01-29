#include "ui_image.h"

#include <cassert>

namespace recompui {

    Image::Image(Element *parent, std::string_view src) : Element(parent, 0, "img") {
        set_src(src);
    }

};