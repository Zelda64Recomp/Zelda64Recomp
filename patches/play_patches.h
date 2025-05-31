#ifndef __PLAY_PATCHES_H__
#define __PLAY_PATCHES_H__

#include "patches.h"
#include "patch_helpers.h"

#define MOUSE_SHIELD_CLAMP_X 7200.0f
#define MOUSE_SHIELD_CLAMP_Y 10800.0f
#define MOUSE_CAMERA_SCALE_X 0.04f
#define MOUSE_CAMERA_SCALE_Y 0.08f // For some reason, the vertical sensitivity seemed less than the horizontal. This compensates.

typedef struct {
    float delta_x;
    float delta_y;

    bool crouch_shielding;
    float shield_pos_x;
    float shield_pos_y;
} MouseInputHandler;

extern MouseInputHandler mouse_input_handler;

void debug_play_update(PlayState* play);
void camera_pre_play_update(PlayState* play);
void camera_post_play_update(PlayState* play);
void analog_cam_pre_play_update(PlayState* play);
void analog_cam_post_play_update(PlayState* play);
void matrix_play_update(PlayState* play);
void autosave_post_play_update(PlayState* play);

DECLARE_FUNC(unsigned int, recomp_get_mouse_buttons, );
DECLARE_FUNC(unsigned int, recomp_get_mouse_button_mask, );
DECLARE_FUNC(void, recomp_set_mouse_button_mask, unsigned int);
DECLARE_FUNC(unsigned int, recomp_get_mouse_wheel_pos, );
#endif
