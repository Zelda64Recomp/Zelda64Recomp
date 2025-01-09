#include "ui_label.h"

#include <cassert>

namespace recompui {

    Label::Label(LabelStyle label_style, Element *parent) : Element(parent) {
        switch (label_style) {
        case LabelStyle::Normal:
            set_font_size(28.0f);
            set_letter_spacing(3.08f);
            set_line_height(28.0f);
            break;
        case LabelStyle::Large:
            set_font_size(36.0f);
            set_letter_spacing(2.52f);
            set_line_height(36.0f);
            break;
        }
        
        set_font_style(FontStyle::Normal);
        set_font_weight(700);
    }

    Label::Label(const std::string &text, LabelStyle label_style, Element *parent) : Label(label_style, parent) {
        set_text(text);
    }

};