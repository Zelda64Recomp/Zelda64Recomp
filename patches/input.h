#ifndef __INPUT_H__
#define __INPUT_H__

#ifdef MIPS
#include "ultra64.h"
#else
#include "recomp.h"
#endif

typedef enum {
    RECOMP_CAMERA_NORMAL,
    RECOMP_CAMERA_DUALANALOG,
} RecompCameraMode;

extern RecompCameraMode recomp_camera_mode;

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

enum RecompDigitalInput {
    RECOMP_DIGITAL_INPUT_ITEM1,
    RECOMP_DIGITAL_INPUT_ITEM2,
    RECOMP_DIGITAL_INPUT_ITEM3,
    RECOMP_DIGITAL_INPUT_MAX
};

enum RecompAnalogInput {
    RECOMP_ANALOG_INPUT_MOVEMENT_X,
    RECOMP_ANALOG_INPUT_MOVEMENT_Y,
    RECOMP_ANALOG_INPUT_CAMERA_X,
    RECOMP_ANALOG_INPUT_CAMERA_Y,
    RECOMP_ANALOG_INPUT_MAX,
};

DECLARE_FUNC(u32, recomp_get_digital_input, u32 which);
DECLARE_FUNC(float, recomp_get_analog_input, u32 which);
DECLARE_FUNC(void, recomp_get_item_inputs, u32* buttons);
DECLARE_FUNC(void, recomp_get_camera_inputs, float* x_out, float* y_out);
// TODO move these
DECLARE_FUNC(void, recomp_puts, const char* data, u32 size);
DECLARE_FUNC(void, recomp_exit);

#ifdef __cplusplus
}
#endif

#endif
