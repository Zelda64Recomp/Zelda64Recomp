#include "ui_style.h"

#include <cassert>

namespace recompui {

    static Rml::Unit to_rml(Unit unit) {
        switch (unit) {
        case Unit::Float:
            return Rml::Unit::NUMBER;
        case Unit::Dp:
            return Rml::Unit::DP;
        case Unit::Percent:
            return Rml::Unit::PERCENT;
        default:
            return Rml::Unit::UNKNOWN;
        }
    }

    static Rml::Style::AlignItems to_rml(AlignItems align_items) {
        switch (align_items) {
        case AlignItems::FlexStart:
            return Rml::Style::AlignItems::FlexStart;
        case AlignItems::FlexEnd:
            return Rml::Style::AlignItems::FlexEnd;
        case AlignItems::Center:
            return Rml::Style::AlignItems::Center;
        case AlignItems::Baseline:
            return Rml::Style::AlignItems::Baseline;
        case AlignItems::Stretch:
            return Rml::Style::AlignItems::Stretch;
        default:
            assert(false && "Unknown align items.");
            return Rml::Style::AlignItems::FlexStart;
        }
    }

    static Rml::Style::Overflow to_rml(Overflow overflow) {
        switch (overflow) {
        case Overflow::Visible:
            return Rml::Style::Overflow::Visible;
        case Overflow::Hidden:
            return Rml::Style::Overflow::Hidden;
        case Overflow::Auto:
            return Rml::Style::Overflow::Auto;
        case Overflow::Scroll:
            return Rml::Style::Overflow::Scroll;
        default:
            assert(false && "Unknown overflow.");
            return Rml::Style::Overflow::Visible;
        }
    }

    static Rml::Style::TextAlign to_rml(TextAlign text_align) {
        switch (text_align) {
        case TextAlign::Left:
            return Rml::Style::TextAlign::Left;
        case TextAlign::Right:
            return Rml::Style::TextAlign::Right;
        case TextAlign::Center:
            return Rml::Style::TextAlign::Center;
        case TextAlign::Justify:
            return Rml::Style::TextAlign::Justify;
        default:
            assert(false && "Unknown text align.");
            return Rml::Style::TextAlign::Left;
        }
    }

    void Style::set_property(Rml::PropertyId property_id, const Rml::Property &property, Animation) {
        property_map[property_id] = property;
    }

    Style::Style() {

    }

    Style::~Style() {

    }

    void Style::set_position(Position position) {
        switch (position) {
        case Position::Absolute:
            set_property(Rml::PropertyId::Position, Rml::Style::Position::Absolute, Animation());
            break;
        case Position::Relative:
            set_property(Rml::PropertyId::Position, Rml::Style::Position::Relative, Animation());
            break;
        default:
            assert(false && "Unknown position.");
            break;
        }
    }

