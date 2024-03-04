#ifndef __INPUT_H__
#define __INPUT_H__

#include "patch_helpers.h"

typedef enum {
    RECOMP_CAMERA_NORMAL,
    RECOMP_CAMERA_DUALANALOG,
} RecompCameraMode;

extern RecompCameraMode recomp_camera_mode;

DECLARE_FUNC(void, recomp_get_gyro_deltas, float* x, float* y);
DECLARE_FUNC(int, recomp_get_targeting_mode);

#endif
