#pragma once

#include <string_view>

#include "RmlUi/Core.h"

#include "../core/ui_resource.h"
#include "ui_types.h"

namespace recompui {
    class ContextId;
    class Style {
        friend class Element; // For access to property_map without making it visible to element subclasses.
        friend class ContextId;
    private:
        std::map<Rml::PropertyId, Rml::Property> property_map;
    protected:
        virtual void set_property(Rml::PropertyId property_id, const Rml::Property &property);
        ResourceId resource_id = ResourceId::null();
    public:
        Style();
        virtual ~Style();
        void set_visibility(Visibility visibility);
        void set_position(Position position);
        void set_left(float left, Unit unit = Unit::Dp);
        void set_top(float top, Unit unit = Unit::Dp);
        void set_right(float right, Unit unit = Unit::Dp);
        void set_bottom(float bottom, Unit unit = Unit::Dp);
        void set_width(float width, Unit unit = Unit::Dp);
        void set_width_auto();
        void set_height(float height, Unit unit = Unit::Dp);
        void set_height_auto();
        void set_min_width(float width, Unit unit = Unit::Dp);
        void set_min_height(float height, Unit unit = Unit::Dp);
        void set_max_width(float width, Unit unit = Unit::Dp);
        void set_max_height(float height, Unit unit = Unit::Dp);
        void set_padding(float padding, Unit unit = Unit::Dp);
        void set_padding_left(float padding, Unit unit = Unit::Dp);
        void set_padding_top(float padding, Unit unit = Unit::Dp);
        void set_padding_right(float padding, Unit unit = Unit::Dp);
        void set_padding_bottom(float padding, Unit unit = Unit::Dp);
        void set_margin(float margin, Unit unit = Unit::Dp);
        void set_margin_left(float margin, Unit unit = Unit::Dp);
        void set_margin_top(float margin, Unit unit = Unit::Dp);
        void set_margin_right(float margin, Unit unit = Unit::Dp);
        void set_margin_bottom(float margin, Unit unit = Unit::Dp);
        void set_margin_auto();
        void set_margin_left_auto();
        void set_margin_top_auto();
        void set_margin_right_auto();
        void set_margin_bottom_auto();
        void set_border_width(float width, Unit unit = Unit::Dp);
        void set_border_left_width(float width, Unit unit = Unit::Dp);
        void set_border_top_width(float width, Unit unit = Unit::Dp);
        void set_border_right_width(float width, Unit unit = Unit::Dp);
        void set_border_bottom_width(float width, Unit unit = Unit::Dp);
        void set_border_radius(float radius, Unit unit = Unit::Dp);
        void set_border_top_left_radius(float radius, Unit unit = Unit::Dp);
        void set_border_top_right_radius(float radius, Unit unit = Unit::Dp);
        void set_border_bottom_left_radius(float radius, Unit unit = Unit::Dp);
        void set_border_bottom_right_radius(float radius, Unit unit = Unit::Dp);
        void set_background_color(const Color &color);
        void set_border_color(const Color &color);
        void set_border_left_color(const Color &color);
        void set_border_top_color(const Color &color);
        void set_border_right_color(const Color &color);
        void set_border_bottom_color(const Color &color);
        void set_color(const Color &color);
        void set_cursor(Cursor cursor);
        void set_opacity(float opacity);
        void set_display(Display display);
        void set_justify_content(JustifyContent justify_content);
        void set_flex_grow(float grow);
        void set_flex_shrink(float shrink);
        void set_flex_basis_auto();
        void set_flex_basis(float basis, Unit unit = Unit::Percent);
        void set_flex(float grow, float shrink);
        void set_flex(float grow, float shrink, float basis, Unit basis_unit = Unit::Percent);
        void set_flex_direction(FlexDirection flex_direction);
        void set_align_items(AlignItems align_items);
        void set_overflow(Overflow overflow);
        void set_overflow_x(Overflow overflow);
        void set_overflow_y(Overflow overflow);
        void set_font_size(float size, Unit unit = Unit::Dp);
        void set_letter_spacing(float spacing, Unit unit = Unit::Dp);
        void set_line_height(float height, Unit unit = Unit::Dp);
        void set_font_style(FontStyle style);
        void set_font_weight(uint32_t weight);
        void set_text_align(TextAlign text_align);
        void set_text_transform(TextTransform text_transform);
        void set_gap(float size, Unit unit = Unit::Dp);
        void set_row_gap(float size, Unit unit = Unit::Dp);
        void set_column_gap(float size, Unit unit = Unit::Dp);
        void set_drag(Drag drag);
        void set_tab_index(TabIndex focus);
        void set_font_family(std::string_view family);
        virtual void set_nav_auto(NavDirection dir);
        virtual void set_nav_none(NavDirection dir);
        virtual void set_nav(NavDirection dir, Element* element);
        virtual void set_nav_manual(NavDirection dir, const std::string& target);
        void set_tab_index_auto();
        void set_tab_index_none();
        void set_focusable(bool focusable);
        virtual bool is_element() { return false; }
        ResourceId get_resource_id() { return resource_id; }
    };

} // namespace recompui