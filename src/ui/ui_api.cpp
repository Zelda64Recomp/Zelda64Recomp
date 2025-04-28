#include "recomp_ui.h"

#include "ui_helpers.h"
#include "ui_api_images.h"

#include "core/ui_context.h"
#include "core/ui_resource.h"

#include "elements/ui_element.h"
#include "elements/ui_button.h"
#include "elements/ui_clickable.h"
#include "elements/ui_container.h"
#include "elements/ui_image.h"
#include "elements/ui_label.h"
#include "elements/ui_radio.h"
#include "elements/ui_scroll_container.h"
#include "elements/ui_slider.h"
#include "elements/ui_span.h"
#include "elements/ui_style.h"
#include "elements/ui_text_input.h"
#include "elements/ui_toggle.h"
#include "elements/ui_types.h"

#include "librecomp/overlays.hpp"
#include "librecomp/helpers.hpp"
#include "librecomp/addresses.hpp"
#include "ultramodern/error_handling.hpp"

using namespace recompui;

// Contexts
void recompui_create_context(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    ContextId ui_context = create_context();
    
    _return<uint32_t>(ctx, ui_context.slot_id);
}

void recompui_open_context(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = get_context(rdram, ctx);

    ui_context.open();
}

void recompui_close_context(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = get_context(rdram, ctx);

    ui_context.close();
}

void recompui_context_root(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = get_context(rdram, ctx);
    (void)ui_context;

    return_resource(ctx, root_element_id);
}

void recompui_show_context(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = get_context(rdram, ctx);

    recompui::show_context(ui_context, "");
}

void recompui_hide_context(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = get_context(rdram, ctx);

    recompui::hide_context(ui_context);
}

void recompui_set_context_captures_input(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = get_context(rdram, ctx);
    bool captures_input = _arg<1, int>(rdram, ctx) != 0;

    ui_context.set_captures_input(captures_input);
}

void recompui_set_context_captures_mouse(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = get_context(rdram, ctx);
    bool captures_mouse = _arg<1, int>(rdram, ctx) != 0;

    ui_context.set_captures_mouse(captures_mouse);
}

// Resources
void recompui_create_style(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = get_context(rdram, ctx);

    Style* ret = ui_context.create_style();
    return_resource(ctx, ret->get_resource_id());
}

void recompui_create_element(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = get_context(rdram, ctx);
    Element* parent = arg_element<1>(rdram, ctx, ui_context);

    Element* ret = ui_context.create_element<Element>(parent);
    return_resource(ctx, ret->get_resource_id());
}

void recompui_destroy_element(uint8_t* rdram, recomp_context* ctx) {
    Style* parent_resource = arg_style<0>(rdram, ctx);

    if (!parent_resource->is_element()) {
        recompui::message_box("Fatal error in mod - attempted to remove child from non-element or element not found in context");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }

    Element* parent = static_cast<Element*>(parent_resource);
    ResourceId to_remove = arg_resource_id<1>(rdram, ctx);

    if (!parent->remove_child(to_remove)) {
        recompui::message_box("Fatal error in mod - attempted to remove child from wrong parent");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }
}

void recompui_create_button(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = get_context(rdram, ctx);
    Element* parent = arg_element<1>(rdram, ctx, ui_context);
    std::string text = _arg_string<2>(rdram, ctx);
    uint32_t style = _arg<3, uint32_t>(rdram, ctx);

    Button* ret = ui_context.create_element<Button>(parent, text, static_cast<ButtonStyle>(style));
    return_resource(ctx, ret->get_resource_id());
}

void recompui_create_label(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = get_context(rdram, ctx);
    Element* parent = arg_element<1>(rdram, ctx, ui_context);
    std::string text = _arg_string<2>(rdram, ctx);
    uint32_t style = _arg<3, uint32_t>(rdram, ctx);

    Element* ret = ui_context.create_element<Label>(parent, text, static_cast<LabelStyle>(style));
    return_resource(ctx, ret->get_resource_id());
}

void recompui_create_span(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = get_context(rdram, ctx);
    Element* parent = arg_element<1>(rdram, ctx, ui_context);
    std::string text = _arg_string<2>(rdram, ctx);

    Element* ret = ui_context.create_element<Span>(parent, text);
    return_resource(ctx, ret->get_resource_id());
}

void recompui_create_textinput(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = get_context(rdram, ctx);
    Element* parent = arg_element<1>(rdram, ctx, ui_context);

    Element* ret = ui_context.create_element<TextInput>(parent);
    return_resource(ctx, ret->get_resource_id());
}

