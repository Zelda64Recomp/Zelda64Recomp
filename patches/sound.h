#ifndef __PATCH_AUDIO_H__
#define __PATCH_AUDIO_H__

#include "patch_helpers.h"

DECLARE_FUNC(float, recomp_get_bgm_volume);
DECLARE_FUNC(u32, recomp_get_low_health_beeps_enabled);

// Surround sound support
// Audio channel settings: 0 = Stereo, 1 = 5.1 Matrix, 2 = 5.1 Raw
DECLARE_FUNC(void, recomp_set_audio_channels, s32 channels);
DECLARE_FUNC(s32, recomp_get_audio_channels);
DECLARE_FUNC(s32, recomp_get_enhanced_surround_enabled);

#endif
