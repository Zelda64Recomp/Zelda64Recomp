#ifndef __INPUT_H__
#define __INPUT_H__

#include "patch_helpers.h"

typedef enum {
    RECOMP_CAMERA_NORMAL,
    RECOMP_CAMERA_DUALANALOG,
} RecompCameraMode;

typedef enum {
    RECOMP_AIMING_OVERRIDE_OFF = 0,
    RECOMP_AIMING_OVERRIDE_DISABLE_LEFT_STICK = 1,
    RECOMP_AIMING_OVERRIDE_FORCE_RIGHT_STICK = 2
} RecompAimingOverideMode;

extern RecompCameraMode recomp_camera_mode;
extern RecompAimingOverideMode recomp_aiming_override_mode;

DECLARE_FUNC(void, recomp_get_gyro_deltas, float* x, float* y);
DECLARE_FUNC(void, recomp_get_mouse_deltas, float* x, float* y);
DECLARE_FUNC(s32, recomp_get_targeting_mode);
DECLARE_FUNC(void, recomp_get_inverted_axes, s32* x, s32* y);
DECLARE_FUNC(s32, recomp_get_analog_cam_enabled);
DECLARE_FUNC(void, recomp_get_analog_inverted_axes, s32* x, s32* y);
DECLARE_FUNC(void, recomp_get_camera_inputs, float* x, float* y);
DECLARE_FUNC(void, recomp_set_right_analog_suppressed, s32 suppressed);

#endif