void recompui_create_passwordinput(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = get_context(rdram, ctx);
    Element* parent = arg_element<1>(rdram, ctx, ui_context);

    Element* ret = ui_context.create_element<TextInput>(parent, false);
    return_resource(ctx, ret->get_resource_id());
}

void recompui_create_labelradio(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = get_context(rdram, ctx);
    Element* parent = arg_element<1>(rdram, ctx, ui_context);
    PTR(PTR(char)) options = _arg<2, PTR(PTR(char))>(rdram, ctx);
    uint32_t num_options = _arg<3, uint32_t>(rdram, ctx);

    Radio* ret = ui_context.create_element<Radio>(parent);

    for (size_t i = 0; i < num_options; i++) {
        ret->add_option(decode_string(rdram, MEM_W(sizeof(uint32_t) * i, options)));
    }

    return_resource(ctx, ret->get_resource_id());
}

void recompui_create_slider(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = get_context(rdram, ctx);
    Element* parent = arg_element<1>(rdram, ctx, ui_context);
    uint32_t type = _arg<2, uint32_t>(rdram, ctx);
    float min_value = arg_float3(rdram, ctx);
    float max_value = arg_float4(rdram, ctx);
    float step = arg_float5(rdram, ctx);
    float initial_value = arg_float6(rdram, ctx);

    Slider* ret = ui_context.create_element<Slider>(parent, static_cast<SliderType>(type));
    ret->set_min_value(min_value);
    ret->set_max_value(max_value);
    ret->set_step_value(step);
    ret->set_value(initial_value);
    return_resource(ctx, ret->get_resource_id());
}

// Position and Layout
void recompui_set_visibility(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    uint32_t visibility = _arg<1, uint32_t>(rdram, ctx);

    resource->set_visibility(static_cast<Visibility>(visibility));
}

void recompui_set_position(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    uint32_t position = _arg<1, uint32_t>(rdram, ctx);

    resource->set_position(static_cast<Position>(position));
}

void recompui_set_left(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float left = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_left(left, static_cast<Unit>(unit));
}

void recompui_set_top(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float top = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_top(top, static_cast<Unit>(unit));
}

void recompui_set_right(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float right = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_right(right, static_cast<Unit>(unit));
}

void recompui_set_bottom(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float bottom = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_bottom(bottom, static_cast<Unit>(unit));
}

// Sizing
void recompui_set_width(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float width = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_width(width, static_cast<Unit>(unit));
}

void recompui_set_width_auto(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);

    resource->set_width_auto();
}

void recompui_set_height(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float height = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_height(height, static_cast<Unit>(unit));
}

void recompui_set_height_auto(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);

    resource->set_height_auto();
}

void recompui_set_min_width(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float width = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_min_width(width, static_cast<Unit>(unit));
}

void recompui_set_min_height(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float height = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_min_height(height, static_cast<Unit>(unit));
}

void recompui_set_max_width(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float width = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_max_width(width, static_cast<Unit>(unit));
}

void recompui_set_max_height(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float height = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_max_height(height, static_cast<Unit>(unit));
}

// Padding
void recompui_set_padding(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float padding = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_padding(padding, static_cast<Unit>(unit));
}

void recompui_set_padding_left(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float padding = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_padding_left(padding, static_cast<Unit>(unit));
}

void recompui_set_padding_top(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float padding = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_padding_top(padding, static_cast<Unit>(unit));
}

void recompui_set_padding_right(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float padding = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_padding_right(padding, static_cast<Unit>(unit));
}

void recompui_set_padding_bottom(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float padding = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_padding_bottom(padding, static_cast<Unit>(unit));
}

// Margins
void recompui_set_margin(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float margin = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_margin(margin, static_cast<Unit>(unit));
}

void recompui_set_margin_left(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float margin = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_margin_left(margin, static_cast<Unit>(unit));
}

void recompui_set_margin_top(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float margin = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_margin_top(margin, static_cast<Unit>(unit));
}

void recompui_set_margin_right(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float margin = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_margin_right(margin, static_cast<Unit>(unit));
}

void recompui_set_margin_bottom(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float margin = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_margin_bottom(margin, static_cast<Unit>(unit));
}

void recompui_set_margin_auto(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);

    resource->set_margin_auto();
}

void recompui_set_margin_left_auto(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);

    resource->set_margin_left_auto();
}

