#include "ui_label.h"

#include <cassert>

namespace recompui {

    Label::Label(Element *parent, LabelStyle label_style) : Element(parent, 0U, "div", true) {
        switch (label_style) {
        case LabelStyle::Annotation:
            set_color(Color{ 185, 125, 242, 255 });
            set_font_size(18.0f);
            set_letter_spacing(2.52f);
            set_line_height(18.0f);
            set_font_weight(400);
            break;
        case LabelStyle::Small:
            set_font_size(20.0f);
            set_letter_spacing(0.0f);
            set_line_height(20.0f);
            set_font_weight(400);
            break;
        case LabelStyle::Normal:
            set_font_size(28.0f);
            set_letter_spacing(3.08f);
            set_line_height(28.0f);
            set_font_weight(700);
            break;
        case LabelStyle::Large:
            set_font_size(36.0f);
            set_letter_spacing(2.52f);
            set_line_height(36.0f);
            set_font_weight(700);
            break;
        }
        
        set_font_style(FontStyle::Normal);
    }

    Label::Label(Element *parent, const std::string &text, LabelStyle label_style) : Label(parent, label_style) {
        set_text(text);
    }

};