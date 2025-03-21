#ifndef __UI_HELPERS_H__
#define __UI_HELPERS_H__

#include "librecomp/helpers.hpp"
#include "librecomp/addresses.hpp"

#include "elements/ui_element.h"
#include "elements/ui_types.h"
#include "core/ui_context.h"
#include "core/ui_resource.h"

namespace recompui {

constexpr ResourceId root_element_id{ 0xFFFFFFFE };

inline ContextId get_context(uint8_t* rdram, recomp_context* ctx) {
    uint32_t context_id = _arg<0, uint32_t>(rdram, ctx);
    return ContextId{ .slot_id = context_id };
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
    else if (resource == root_element_id) {
        ContextId ui_context = recompui::get_current_context();
        return ui_context.get_root_element();
    }

    return *resource;
}

template <int arg_index>
Color arg_color(uint8_t* rdram, recomp_context* ctx) {
    PTR(u8) color_arg = _arg<arg_index, PTR(u8)>(rdram, ctx);

    Color ret{};

    ret.r = MEM_B(0, color_arg);
    ret.g = MEM_B(1, color_arg);
    ret.b = MEM_B(2, color_arg);
    ret.a = MEM_B(3, color_arg);

    return ret;
}

inline void return_resource(recomp_context* ctx, ResourceId resource) {
    _return<uint32_t>(ctx, resource.slot_id);
}

inline void return_string(uint8_t* rdram, recomp_context* ctx, const std::string& ret) {
    gpr addr = (reinterpret_cast<uint8_t*>(recomp::alloc(rdram, ret.size() + 1)) - rdram) + 0xFFFFFFFF80000000ULL;

    for (size_t i = 0; i < ret.size(); i++) {
        MEM_B(i, addr) = ret[i];
    }
    MEM_B(ret.size(), addr) = '\x00';
    
    _return<PTR(char)>(ctx, addr);
}
}

#endif