void recompui_set_margin_top_auto(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);

    resource->set_margin_top_auto();
}

void recompui_set_margin_right_auto(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);

    resource->set_margin_right_auto();
}

void recompui_set_margin_bottom_auto(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);

    resource->set_margin_bottom_auto();
}

// Borders
void recompui_set_border_width(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float width = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_border_width(width, static_cast<Unit>(unit));
}

void recompui_set_border_left_width(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float width = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_border_left_width(width, static_cast<Unit>(unit));
}

void recompui_set_border_top_width(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float width = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_border_top_width(width, static_cast<Unit>(unit));
}

void recompui_set_border_right_width(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float width = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_border_right_width(width, static_cast<Unit>(unit));
}

void recompui_set_border_bottom_width(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float width = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_border_bottom_width(width, static_cast<Unit>(unit));
}

void recompui_set_border_radius(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float radius = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_border_radius(radius, static_cast<Unit>(unit));
}

void recompui_set_border_top_left_radius(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float radius = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_border_top_left_radius(radius, static_cast<Unit>(unit));
}

void recompui_set_border_top_right_radius(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float radius = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_border_top_right_radius(radius, static_cast<Unit>(unit));
}

void recompui_set_border_bottom_left_radius(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float radius = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_border_bottom_left_radius(radius, static_cast<Unit>(unit));
}

void recompui_set_border_bottom_right_radius(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float radius = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_border_bottom_right_radius(radius, static_cast<Unit>(unit));
}

// Colors
void recompui_set_background_color(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    Color color = arg_color<1>(rdram, ctx);

    resource->set_background_color(color);
}

void recompui_set_border_color(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    Color color = arg_color<1>(rdram, ctx);

    resource->set_border_color(color);

}

void recompui_set_border_left_color(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    Color color = arg_color<1>(rdram, ctx);

    resource->set_border_left_color(color);
}

void recompui_set_border_top_color(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    Color color = arg_color<1>(rdram, ctx);

    resource->set_border_top_color(color);
}

void recompui_set_border_right_color(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    Color color = arg_color<1>(rdram, ctx);

    resource->set_border_right_color(color);
}

void recompui_set_border_bottom_color(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    Color color = arg_color<1>(rdram, ctx);

    resource->set_border_bottom_color(color);
}

void recompui_set_color(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    Color color = arg_color<1>(rdram, ctx);

    resource->set_color(color);
}

// Cursor and Display
void recompui_set_cursor(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    uint32_t cursor = _arg<1, uint32_t>(rdram, ctx);

    resource->set_cursor(static_cast<Cursor>(cursor));
}

void recompui_set_opacity(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float opacity = _arg_float_a1(rdram, ctx);

    resource->set_opacity(opacity);
}

void recompui_set_display(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    uint32_t display = _arg<1, uint32_t>(rdram, ctx);

    resource->set_display(static_cast<Display>(display));
}

// Flexbox
void recompui_set_justify_content(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    uint32_t justify_content = _arg<1, uint32_t>(rdram, ctx);

    resource->set_justify_content(static_cast<JustifyContent>(justify_content));
}

void recompui_set_flex_grow(uint8_t* rdram, recomp_context* ctx) { // float grow
    Style* resource = arg_style<0>(rdram, ctx);
    float grow = _arg_float_a1(rdram, ctx);

    resource->set_flex_grow(grow);
}

void recompui_set_flex_shrink(uint8_t* rdram, recomp_context* ctx) { // float shrink
    Style* resource = arg_style<0>(rdram, ctx);
    float shrink = _arg_float_a1(rdram, ctx);

    resource->set_flex_shrink(shrink);
}

void recompui_set_flex_basis_auto(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);

    resource->set_flex_basis_auto();
}

void recompui_set_flex_basis(uint8_t* rdram, recomp_context* ctx) { // float basis, Unit unit = Unit::Percent
    Style* resource = arg_style<0>(rdram, ctx);
    float basis = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_flex_basis(basis, static_cast<Unit>(unit));
}

void recompui_set_flex_direction(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    uint32_t direction = _arg<1, uint32_t>(rdram, ctx);

    resource->set_flex_direction(static_cast<FlexDirection>(direction));
}

void recompui_set_align_items(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    uint32_t align_items = _arg<1, uint32_t>(rdram, ctx);

    resource->set_align_items(static_cast<AlignItems>(align_items));
}

