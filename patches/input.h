#ifndef __INPUT_H__
#define __INPUT_H__

#ifdef MIPS
#include "ultra64.h"
#else
#include "recomp.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MIPS
#   define DECLARE_FUNC(type, name, ...) \
        type name(__VA_ARGS__);
#else
#   define DECLARE_FUNC(type, name, ...) \
        void name(uint8_t* rdram, recomp_context* ctx);
#endif

DECLARE_FUNC(void, recomp_get_item_inputs, u32* buttons);
// TODO move this
DECLARE_FUNC(void, recomp_puts, const char* data, u32 size);

#ifdef __cplusplus
}
#endif

#endif
