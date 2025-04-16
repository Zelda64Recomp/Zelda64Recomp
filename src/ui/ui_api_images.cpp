#include <mutex>
#include <unordered_set>

#include "recomp_ui.h"
#include "librecomp/overlays.hpp"
#include "librecomp/helpers.hpp"
#include "ultramodern/error_handling.hpp"

#include "ui_helpers.h"
#include "ui_api_images.h"
#include "elements/ui_image.h"

using namespace recompui;

struct {
    std::mutex mutex;
    std::unordered_set<uint32_t> textures{};
    uint32_t textures_created = 0;
} TextureState;

const std::string mod_texture_prefix = "?/mod_api/";

static std::string get_texture_name(uint32_t texture_id) {
    return mod_texture_prefix + std::to_string(texture_id);
}

static uint32_t get_new_texture_id() {
    std::lock_guard lock{TextureState.mutex};
    uint32_t cur_id = TextureState.textures_created++;
    TextureState.textures.emplace(cur_id);

    return cur_id;
}

static void release_texture(uint32_t texture_id) {
    std::string texture_name = get_texture_name(texture_id);
    std::lock_guard lock{TextureState.mutex};

    if (TextureState.textures.erase(texture_id) == 0) {
        recompui::message_box("Fatal error in mod - attempted to destroy texture that doesn't exist!");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }

    recompui::release_image(texture_name);
}

thread_local std::vector<char> swapped_image_bytes;

void recompui_create_texture_rgba32(uint8_t* rdram, recomp_context* ctx) {
    PTR(void) data_in = _arg<0, PTR(void)>(rdram, ctx);
    uint32_t width = _arg<1, uint32_t>(rdram, ctx);
    uint32_t height = _arg<2, uint32_t>(rdram, ctx);
    uint32_t cur_id = get_new_texture_id();

    // The size in bytes of the image's pixel data.
    size_t size_bytes = width * height * 4 * sizeof(uint8_t);
    swapped_image_bytes.resize(size_bytes);

    // Byteswap copy the pixel data.
    for (size_t i = 0; i < size_bytes; i++) {
        swapped_image_bytes[i] = MEM_B(i, data_in);
    }

    // Create a texture name from the ID and queue its bytes.
    std::string texture_name = get_texture_name(cur_id);
    recompui::queue_image_from_bytes_rgba32(texture_name, swapped_image_bytes, width, height);

    // Return the new texture ID.
    _return(ctx, cur_id);
}

void recompui_create_texture_image_bytes(uint8_t* rdram, recomp_context* ctx) {
    PTR(void) data_in = _arg<0, PTR(void)>(rdram, ctx);
    uint32_t size_bytes = _arg<1, u32>(rdram, ctx);
    uint32_t cur_id = get_new_texture_id();

    // The size in bytes of the image's data.
    swapped_image_bytes.resize(size_bytes);

    // Byteswap copy the image's data.
    for (size_t i = 0; i < size_bytes; i++) {
        swapped_image_bytes[i] = MEM_B(i, data_in);
    }

    // Create a texture name from the ID and queue its bytes.
    std::string texture_name = get_texture_name(cur_id);
    recompui::queue_image_from_bytes_file(texture_name, swapped_image_bytes);

    // Return the new texture ID.
    _return(ctx, cur_id);
}

void recompui_destroy_texture(uint8_t* rdram, recomp_context* ctx) {
    uint32_t texture_id = _arg<0, uint32_t>(rdram, ctx);

    release_texture(texture_id);
}

void recompui_create_imageview(uint8_t* rdram, recomp_context* ctx) {
    ContextId ui_context = get_context(rdram, ctx);
    Element* parent = arg_element<1>(rdram, ctx, ui_context);
    uint32_t texture_id = _arg<2, uint32_t>(rdram, ctx);

    Element* ret = ui_context.create_element<Image>(parent, get_texture_name(texture_id));
    return_resource(ctx, ret->get_resource_id());
}

void recompui_set_imageview_texture(uint8_t* rdram, recomp_context* ctx) {
    Style* resource = arg_style<0>(rdram, ctx);
    uint32_t texture_id = _arg<1, uint32_t>(rdram, ctx);

    if (!resource->is_element()) {
        recompui::message_box("Fatal error in mod - attempted to set texture of non-element");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }

    Element* element = static_cast<Element*>(resource);
    element->set_src(get_texture_name(texture_id));
}

#define REGISTER_FUNC(name) recomp::overlays::register_base_export(#name, name)

void recompui::register_ui_image_exports() {
    REGISTER_FUNC(recompui_create_texture_rgba32);
    REGISTER_FUNC(recompui_create_texture_image_bytes);
    REGISTER_FUNC(recompui_destroy_texture);
    REGISTER_FUNC(recompui_create_imageview);
    REGISTER_FUNC(recompui_set_imageview_texture);
}