// Overflow
void recompui_set_overflow(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    uint32_t overflow = _arg<1, uint32_t>(rdram, ctx);

    resource->set_overflow(static_cast<Overflow>(overflow));
}

void recompui_set_overflow_x(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    uint32_t overflow = _arg<1, uint32_t>(rdram, ctx);

    resource->set_overflow_x(static_cast<Overflow>(overflow));
}

void recompui_set_overflow_y(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    uint32_t overflow = _arg<1, uint32_t>(rdram, ctx);

    resource->set_overflow_y(static_cast<Overflow>(overflow));
}

// Text and Fonts
void recompui_set_text(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);

    if (resource == nullptr || !resource->is_element()) {
        recompui::message_box("Fatal error in mod - attempted to set text of non-element or element not found in context");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }

    Element* element = static_cast<Element*>(resource);
    element->set_text(_arg_string<1>(rdram, ctx));
}

void recompui_set_font_size(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float size = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_font_size(size, static_cast<Unit>(unit));
}

void recompui_set_letter_spacing(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float spacing = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_letter_spacing(spacing, static_cast<Unit>(unit));
}

void recompui_set_line_height(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float height = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_line_height(height, static_cast<Unit>(unit));
}

void recompui_set_font_style(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    uint32_t style = _arg<1, uint32_t>(rdram, ctx);

    resource->set_font_style(static_cast<FontStyle>(style));
}

void recompui_set_font_weight(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    int32_t weight = _arg<1, int32_t>(rdram, ctx);

    resource->set_font_weight(weight);
}

void recompui_set_text_align(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    uint32_t text_align = _arg<1, uint32_t>(rdram, ctx);

    resource->set_text_align(static_cast<TextAlign>(text_align));
}

// Gaps
void recompui_set_gap(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float size = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_gap(size, static_cast<Unit>(unit));
}

void recompui_set_row_gap(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float size = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_row_gap(size, static_cast<Unit>(unit));

}

void recompui_set_column_gap(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float size = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_column_gap(size, static_cast<Unit>(unit));
}

// Drag and Focus
void recompui_set_drag(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    uint32_t drag = _arg<1, uint32_t>(rdram, ctx);

    resource->set_drag(static_cast<Drag>(drag));
}

void recompui_set_tab_index(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    uint32_t tab_index = _arg<1, uint32_t>(rdram, ctx);

    resource->set_tab_index(static_cast<TabIndex>(tab_index));
}

// Values
void recompui_get_input_value_u32(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);

    if (resource == nullptr || !resource->is_element()) {
        recompui::message_box("Fatal error in mod - attempted to get value of non-element or element not found in context");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }
    
    Element* element = static_cast<Element*>(resource);
    _return<uint32_t>(ctx, element->get_input_value_u32());
}

void recompui_get_input_value_float(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);

    if (resource == nullptr || !resource->is_element()) {
        recompui::message_box("Fatal error in mod - attempted to get value of non-element or element not found in context");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }
    
    Element* element = static_cast<Element*>(resource);
    _return<float>(ctx, element->get_input_value_float());
}

void recompui_get_input_text(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);

    if (resource == nullptr || !resource->is_element()) {
        recompui::message_box("Fatal error in mod - attempted to get input text of non-element or element not found in context");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }
    
    Element* element = static_cast<Element*>(resource);
    std::string ret = element->get_input_text();
    return_string(rdram, ctx, ret);
}

void recompui_set_input_value_u32(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    uint32_t value = _arg<1, uint32_t>(rdram, ctx);

    if (resource == nullptr || !resource->is_element()) {
        recompui::message_box("Fatal error in mod - attempted to set value of non-element or element not found in context");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }
    
    Element* element = static_cast<Element*>(resource);
    element->set_input_value_u32(value);
}

void recompui_set_input_value_float(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float value = _arg_float_a1(rdram, ctx);

    if (resource == nullptr || !resource->is_element()) {
        recompui::message_box("Fatal error in mod - attempted to set value of non-element or element not found in context");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }
    
    Element* element = static_cast<Element*>(resource);
    element->set_input_value_float(value);
}

void recompui_set_input_text(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);

    if (resource == nullptr || !resource->is_element()) {
        recompui::message_box("Fatal error in mod - attempted to set input text of non-element or element not found in context");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }

    Element* element = static_cast<Element*>(resource);
    element->set_input_text(_arg_string<1>(rdram, ctx));
}

