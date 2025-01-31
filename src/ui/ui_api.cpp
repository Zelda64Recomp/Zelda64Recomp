#include "recomp_ui.h"

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
#include "elements/ui_style.h"
#include "elements/ui_text_input.h"
#include "elements/ui_toggle.h"
#include "elements/ui_types.h"

#include "librecomp/overlays.hpp"
#include "librecomp/helpers.hpp"

using namespace recompui;

constexpr ResourceId root_element_id{ 0xFFFFFFFE };

// Helpers

ContextId get_context(uint8_t* rdram, recomp_context* ctx) {
    uint32_t context_id = _arg<0, uint32_t>(rdram, ctx);
    return ContextId{ .slot_id = context_id };
}

template <int arg_index>
std::string arg_string(uint8_t* rdram, recomp_context* ctx) {
    PTR(char) str = _arg<arg_index, PTR(char)>(rdram, ctx);

    // Get the length of the byteswapped string.
    size_t len = 0;
    while (MEM_B(str, len) != 0x00) {
        len++;
    }

    std::string ret{};
    ret.reserve(len + 1);

    for (size_t i = 0; i < len; i++) {
        ret += (char)MEM_B(str, i);
    }

    return ret;
}

template <int arg_index>
ResourceId arg_resource_id(uint8_t* rdram, recomp_context* ctx) {
    uint32_t slot_id = _arg<arg_index, uint32_t>(rdram, ctx);
    
    return ResourceId{ .slot_id = slot_id };
}

template <int arg_index>
Element* arg_element(uint8_t* rdram, recomp_context* ctx, ContextId ui_context) {
    ResourceId resource = arg_resource_id<arg_index>(rdram, ctx);

    if (resource == ResourceId::null()) {
        return nullptr;
    }
    else if (resource == root_element_id) {
        return ui_context.get_root_element();
    }

    return resource.as_element();
}

template <int arg_index>
Style* arg_style(uint8_t* rdram, recomp_context* ctx) {
    ResourceId resource = arg_resource_id<arg_index>(rdram, ctx);

    if (resource == ResourceId::null()) {
        return nullptr;
    }

    return *resource;
}

void return_resource(recomp_context* ctx, ResourceId resource) {
    _return<uint32_t>(ctx, resource.slot_id);
}

// Context functions

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

// Resource creation functions

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

void recompui_create_button(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = get_context(rdram, ctx);
    Element* parent = arg_element<1>(rdram, ctx, ui_context);
    std::string text = arg_string<2>(rdram, ctx);
    uint32_t style = _arg<3, uint32_t>(rdram, ctx);

    Button* ret = ui_context.create_element<Button>(parent, text, static_cast<ButtonStyle>(style));
    return_resource(ctx, ret->get_resource_id());
}

// Style functions

void recompui_set_width(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float width = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_width(width, static_cast<Unit>(unit));
}

void recompui_set_height(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    float height = _arg_float_a1(rdram, ctx);
    uint32_t unit = _arg<2, uint32_t>(rdram, ctx);

    resource->set_height(height, static_cast<Unit>(unit));
}

void recompui_set_display(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    uint32_t display = _arg<1, uint32_t>(rdram, ctx);

    resource->set_display(static_cast<Display>(display));
}

void recompui_set_flex_direction(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    uint32_t direction = _arg<1, uint32_t>(rdram, ctx);

    resource->set_flex_direction(static_cast<FlexDirection>(direction));
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

#define REGISTER_FUNC(name) recomp::overlays::register_base_export(#name, name)

void recompui::register_ui_exports() {
    REGISTER_FUNC(recompui_create_context);
    REGISTER_FUNC(recompui_open_context);
    REGISTER_FUNC(recompui_close_context);
    REGISTER_FUNC(recompui_context_root);
    REGISTER_FUNC(recompui_show_context);
    REGISTER_FUNC(recompui_hide_context);
    REGISTER_FUNC(recompui_create_style);
    REGISTER_FUNC(recompui_create_element);
    REGISTER_FUNC(recompui_create_button);
    REGISTER_FUNC(recompui_set_width);
    REGISTER_FUNC(recompui_set_height);
    REGISTER_FUNC(recompui_set_display);
    REGISTER_FUNC(recompui_set_flex_direction);
    REGISTER_FUNC(recompui_set_flex_grow);
    REGISTER_FUNC(recompui_set_flex_shrink);
    REGISTER_FUNC(recompui_set_flex_basis_auto);
    REGISTER_FUNC(recompui_set_flex_basis);
}