    void Style::set_left(float left, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::Left, Rml::Property(left, to_rml(unit)), animation);
    }

    void Style::set_top(float top, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::Top, Rml::Property(top, to_rml(unit)), animation);
    }

    void Style::set_right(float right, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::Right, Rml::Property(right, to_rml(unit)), animation);
    }

    void Style::set_bottom(float bottom, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::Bottom, Rml::Property(bottom, to_rml(unit)), animation);
    }

    void Style::set_width(float width, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::Width, Rml::Property(width, to_rml(unit)), animation);
    }

    void Style::set_width_auto() {
        set_property(Rml::PropertyId::Width, Rml::Property(Rml::Style::FlexBasis::Type::Auto, Rml::Unit::KEYWORD), Animation());
    }

    void Style::set_height(float height, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::Height, Rml::Property(height, to_rml(unit)), animation);
    }

    void Style::set_height_auto() {
        set_property(Rml::PropertyId::Height, Rml::Property(Rml::Style::FlexBasis::Type::Auto, Rml::Unit::KEYWORD), Animation());
    }

    void Style::set_min_width(float width, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::MinWidth, Rml::Property(width, to_rml(unit)), animation);
    }

    void Style::set_min_height(float height, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::MinHeight, Rml::Property(height, to_rml(unit)), animation);
    }

    void Style::set_max_width(float width, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::MaxWidth, Rml::Property(width, to_rml(unit)), animation);
    }

    void Style::set_max_height(float height, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::MaxHeight, Rml::Property(height, to_rml(unit)), animation);
    }

    void Style::set_padding(float padding, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::PaddingLeft, Rml::Property(padding, to_rml(unit)), animation);
        set_property(Rml::PropertyId::PaddingTop, Rml::Property(padding, to_rml(unit)), animation);
        set_property(Rml::PropertyId::PaddingRight, Rml::Property(padding, to_rml(unit)), animation);
        set_property(Rml::PropertyId::PaddingBottom, Rml::Property(padding, to_rml(unit)), animation);
    }

    void Style::set_padding_left(float padding, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::PaddingLeft, Rml::Property(padding, to_rml(unit)), animation);
    }

    void Style::set_padding_top(float padding, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::PaddingTop, Rml::Property(padding, to_rml(unit)), animation);
    }

    void Style::set_padding_right(float padding, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::PaddingRight, Rml::Property(padding, to_rml(unit)), animation);
    }

    void Style::set_padding_bottom(float padding, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::PaddingBottom, Rml::Property(padding, to_rml(unit)), animation);
    }

    void Style::set_margin(float margin, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::MarginLeft, Rml::Property(margin, to_rml(unit)), animation);
        set_property(Rml::PropertyId::MarginTop, Rml::Property(margin, to_rml(unit)), animation);
        set_property(Rml::PropertyId::MarginRight, Rml::Property(margin, to_rml(unit)), animation);
        set_property(Rml::PropertyId::MarginBottom, Rml::Property(margin, to_rml(unit)), animation);
    }

    void Style::set_margin_left(float margin, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::MarginLeft, Rml::Property(margin, to_rml(unit)), animation);
    }

    void Style::set_margin_top(float margin, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::MarginTop, Rml::Property(margin, to_rml(unit)), animation);
    }

    void Style::set_margin_right(float margin, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::MarginRight, Rml::Property(margin, to_rml(unit)), animation);
    }

    void Style::set_margin_bottom(float margin, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::MarginBottom, Rml::Property(margin, to_rml(unit)), animation);
    }

    void Style::set_border_width(float width, Unit unit, Animation animation) {
        Rml::Property property(width, to_rml(unit));
        set_property(Rml::PropertyId::BorderTopWidth, property, animation);
        set_property(Rml::PropertyId::BorderBottomWidth, property, animation);
        set_property(Rml::PropertyId::BorderLeftWidth, property, animation);
        set_property(Rml::PropertyId::BorderRightWidth, property, animation);
    }

    void Style::set_border_left_width(float width, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::BorderLeftWidth, Rml::Property(width, to_rml(unit)), animation);
    }

    void Style::set_border_top_width(float width, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::BorderTopWidth, Rml::Property(width, to_rml(unit)), animation);
    }

    void Style::set_border_right_width(float width, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::BorderRightWidth, Rml::Property(width, to_rml(unit)), animation);
    }

    void Style::set_border_bottom_width(float width, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::BorderBottomWidth, Rml::Property(width, to_rml(unit)), animation);
    }

    void Style::set_border_radius(float radius, Unit unit, Animation animation) {
        Rml::Property property(radius, to_rml(unit));
        set_property(Rml::PropertyId::BorderTopLeftRadius, property, animation);
        set_property(Rml::PropertyId::BorderTopRightRadius, property, animation);
        set_property(Rml::PropertyId::BorderBottomLeftRadius, property, animation);
        set_property(Rml::PropertyId::BorderBottomRightRadius, property, animation);
    }

    void Style::set_border_top_left_radius(float radius, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::BorderTopLeftRadius, Rml::Property(radius, to_rml(unit)), animation);
    }

    void Style::set_border_top_right_radius(float radius, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::BorderTopRightRadius, Rml::Property(radius, to_rml(unit)), animation);
    }

    void Style::set_border_bottom_left_radius(float radius, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::BorderBottomLeftRadius, Rml::Property(radius, to_rml(unit)), animation);
    }

    void Style::set_border_bottom_right_radius(float radius, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::BorderBottomRightRadius, Rml::Property(radius, to_rml(unit)), animation);
    }

    void Style::set_background_color(const Color &color, Animation animation) {
        Rml::Property property(Rml::Colourb(color.r, color.g, color.b, color.a), Rml::Unit::COLOUR);
        set_property(Rml::PropertyId::BackgroundColor, property, animation);
    }

    void Style::set_border_color(const Color &color, Animation animation) {
        Rml::Property property(Rml::Colourb(color.r, color.g, color.b, color.a), Rml::Unit::COLOUR);
        set_property(Rml::PropertyId::BorderTopColor, property, animation);
        set_property(Rml::PropertyId::BorderBottomColor, property, animation);
        set_property(Rml::PropertyId::BorderLeftColor, property, animation);
        set_property(Rml::PropertyId::BorderRightColor, property, animation);
    }

    void Style::set_border_left_color(const Color &color, Animation animation) {
        Rml::Property property(Rml::Colourb(color.r, color.g, color.b, color.a), Rml::Unit::COLOUR);
        set_property(Rml::PropertyId::BorderLeftColor, property, animation);
    }

    void Style::set_border_top_color(const Color &color, Animation animation) {
        Rml::Property property(Rml::Colourb(color.r, color.g, color.b, color.a), Rml::Unit::COLOUR);
        set_property(Rml::PropertyId::BorderTopColor, property, animation);
    }

    void Style::set_border_right_color(const Color &color, Animation animation) {
        Rml::Property property(Rml::Colourb(color.r, color.g, color.b, color.a), Rml::Unit::COLOUR);
        set_property(Rml::PropertyId::BorderRightColor, property, animation);
    }

    void Style::set_border_bottom_color(const Color &color, Animation animation) {
        Rml::Property property(Rml::Colourb(color.r, color.g, color.b, color.a), Rml::Unit::COLOUR);
        set_property(Rml::PropertyId::BorderBottomColor, property, animation);
    }

    void Style::set_color(const Color &color, Animation animation) {
        Rml::Property property(Rml::Colourb(color.r, color.g, color.b, color.a), Rml::Unit::COLOUR);
        set_property(Rml::PropertyId::Color, property, animation);
    }

    void Style::set_cursor(Cursor cursor) {
        switch (cursor) {
        case Cursor::None:
            assert(false && "Unimplemented.");
            break;
        case Cursor::Pointer:
            set_property(Rml::PropertyId::Cursor, Rml::Property("pointer", Rml::Unit::STRING), Animation());
            break;
        default:
            assert(false && "Unknown cursor.");
            break;
        }
    }

    void Style::set_opacity(float opacity, Animation animation) {
        set_property(Rml::PropertyId::Opacity, Rml::Property(opacity, Rml::Unit::NUMBER), animation);
    }

    void Style::set_display(Display display) {
        switch (display) {
        case Display::Block:
            set_property(Rml::PropertyId::Display, Rml::Style::Display::Block, Animation());
            break;
        case Display::Flex:
            set_property(Rml::PropertyId::Display, Rml::Style::Display::Flex, Animation());
            break;
        default:
            assert(false && "Unknown display.");
            break;
        }
    }

    void Style::set_justify_content(JustifyContent justify_content) {
        switch (justify_content) {
        case JustifyContent::FlexStart:
            set_property(Rml::PropertyId::JustifyContent, Rml::Style::JustifyContent::FlexStart, Animation());
            break;
        case JustifyContent::FlexEnd:
            set_property(Rml::PropertyId::JustifyContent, Rml::Style::JustifyContent::FlexEnd, Animation());
            break;
        case JustifyContent::Center:
            set_property(Rml::PropertyId::JustifyContent, Rml::Style::JustifyContent::Center, Animation());
            break;
        case JustifyContent::SpaceBetween:
            set_property(Rml::PropertyId::JustifyContent, Rml::Style::JustifyContent::SpaceBetween, Animation());
            break;
        case JustifyContent::SpaceAround:
            set_property(Rml::PropertyId::JustifyContent, Rml::Style::JustifyContent::SpaceAround, Animation());
            break;
        case JustifyContent::SpaceEvenly:
            set_property(Rml::PropertyId::JustifyContent, Rml::Style::JustifyContent::SpaceEvenly, Animation());
            break;
        default:
            assert(false && "Unknown justify content.");
            break;
        }
    }

    void Style::set_flex_grow(float grow, Animation animation) {
        set_property(Rml::PropertyId::FlexGrow, Rml::Property(grow, Rml::Unit::NUMBER), animation);
    }

    void Style::set_flex_shrink(float shrink, Animation animation) {
        set_property(Rml::PropertyId::FlexShrink, Rml::Property(shrink, Rml::Unit::NUMBER), animation);
    }

    void Style::set_flex_basis_auto() {
        set_property(Rml::PropertyId::FlexBasis, Rml::Property(Rml::Style::FlexBasis::Type::Auto, Rml::Unit::KEYWORD), Animation());
    }

    void Style::set_flex_basis_percentage(float basis, Animation animation) {
        set_property(Rml::PropertyId::FlexBasis, Rml::Property(basis, Rml::Unit::PERCENT), animation);
    }

    void Style::set_flex(float grow, float shrink, Animation animation) {
        set_flex_grow(grow, animation);
        set_flex_shrink(shrink, animation);
        set_flex_basis_auto();
    }

    void Style::set_flex(float grow, float shrink, float basis_percentage, Animation animation) {
        set_flex_grow(grow, animation);
        set_flex_shrink(shrink, animation);
        set_flex_basis_percentage(basis_percentage, animation);
    }

    void Style::set_flex_direction(FlexDirection flex_direction) {
        switch (flex_direction) {
        case FlexDirection::Row:
            set_property(Rml::PropertyId::FlexDirection, Rml::Style::FlexDirection::Row, Animation());
            break;
        case FlexDirection::Column:
            set_property(Rml::PropertyId::FlexDirection, Rml::Style::FlexDirection::Column, Animation());
            break;
        default:
            assert(false && "Unknown flex direction.");
            break;
        }
    }

    void Style::set_align_items(AlignItems align_items) {
        set_property(Rml::PropertyId::AlignItems, to_rml(align_items), Animation());
    }

    void Style::set_overflow(Overflow overflow) {
        set_property(Rml::PropertyId::OverflowX, to_rml(overflow), Animation());
        set_property(Rml::PropertyId::OverflowY, to_rml(overflow), Animation());
    }

    void Style::set_overflow_x(Overflow overflow) {
        set_property(Rml::PropertyId::OverflowX, to_rml(overflow), Animation());
    }

    void Style::set_overflow_y(Overflow overflow) {
        set_property(Rml::PropertyId::OverflowY, to_rml(overflow), Animation());
    }

    void Style::set_font_size(float size, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::FontSize, Rml::Property(size, to_rml(unit)), animation);
    }

    void Style::set_letter_spacing(float spacing, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::LetterSpacing, Rml::Property(spacing, to_rml(unit)), animation);
    }

    void Style::set_line_height(float height, Unit unit, Animation animation) {
        set_property(Rml::PropertyId::LineHeight, Rml::Property(height, to_rml(unit)), animation);
    }

    void Style::set_font_style(FontStyle style) {
        switch (style) {
        case FontStyle::Normal:
            set_property(Rml::PropertyId::FontStyle, Rml::Style::FontStyle::Normal, Animation());
            break;
        case FontStyle::Italic:
            set_property(Rml::PropertyId::FontStyle, Rml::Style::FontStyle::Italic, Animation());
            break;
        default:
            assert(false && "Unknown font style.");
            break;
        }
    }

    void Style::set_font_weight(uint32_t weight, Animation animation) {
        set_property(Rml::PropertyId::FontWeight, Rml::Style::FontWeight(weight), animation);
    }

    void Style::set_text_align(TextAlign text_align) {
        set_property(Rml::PropertyId::TextAlign, to_rml(text_align), Animation());
    }

} // namespace recompui