// Callbacks
void recompui_register_callback(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = recompui::get_current_context();

    if (ui_context == ContextId::null()) {
        recompui::message_box("Fatal error in mod - attempted to register callback with no active context");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }

    Style* resource = arg_style<0>(rdram, ctx);

    if (resource == nullptr || !resource->is_element()) {
        recompui::message_box("Fatal error in mod - attempted to register callback on non-element or element not found in context");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }

    Element* element = static_cast<Element*>(resource);
    PTR(void) callback = _arg<1, PTR(void)>(rdram, ctx);
    PTR(void) userdata = _arg<2, PTR(void)>(rdram, ctx);

    element->register_callback(ui_context, callback, userdata);
}

// Navigation
void recompui_set_nav_auto(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = recompui::get_current_context();

    if (ui_context == ContextId::null()) {
        recompui::message_box("Fatal error in mod - attempted to set element navigation with no active context");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }

    Style* resource = arg_style<0>(rdram, ctx);

    if (resource == nullptr || !resource->is_element()) {
        recompui::message_box("Fatal error in mod - attempted to set navigation on non-element or element not found in context");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }

    Element* element = static_cast<Element*>(resource);
    u32 nav_dir = _arg<1, u32>(rdram, ctx);

    element->set_nav_auto(static_cast<recompui::NavDirection>(nav_dir));
}

void recompui_set_nav_none(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = recompui::get_current_context();

    if (ui_context == ContextId::null()) {
        recompui::message_box("Fatal error in mod - attempted to set element navigation with no active context");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }

    Style* resource = arg_style<0>(rdram, ctx);

    if (resource == nullptr || !resource->is_element()) {
        recompui::message_box("Fatal error in mod - attempted to set navigation on non-element or element not found in context");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }

    Element* element = static_cast<Element*>(resource);
    u32 nav_dir = _arg<1, u32>(rdram, ctx);

    element->set_nav_none(static_cast<recompui::NavDirection>(nav_dir));
}

void recompui_set_nav(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = recompui::get_current_context();

    if (ui_context == ContextId::null()) {
        recompui::message_box("Fatal error in mod - attempted to set element navigation with no active context");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }

    Style* resource = arg_style<0>(rdram, ctx);

    if (resource == nullptr || !resource->is_element()) {
        recompui::message_box("Fatal error in mod - attempted to set navigation on non-element or element not found in context");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }

    Style* target_resource = arg_style<2>(rdram, ctx);

    if (target_resource == nullptr || !target_resource->is_element()) {
        recompui::message_box("Fatal error in mod - attempted to set element navigation to non-element or target element not found in context");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }

    Element* element = static_cast<Element*>(resource);
    Element* target_element = static_cast<Element*>(target_resource);
    u32 nav_dir = _arg<1, u32>(rdram, ctx);

    element->set_nav(static_cast<recompui::NavDirection>(nav_dir), target_element);
}

#define REGISTER_FUNC(name) recomp::overlays::register_base_export(#name, name)

