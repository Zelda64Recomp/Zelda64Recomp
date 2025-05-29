#ifndef __PLAY_PATCHES_H__
#define __PLAY_PATCHES_H__

#include "patches.h"

#define MOUSE_SHIELD_CLAMP_X 7200.0f
#define MOUSE_SHIELD_CLAMP_Y 10800.0f
#define MOUSE_CAMERA_SCALE_X 0.04f
#define MOUSE_CAMERA_SCALE_Y 0.04f

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

#endif
