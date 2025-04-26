#include "ui_style.h"
#include "ui_element.h"

#include <cassert>

namespace recompui {

    static Rml::Unit to_rml(Unit unit) {
        switch (unit) {
        case Unit::Px:
            return Rml::Unit::PX;
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

    static Rml::Style::TextTransform to_rml(TextTransform text_transform) {
        switch (text_transform) {
        case TextTransform::None:
            return Rml::Style::TextTransform::None;
        case TextTransform::Capitalize:
            return Rml::Style::TextTransform::Capitalize;
        case TextTransform::Uppercase:
            return Rml::Style::TextTransform::Uppercase;
        case TextTransform::Lowercase:
            return Rml::Style::TextTransform::Lowercase;
        default:
            assert(false && "Unknown text transform.");
            return Rml::Style::TextTransform::None;
        }
    }

    static Rml::Style::Drag to_rml(Drag drag) {
        switch (drag) {
        case Drag::None:
            return Rml::Style::Drag::None;
        case Drag::Drag:
            return Rml::Style::Drag::Drag;
        case Drag::DragDrop:
            return Rml::Style::Drag::DragDrop;
        case Drag::Block:
            return Rml::Style::Drag::Block;
        case Drag::Clone:
            return Rml::Style::Drag::Clone;
        default:
            assert(false && "Unknown drag.");
            return Rml::Style::Drag::None;
        }
    }

    static Rml::Style::TabIndex to_rml(TabIndex tab_index) {
        switch (tab_index) {
        case TabIndex::None:
            return Rml::Style::TabIndex::None;
        case TabIndex::Auto:
            return Rml::Style::TabIndex::Auto;
        default:
            assert(false && "Unknown tab index.");
            return Rml::Style::TabIndex::None;
        }
    }

    static Rml::Style::Display to_rml(Display display) {
        switch (display) {
        case Display::None:
            return Rml::Style::Display::None;
        case Display::Block:
            return Rml::Style::Display::Block;
        case Display::Inline:
            return Rml::Style::Display::Inline;
        case Display::InlineBlock:
            return Rml::Style::Display::InlineBlock;
        case Display::FlowRoot:
            return Rml::Style::Display::FlowRoot;
        case Display::Flex:
            return Rml::Style::Display::Flex;
        case Display::InlineFlex:
            return Rml::Style::Display::InlineFlex;
        case Display::Table:
            return Rml::Style::Display::Table;
        case Display::InlineTable:
            return Rml::Style::Display::InlineTable;
        case Display::TableRow:
            return Rml::Style::Display::TableRow;
        case Display::TableRowGroup:
            return Rml::Style::Display::TableRowGroup;
        case Display::TableColumn:
            return Rml::Style::Display::TableColumn;
        case Display::TableColumnGroup:
            return Rml::Style::Display::TableColumnGroup;
        case Display::TableCell:
            return Rml::Style::Display::TableCell;
        default:
            assert(false && "Unknown display.");
            return Rml::Style::Display::Block;
        }
    }

    static Rml::Style::JustifyContent to_rml(JustifyContent justify_content) {
        switch (justify_content) {
        case JustifyContent::FlexStart:
            return Rml::Style::JustifyContent::FlexStart;
        case JustifyContent::FlexEnd:
            return Rml::Style::JustifyContent::FlexEnd;
        case JustifyContent::Center:
            return Rml::Style::JustifyContent::Center;
        case JustifyContent::SpaceBetween:
            return Rml::Style::JustifyContent::SpaceBetween;
        case JustifyContent::SpaceAround:
            return Rml::Style::JustifyContent::SpaceAround;
        case JustifyContent::SpaceEvenly:
            return Rml::Style::JustifyContent::SpaceEvenly;
        default:
            assert(false && "Unknown justify content.");
            return Rml::Style::JustifyContent::FlexStart;
        }
    }

    static Rml::PropertyId nav_to_property(NavDirection dir) {
        switch (dir) {
            case NavDirection::Up:
                return Rml::PropertyId::NavUp;
            case NavDirection::Right:
                return Rml::PropertyId::NavRight;
            case NavDirection::Down:
                return Rml::PropertyId::NavDown;
            case NavDirection::Left:
                return Rml::PropertyId::NavLeft;
            default:
                assert(false && "Unknown nav direction.");
                return Rml::PropertyId::Invalid;
        }
    }

    void Style::set_property(Rml::PropertyId property_id, const Rml::Property &property) {
        property_map[property_id] = property;
    }

    Style::Style() {

    }

    Style::~Style() {

    }

    void Style::set_visibility(Visibility visibility) {
        switch (visibility) {
        case Visibility::Visible:
            set_property(Rml::PropertyId::Visibility, Rml::Style::Visibility::Visible);
            break;
        case Visibility::Hidden:
            set_property(Rml::PropertyId::Visibility, Rml::Style::Visibility::Hidden);
            break;
        }
    }

    void Style::set_position(Position position) {
        switch (position) {
        case Position::Absolute:
            set_property(Rml::PropertyId::Position, Rml::Style::Position::Absolute);
            break;
        case Position::Relative:
            set_property(Rml::PropertyId::Position, Rml::Style::Position::Relative);
            break;
        default:
            assert(false && "Unknown position.");
            break;
        }
    }

    void Style::set_left(float left, Unit unit) {
        set_property(Rml::PropertyId::Left, Rml::Property(left, to_rml(unit)));
    }

    void Style::set_top(float top, Unit unit) {
        set_property(Rml::PropertyId::Top, Rml::Property(top, to_rml(unit)));
    }

    void Style::set_right(float right, Unit unit) {
        set_property(Rml::PropertyId::Right, Rml::Property(right, to_rml(unit)));
    }

    void Style::set_bottom(float bottom, Unit unit) {
        set_property(Rml::PropertyId::Bottom, Rml::Property(bottom, to_rml(unit)));
    }

    void Style::set_width(float width, Unit unit) {
        set_property(Rml::PropertyId::Width, Rml::Property(width, to_rml(unit)));
    }

    void Style::set_width_auto() {
        set_property(Rml::PropertyId::Width, Rml::Property(Rml::Style::FlexBasis::Type::Auto, Rml::Unit::KEYWORD));
    }

    void Style::set_height(float height, Unit unit) {
        set_property(Rml::PropertyId::Height, Rml::Property(height, to_rml(unit)));
    }

    void Style::set_height_auto() {
        set_property(Rml::PropertyId::Height, Rml::Property(Rml::Style::FlexBasis::Type::Auto, Rml::Unit::KEYWORD));
    }

    void Style::set_min_width(float width, Unit unit) {
        set_property(Rml::PropertyId::MinWidth, Rml::Property(width, to_rml(unit)));
    }

    void Style::set_min_height(float height, Unit unit) {
        set_property(Rml::PropertyId::MinHeight, Rml::Property(height, to_rml(unit)));
    }

    void Style::set_max_width(float width, Unit unit) {
        set_property(Rml::PropertyId::MaxWidth, Rml::Property(width, to_rml(unit)));
    }

    void Style::set_max_height(float height, Unit unit) {
        set_property(Rml::PropertyId::MaxHeight, Rml::Property(height, to_rml(unit)));
    }

    void Style::set_padding(float padding, Unit unit) {
        set_property(Rml::PropertyId::PaddingLeft, Rml::Property(padding, to_rml(unit)));
        set_property(Rml::PropertyId::PaddingTop, Rml::Property(padding, to_rml(unit)));
        set_property(Rml::PropertyId::PaddingRight, Rml::Property(padding, to_rml(unit)));
        set_property(Rml::PropertyId::PaddingBottom, Rml::Property(padding, to_rml(unit)));
    }

    void Style::set_padding_left(float padding, Unit unit) {
        set_property(Rml::PropertyId::PaddingLeft, Rml::Property(padding, to_rml(unit)));
    }

    void Style::set_padding_top(float padding, Unit unit) {
        set_property(Rml::PropertyId::PaddingTop, Rml::Property(padding, to_rml(unit)));
    }

    void Style::set_padding_right(float padding, Unit unit) {
        set_property(Rml::PropertyId::PaddingRight, Rml::Property(padding, to_rml(unit)));
    }

    void Style::set_padding_bottom(float padding, Unit unit) {
        set_property(Rml::PropertyId::PaddingBottom, Rml::Property(padding, to_rml(unit)));
    }

    void Style::set_margin(float margin, Unit unit) {
        set_property(Rml::PropertyId::MarginLeft, Rml::Property(margin, to_rml(unit)));
        set_property(Rml::PropertyId::MarginTop, Rml::Property(margin, to_rml(unit)));
        set_property(Rml::PropertyId::MarginRight, Rml::Property(margin, to_rml(unit)));
        set_property(Rml::PropertyId::MarginBottom, Rml::Property(margin, to_rml(unit)));
    }

    void Style::set_margin_left(float margin, Unit unit) {
        set_property(Rml::PropertyId::MarginLeft, Rml::Property(margin, to_rml(unit)));
    }

    void Style::set_margin_top(float margin, Unit unit) {
        set_property(Rml::PropertyId::MarginTop, Rml::Property(margin, to_rml(unit)));
    }

    void Style::set_margin_right(float margin, Unit unit) {
        set_property(Rml::PropertyId::MarginRight, Rml::Property(margin, to_rml(unit)));
    }

    void Style::set_margin_bottom(float margin, Unit unit) {
        set_property(Rml::PropertyId::MarginBottom, Rml::Property(margin, to_rml(unit)));
    }

    void Style::set_margin_auto() {
        set_property(Rml::PropertyId::MarginLeft, Rml::Property(Rml::Style::Margin::Type::Auto, Rml::Unit::KEYWORD));
        set_property(Rml::PropertyId::MarginTop, Rml::Property(Rml::Style::Margin::Type::Auto, Rml::Unit::KEYWORD));
        set_property(Rml::PropertyId::MarginRight, Rml::Property(Rml::Style::Margin::Type::Auto, Rml::Unit::KEYWORD));
        set_property(Rml::PropertyId::MarginBottom, Rml::Property(Rml::Style::Margin::Type::Auto, Rml::Unit::KEYWORD));
    }

    void Style::set_margin_left_auto() {
        set_property(Rml::PropertyId::MarginLeft, Rml::Property(Rml::Style::Margin::Type::Auto, Rml::Unit::KEYWORD));
    }

    void Style::set_margin_top_auto() {
        set_property(Rml::PropertyId::MarginTop, Rml::Property(Rml::Style::Margin::Type::Auto, Rml::Unit::KEYWORD));
    }

    void Style::set_margin_right_auto() {
        set_property(Rml::PropertyId::MarginRight, Rml::Property(Rml::Style::Margin::Type::Auto, Rml::Unit::KEYWORD));
    }

    void Style::set_margin_bottom_auto() {
        set_property(Rml::PropertyId::MarginBottom, Rml::Property(Rml::Style::Margin::Type::Auto, Rml::Unit::KEYWORD));
    }

    void Style::set_border_width(float width, Unit unit) {
        Rml::Property property(width, to_rml(unit));
        set_property(Rml::PropertyId::BorderTopWidth, property);
        set_property(Rml::PropertyId::BorderBottomWidth, property);
        set_property(Rml::PropertyId::BorderLeftWidth, property);
        set_property(Rml::PropertyId::BorderRightWidth, property);
    }

    void Style::set_border_left_width(float width, Unit unit) {
        set_property(Rml::PropertyId::BorderLeftWidth, Rml::Property(width, to_rml(unit)));
    }

    void Style::set_border_top_width(float width, Unit unit) {
        set_property(Rml::PropertyId::BorderTopWidth, Rml::Property(width, to_rml(unit)));
    }

    void Style::set_border_right_width(float width, Unit unit) {
        set_property(Rml::PropertyId::BorderRightWidth, Rml::Property(width, to_rml(unit)));
    }

    void Style::set_border_bottom_width(float width, Unit unit) {
        set_property(Rml::PropertyId::BorderBottomWidth, Rml::Property(width, to_rml(unit)));
    }

    void Style::set_border_radius(float radius, Unit unit) {
        Rml::Property property(radius, to_rml(unit));
        set_property(Rml::PropertyId::BorderTopLeftRadius, property);
        set_property(Rml::PropertyId::BorderTopRightRadius, property);
        set_property(Rml::PropertyId::BorderBottomLeftRadius, property);
        set_property(Rml::PropertyId::BorderBottomRightRadius, property);
    }

    void Style::set_border_top_left_radius(float radius, Unit unit) {
        set_property(Rml::PropertyId::BorderTopLeftRadius, Rml::Property(radius, to_rml(unit)));
    }

    void Style::set_border_top_right_radius(float radius, Unit unit) {
        set_property(Rml::PropertyId::BorderTopRightRadius, Rml::Property(radius, to_rml(unit)));
    }

    void Style::set_border_bottom_left_radius(float radius, Unit unit) {
        set_property(Rml::PropertyId::BorderBottomLeftRadius, Rml::Property(radius, to_rml(unit)));
    }

    void Style::set_border_bottom_right_radius(float radius, Unit unit) {
        set_property(Rml::PropertyId::BorderBottomRightRadius, Rml::Property(radius, to_rml(unit)));
    }

    void Style::set_background_color(const Color &color) {
        Rml::Property property(Rml::Colourb(color.r, color.g, color.b, color.a), Rml::Unit::COLOUR);
        set_property(Rml::PropertyId::BackgroundColor, property);
    }

    void Style::set_border_color(const Color &color) {
        Rml::Property property(Rml::Colourb(color.r, color.g, color.b, color.a), Rml::Unit::COLOUR);
        set_property(Rml::PropertyId::BorderTopColor, property);
        set_property(Rml::PropertyId::BorderBottomColor, property);
        set_property(Rml::PropertyId::BorderLeftColor, property);
        set_property(Rml::PropertyId::BorderRightColor, property);
    }

    void Style::set_border_left_color(const Color &color) {
        Rml::Property property(Rml::Colourb(color.r, color.g, color.b, color.a), Rml::Unit::COLOUR);
        set_property(Rml::PropertyId::BorderLeftColor, property);
    }

    void Style::set_border_top_color(const Color &color) {
        Rml::Property property(Rml::Colourb(color.r, color.g, color.b, color.a), Rml::Unit::COLOUR);
        set_property(Rml::PropertyId::BorderTopColor, property);
    }

    void Style::set_border_right_color(const Color &color) {
        Rml::Property property(Rml::Colourb(color.r, color.g, color.b, color.a), Rml::Unit::COLOUR);
        set_property(Rml::PropertyId::BorderRightColor, property);
    }

    void Style::set_border_bottom_color(const Color &color) {
        Rml::Property property(Rml::Colourb(color.r, color.g, color.b, color.a), Rml::Unit::COLOUR);
        set_property(Rml::PropertyId::BorderBottomColor, property);
    }

    void Style::set_color(const Color &color) {
        Rml::Property property(Rml::Colourb(color.r, color.g, color.b, color.a), Rml::Unit::COLOUR);
        set_property(Rml::PropertyId::Color, property);
    }

    void Style::set_cursor(Cursor cursor) {
        switch (cursor) {
        case Cursor::None:
            set_property(Rml::PropertyId::Cursor, Rml::Property("", Rml::Unit::STRING));
            break;
        case Cursor::Pointer:
            set_property(Rml::PropertyId::Cursor, Rml::Property("pointer", Rml::Unit::STRING));
            break;
        default:
            assert(false && "Unknown cursor.");
            break;
        }
    }

    void Style::set_opacity(float opacity) {
        set_property(Rml::PropertyId::Opacity, Rml::Property(opacity, Rml::Unit::NUMBER));
    }

    void Style::set_display(Display display) {
        set_property(Rml::PropertyId::Display, to_rml(display));
    }

    void Style::set_justify_content(JustifyContent justify_content) {
        set_property(Rml::PropertyId::JustifyContent, to_rml(justify_content));
    }

    void Style::set_flex_grow(float grow) {
        set_property(Rml::PropertyId::FlexGrow, Rml::Property(grow, Rml::Unit::NUMBER));
    }

    void Style::set_flex_shrink(float shrink) {
        set_property(Rml::PropertyId::FlexShrink, Rml::Property(shrink, Rml::Unit::NUMBER));
    }

    void Style::set_flex_basis_auto() {
        set_property(Rml::PropertyId::FlexBasis, Rml::Property(Rml::Style::FlexBasis::Type::Auto, Rml::Unit::KEYWORD));
    }

    void Style::set_flex_basis(float basis, Unit unit) {
        set_property(Rml::PropertyId::FlexBasis, Rml::Property(basis, to_rml(unit)));
    }

    void Style::set_flex(float grow, float shrink) {
        set_flex_grow(grow);
        set_flex_shrink(shrink);
        set_flex_basis_auto();
    }

    void Style::set_flex(float grow, float shrink, float basis, Unit basis_unit) {
        set_flex_grow(grow);
        set_flex_shrink(shrink);
        set_flex_basis(basis, basis_unit);
    }

    void Style::set_flex_direction(FlexDirection flex_direction) {
        switch (flex_direction) {
        case FlexDirection::Row:
            set_property(Rml::PropertyId::FlexDirection, Rml::Style::FlexDirection::Row);
            break;
        case FlexDirection::Column:
            set_property(Rml::PropertyId::FlexDirection, Rml::Style::FlexDirection::Column);
            break;
        case FlexDirection::RowReverse:
            set_property(Rml::PropertyId::FlexDirection, Rml::Style::FlexDirection::RowReverse);
            break;
        case FlexDirection::ColumnReverse:
            set_property(Rml::PropertyId::FlexDirection, Rml::Style::FlexDirection::ColumnReverse);
            break;
        default:
            assert(false && "Unknown flex direction.");
            break;
        }
    }

    void Style::set_align_items(AlignItems align_items) {
        set_property(Rml::PropertyId::AlignItems, to_rml(align_items));
    }

    void Style::set_overflow(Overflow overflow) {
        set_property(Rml::PropertyId::OverflowX, to_rml(overflow));
        set_property(Rml::PropertyId::OverflowY, to_rml(overflow));
    }

    void Style::set_overflow_x(Overflow overflow) {
        set_property(Rml::PropertyId::OverflowX, to_rml(overflow));
    }

    void Style::set_overflow_y(Overflow overflow) {
        set_property(Rml::PropertyId::OverflowY, to_rml(overflow));
    }

    void Style::set_font_size(float size, Unit unit) {
        set_property(Rml::PropertyId::FontSize, Rml::Property(size, to_rml(unit)));
    }

    void Style::set_letter_spacing(float spacing, Unit unit) {
        set_property(Rml::PropertyId::LetterSpacing, Rml::Property(spacing, to_rml(unit)));
    }

    void Style::set_line_height(float height, Unit unit) {
        set_property(Rml::PropertyId::LineHeight, Rml::Property(height, to_rml(unit)));
    }

    void Style::set_font_style(FontStyle style) {
        switch (style) {
        case FontStyle::Normal:
            set_property(Rml::PropertyId::FontStyle, Rml::Style::FontStyle::Normal);
            break;
        case FontStyle::Italic:
            set_property(Rml::PropertyId::FontStyle, Rml::Style::FontStyle::Italic);
            break;
        default:
            assert(false && "Unknown font style.");
            break;
        }
    }

    void Style::set_font_weight(uint32_t weight) {
        set_property(Rml::PropertyId::FontWeight, Rml::Style::FontWeight(weight));
    }

    void Style::set_text_align(TextAlign text_align) {
        set_property(Rml::PropertyId::TextAlign, to_rml(text_align));
    }

    void Style::set_text_transform(TextTransform text_transform) {
        set_property(Rml::PropertyId::TextTransform, to_rml(text_transform));
    }

    void Style::set_gap(float size, Unit unit) {
        set_row_gap(size, unit);
        set_column_gap(size, unit);
    }

    void Style::set_row_gap(float size, Unit unit) {
        set_property(Rml::PropertyId::RowGap, Rml::Property(size, to_rml(unit)));
    }

    void Style::set_column_gap(float size, Unit unit) {
        set_property(Rml::PropertyId::ColumnGap, Rml::Property(size, to_rml(unit)));
    }

    void Style::set_drag(Drag drag) {
        set_property(Rml::PropertyId::Drag, to_rml(drag));
    }

    void Style::set_tab_index(TabIndex tab_index) {
        set_property(Rml::PropertyId::TabIndex, to_rml(tab_index));
    }

    void Style::set_font_family(std::string_view family) {
        set_property(Rml::PropertyId::FontFamily, Rml::Property(Rml::String{ family }, Rml::Unit::UNKNOWN));
    }
    
    void Style::set_nav_auto(NavDirection dir) {
        set_property(nav_to_property(dir), Rml::Style::Nav::Auto);
    }

    void Style::set_nav_none(NavDirection dir) {
        set_property(nav_to_property(dir), Rml::Style::Nav::None);
    }

    void Style::set_nav(NavDirection dir, Element* element) {
        set_property(nav_to_property(dir), Rml::Property(Rml::String{ "#" + element->get_id() }, Rml::Unit::STRING));
    }

    void Style::set_nav_manual(NavDirection dir, const std::string& target) {
        set_property(nav_to_property(dir), Rml::Property(target, Rml::Unit::STRING));
    }

    void Style::set_tab_index_auto() {
        set_property(Rml::PropertyId::TabIndex, Rml::Style::Nav::Auto);
    }

    void Style::set_tab_index_none() {
        set_property(Rml::PropertyId::TabIndex, Rml::Style::Nav::None);
    }
    
    void Style::set_focusable(bool focusable) {
        set_property(Rml::PropertyId::Focus, focusable ? Rml::Style::Focus::Auto : Rml::Style::Focus::None);
    }


} // namespace recompui