void recompui::register_ui_exports() {
    REGISTER_FUNC(recompui_create_context);
    REGISTER_FUNC(recompui_open_context);
    REGISTER_FUNC(recompui_close_context);
    REGISTER_FUNC(recompui_context_root);
    REGISTER_FUNC(recompui_show_context);
    REGISTER_FUNC(recompui_hide_context);
    REGISTER_FUNC(recompui_set_context_captures_input);
    REGISTER_FUNC(recompui_set_context_captures_mouse);
    REGISTER_FUNC(recompui_create_style);
    REGISTER_FUNC(recompui_create_element);
    REGISTER_FUNC(recompui_destroy_element);
    REGISTER_FUNC(recompui_create_button);
    REGISTER_FUNC(recompui_create_label);
    // REGISTER_FUNC(recompui_create_span);
    REGISTER_FUNC(recompui_create_textinput);
    REGISTER_FUNC(recompui_create_passwordinput);
    REGISTER_FUNC(recompui_create_labelradio);
    REGISTER_FUNC(recompui_create_slider);
    REGISTER_FUNC(recompui_set_visibility);
    REGISTER_FUNC(recompui_set_position);
    REGISTER_FUNC(recompui_set_left);
    REGISTER_FUNC(recompui_set_top);
    REGISTER_FUNC(recompui_set_right);
    REGISTER_FUNC(recompui_set_bottom);
    REGISTER_FUNC(recompui_set_width);
    REGISTER_FUNC(recompui_set_width_auto);
    REGISTER_FUNC(recompui_set_height);
    REGISTER_FUNC(recompui_set_height_auto);
    REGISTER_FUNC(recompui_set_min_width);
    REGISTER_FUNC(recompui_set_min_height);
    REGISTER_FUNC(recompui_set_max_width);
    REGISTER_FUNC(recompui_set_max_height);
    REGISTER_FUNC(recompui_set_padding);
    REGISTER_FUNC(recompui_set_padding_left);
    REGISTER_FUNC(recompui_set_padding_top);
    REGISTER_FUNC(recompui_set_padding_right);
    REGISTER_FUNC(recompui_set_padding_bottom);
    REGISTER_FUNC(recompui_set_margin);
    REGISTER_FUNC(recompui_set_margin_left);
    REGISTER_FUNC(recompui_set_margin_top);
    REGISTER_FUNC(recompui_set_margin_right);
    REGISTER_FUNC(recompui_set_margin_bottom);
    REGISTER_FUNC(recompui_set_margin_auto);
    REGISTER_FUNC(recompui_set_margin_left_auto);
    REGISTER_FUNC(recompui_set_margin_top_auto);
    REGISTER_FUNC(recompui_set_margin_right_auto);
    REGISTER_FUNC(recompui_set_margin_bottom_auto);
    REGISTER_FUNC(recompui_set_border_width);
    REGISTER_FUNC(recompui_set_border_left_width);
    REGISTER_FUNC(recompui_set_border_top_width);
    REGISTER_FUNC(recompui_set_border_right_width);
    REGISTER_FUNC(recompui_set_border_bottom_width);
    REGISTER_FUNC(recompui_set_border_radius);
    REGISTER_FUNC(recompui_set_border_top_left_radius);
    REGISTER_FUNC(recompui_set_border_top_right_radius);
    REGISTER_FUNC(recompui_set_border_bottom_left_radius);
    REGISTER_FUNC(recompui_set_border_bottom_right_radius);
    REGISTER_FUNC(recompui_set_background_color);
    REGISTER_FUNC(recompui_set_border_color);
    REGISTER_FUNC(recompui_set_border_left_color);
    REGISTER_FUNC(recompui_set_border_top_color);
    REGISTER_FUNC(recompui_set_border_right_color);
    REGISTER_FUNC(recompui_set_border_bottom_color);
    REGISTER_FUNC(recompui_set_color);
    REGISTER_FUNC(recompui_set_cursor);
    REGISTER_FUNC(recompui_set_opacity);
    REGISTER_FUNC(recompui_set_display);
    REGISTER_FUNC(recompui_set_justify_content);
    REGISTER_FUNC(recompui_set_flex_grow);
    REGISTER_FUNC(recompui_set_flex_shrink);
    REGISTER_FUNC(recompui_set_flex_basis_auto);
    REGISTER_FUNC(recompui_set_flex_basis);
    REGISTER_FUNC(recompui_set_flex_direction);
    REGISTER_FUNC(recompui_set_align_items);
    REGISTER_FUNC(recompui_set_overflow);
    REGISTER_FUNC(recompui_set_overflow_x);
    REGISTER_FUNC(recompui_set_overflow_y);
    REGISTER_FUNC(recompui_set_text);
    REGISTER_FUNC(recompui_set_font_size);
    REGISTER_FUNC(recompui_set_letter_spacing);
    REGISTER_FUNC(recompui_set_line_height);
    REGISTER_FUNC(recompui_set_font_style);
    REGISTER_FUNC(recompui_set_font_weight);
    REGISTER_FUNC(recompui_set_text_align);
    REGISTER_FUNC(recompui_set_gap);
    REGISTER_FUNC(recompui_set_row_gap);
    REGISTER_FUNC(recompui_set_column_gap);
    REGISTER_FUNC(recompui_set_drag);
    REGISTER_FUNC(recompui_set_tab_index);
    REGISTER_FUNC(recompui_get_input_value_u32);
    REGISTER_FUNC(recompui_get_input_value_float);
    REGISTER_FUNC(recompui_get_input_text);
    REGISTER_FUNC(recompui_set_input_value_u32);
    REGISTER_FUNC(recompui_set_input_value_float);
    REGISTER_FUNC(recompui_set_input_text);
    REGISTER_FUNC(recompui_set_nav_auto);
    REGISTER_FUNC(recompui_set_nav_none);
    REGISTER_FUNC(recompui_set_nav);
    REGISTER_FUNC(recompui_register_callback);
    register_ui_image_exports();
}
