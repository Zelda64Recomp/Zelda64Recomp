#ifndef __RECOMP_HELPERS__
#define __RECOMP_HELPERS__

#include "recomp.h"
#include "../ultramodern/ultra64.h"

template<int index, typename T>
T _arg(uint8_t* rdram, recomp_context* ctx) {
    static_assert(index < 4, "Only args 0 through 3 supported");
    gpr raw_arg = (&ctx->r4)[index];
    if constexpr (std::is_same_v<T, float>) {
        if constexpr (index < 2) {
            static_assert(index != 1, "Floats in arg 1 not supported");
            return ctx->f12.fl;
        }
        else {
            // static_assert in else workaround
            [] <bool flag = false>() {
                static_assert(flag, "Floats in a2/a3 not supported");
            }();
        }
    }
    else if constexpr (std::is_pointer_v<T>) {
        static_assert (!std::is_pointer_v<std::remove_pointer_t<T>>, "Double pointers not supported");
        return TO_PTR(std::remove_pointer_t<T>, raw_arg);
    }
    else if constexpr (std::is_integral_v<T>) {
        static_assert(sizeof(T) <= 4, "64-bit args not supported");
        return static_cast<T>(raw_arg);
    }
    else {
        // static_assert in else workaround
        [] <bool flag = false>() {
            static_assert(flag, "Unsupported type");
        }();
    }
}

template <typename T>
void _return(recomp_context* ctx, T val) {
    static_assert(sizeof(T) <= 4 && "Only 32-bit value returns supported currently");
    if (std::is_same_v<T, float>) {
        ctx->f0.fl = val;
    }
    else if (std::is_integral_v<T> && sizeof(T) <= 4) {
        ctx->r2 = int32_t(val);
    }
}

#endif