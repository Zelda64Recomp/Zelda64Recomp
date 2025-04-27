#include "patches.h"
#include "input.h"
#include "z64snap.h"
// Decomp rename, TODO update decomp and remove this
#define AudioVoice_GetWord func_801A5100
#include "z64voice.h"
#include "audiothread_cmd.h"

RECOMP_DECLARE_EVENT(recomp_before_first_person_aiming_update_event(PlayState* play, Player* this, bool in_free_look, RecompAimingOverideMode* recomp_aiming_override_mode));
RECOMP_DECLARE_EVENT(recomp_after_first_person_aiming_update_event(PlayState* play, Player* this, bool in_free_look));
RECOMP_DECLARE_EVENT(recomp_set_extra_item_slot_statuses(PlayState* play, s32 enabled));

s32 func_80847190(PlayState* play, Player* this, s32 arg2);
s16 func_80832754(Player* this, s32 arg1);
s32 func_8082EF20(Player* this);

// This flag is reset every frame by 'poll_inputs()'.
RecompAimingOverideMode recomp_aiming_override_mode = RECOMP_AIMING_OVERRIDE_OFF;

// @recomp Patched to add gyro and mouse aiming.
RECOMP_PATCH s32 func_80847190(PlayState* play, Player* this, s32 arg2) {
    s32 pad;
    s16 var_s0;

    // Checks if we're in free look (C-Up look around mode).
    bool in_free_look = (!func_800B7128(this) && !func_8082EF20(this) && !arg2);

    // Checking if any mods have disabled aiming with the left stick.
    recomp_before_first_person_aiming_update_event(play, this, in_free_look, &recomp_aiming_override_mode);

    // @recomp Get the aiming camera inversion state.
    s32 inverted_x, inverted_y;
    recomp_get_inverted_axes(&inverted_x, &inverted_y);

    // @recomp Get the analog camera input values if analog cam is enabled, or right-stick aiming is being forced.
    s32 analog_x = 0;
    s32 analog_y = 0;
    if (recomp_get_analog_cam_enabled() || recomp_aiming_override_mode == RECOMP_AIMING_OVERRIDE_FORCE_RIGHT_STICK) {
        float analog_x_float = 0.0f;
        float analog_y_float = 0.0f;
        recomp_get_camera_inputs(&analog_x_float, &analog_y_float);
        // Scale by 127 to match what ultramodern does, then clamp to 60 to match the game's handling.
        analog_x = (s32)(analog_x_float * 127.0f);
        analog_x = CLAMP(analog_x, -60, 60);
        analog_y = (s32)(analog_y_float * -127.0f);
        analog_y = CLAMP(analog_y, -60, 60);
    }

    // recomp_printf("stick_x: %d stick_y: %d analog_x: %d analog_y: %d\n",
    //     play->state.input[0].rel.stick_x, play->state.input[0].rel.stick_y,
    //     analog_x, analog_y);

    if (in_free_look) {
        // @recomp Add in the analog camera Y input. Clamp to prevent moving the camera twice as fast if both sticks are held.
        s32 cam_input_y = analog_y;
        if (recomp_aiming_override_mode == RECOMP_AIMING_OVERRIDE_OFF) {
            cam_input_y += play->state.input[0].rel.stick_y;
        }
        var_s0 = CLAMP(cam_input_y, -61, 61) * 0xF0;
        
        // @recomp Invert the Y axis accordingly (default is inverted, so negate if not inverted).
        if (!inverted_y) {
            var_s0 = -var_s0;
        }
        Math_SmoothStepToS(&this->actor.focus.rot.x, var_s0, 0xE, 0xFA0, 0x1E);

        // @recomp Add in the analog camera X input. Clamp to prevent moving the camera twice as fast if both sticks are held.
        s32 cam_input_x = analog_x;
        if (recomp_aiming_override_mode == RECOMP_AIMING_OVERRIDE_OFF) {
            cam_input_x += play->state.input[0].rel.stick_x;
        }
        var_s0 = CLAMP(cam_input_x, -61, 61) * -0x10;

        // @recomp Invert the X axis accordingly
        if (inverted_x) {
            var_s0 = -var_s0;
        }
        var_s0 = CLAMP(var_s0, -0xBB8, 0xBB8);
        this->actor.focus.rot.y += var_s0;
    }
    else {
        static float total_gyro_x, total_gyro_y;
        static float total_mouse_x, total_mouse_y;
        static float filtered_gyro_x, filtered_gyro_y;
        static int applied_aim_x, applied_aim_y;

        const float gyro_filter_factor = 0.00f;

        // // TODO remappable gyro reset button
        // if (play->state.input[0].press.button & BTN_L) {
        //     total_gyro_x = 0;
        //     total_gyro_y = 0;
        //     filtered_gyro_x = 0;
        //     filtered_gyro_y = 0;
        // }

        float delta_gyro_x, delta_gyro_y;
        recomp_get_gyro_deltas(&delta_gyro_x, &delta_gyro_y);

        total_gyro_x += delta_gyro_x;
        total_gyro_y += delta_gyro_y;

        filtered_gyro_x = filtered_gyro_x * gyro_filter_factor + total_gyro_x * (1.0f - gyro_filter_factor);
        filtered_gyro_y = filtered_gyro_y * gyro_filter_factor + total_gyro_y * (1.0f - gyro_filter_factor);

        float delta_mouse_x, delta_mouse_y;
        recomp_get_mouse_deltas(&delta_mouse_x, &delta_mouse_y);
        
        total_mouse_x += delta_mouse_x;
        total_mouse_y += delta_mouse_y;

        // The gyro X-axis (tilt) corresponds to the camera X-axis (tilt).
        // The gyro Y-axis (left/right rotation) corresponds to the camera Y-axis (left/right rotation).
        // The mouse Y-axis (up/down movement) corresponds to the camera X-axis (tilt).
        // The mouse X-axis (left/right movement) corresponds to the camera Y-axis (left/right rotation).
        int target_aim_x = (int)(filtered_gyro_x * -3.0f + total_mouse_y * 20.0f);
        int target_aim_y = (int)(filtered_gyro_y * 3.0f  + total_mouse_x * -20.0f);

        s16 temp3;

        // @recomp Invert the Y axis accordingly (default is inverted, so negate if not inverted).
        // Also add in the analog camera Y input. Clamp to prevent moving the camera twice as fast if both sticks are held.
        s32 cam_input_y = analog_y;
        if (recomp_aiming_override_mode == RECOMP_AIMING_OVERRIDE_OFF) {
            cam_input_y += play->state.input[0].rel.stick_y;
        }
        s32 stick_y;
        stick_y = CLAMP(cam_input_y, -61, 61);

        if (!inverted_y) {
            stick_y = -stick_y;
        }

        temp3 = ((stick_y >= 0) ? 1 : -1) *
            (s32)((1.0f - Math_CosS(stick_y * 0xC8)) * 1500.0f);
        this->actor.focus.rot.x += temp3 + (s32)(target_aim_x - applied_aim_x);
        applied_aim_x = target_aim_x;

        if (this->stateFlags1 & PLAYER_STATE1_800000) {
            this->actor.focus.rot.x = CLAMP(this->actor.focus.rot.x, -0x1F40, 0xFA0);
        }
        else {
            this->actor.focus.rot.x = CLAMP(this->actor.focus.rot.x, -0x36B0, 0x36B0);
        }

        var_s0 = this->actor.focus.rot.y - this->actor.shape.rot.y;

        // @recomp Invert the X axis accordingly. Also add in the analog camera Y input.
        // Clamp to prevent moving the camera twice as fast if both sticks are held.
        s32 cam_input_x = analog_x;
        if (recomp_aiming_override_mode == RECOMP_AIMING_OVERRIDE_OFF) {
            cam_input_x += play->state.input[0].rel.stick_x;
        }
        s32 stick_x;
        stick_x = CLAMP(cam_input_x, -61, 61);

        if (inverted_x) {
            stick_x = -stick_x;
        }
        temp3 = ((stick_x >= 0) ? 1 : -1) *
            (s32)((1.0f - Math_CosS(stick_x * 0xC8)) * -1500.0f);
        var_s0 += temp3 + (s32)(target_aim_y - applied_aim_y);
        applied_aim_y = target_aim_y;

        this->actor.focus.rot.y = CLAMP(var_s0, -0x4AAA, 0x4AAA) + this->actor.shape.rot.y;
    }

    recomp_after_first_person_aiming_update_event(play, this, in_free_look);

    this->unk_AA6 |= 2;

    return func_80832754(this, (play->unk_1887C != 0) || func_800B7128(this) || func_8082EF20(this));
}

extern Input* sPlayerControlInput;

/**
 * Update for using telescopes. SCENE_AYASHIISHOP acts quite differently: it has a different camera mode and cannot use
 * zooming.
 *
 * - Stick inputs move the view; shape.rot.y is used as a base position which cannot be looked too far away from. (This
 * is not necessarily the same as the original angle of the spawn.)
 * - A can be used to zoom (except in SCENE_AYASHIISHOP)
 * - B exits, using the RESPAWN_MODE_DOWN entrance
 */
// @recomp Patched for aiming inversion and supporting the right stick in dual analog.
RECOMP_PATCH void func_8083A98C(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    Player* this = (Player*)thisx;
    s32 camMode;

    if (play->csCtx.state != CS_STATE_IDLE) {
        return;
    }

    // @recomp Get the aiming camera inversion state.
    s32 inverted_x, inverted_y;
    recomp_get_inverted_axes(&inverted_x, &inverted_y);
    // @recomp Get the analog camera input values if analog cam is enabled.
    s32 analog_x = 0;
    s32 analog_y = 0;
    if (recomp_get_analog_cam_enabled()) {
        float analog_x_float = 0.0f;
        float analog_y_float = 0.0f;
        recomp_get_camera_inputs(&analog_x_float, &analog_y_float);
        // Scale by 127 to match what ultramodern does, then clamp to 60 to match the game's handling.
        analog_x = (s32)(analog_x_float * 127.0f);
        analog_x = CLAMP(analog_x, -60, 60);
        analog_y = (s32)(analog_y_float * -127.0f);
        analog_y = CLAMP(analog_y, -60, 60);
    }

    if (DECR(this->av2.actionVar2) != 0) {
        camMode = (play->sceneId != SCENE_AYASHIISHOP) ? CAM_MODE_FIRSTPERSON : CAM_MODE_DEKUHIDE;

        // Show controls overlay. SCENE_AYASHIISHOP does not have Zoom, so has a different one.
        if (this->av2.actionVar2 == 1) {
            Message_StartTextbox(play, (play->sceneId == SCENE_AYASHIISHOP) ? 0x2A00 : 0x5E6, NULL);
        }
    } else {
        sPlayerControlInput = play->state.input;
        if (play->view.fovy >= 25.0f) {
            s16 prevFocusX = thisx->focus.rot.x;
            s16 prevFocusY = thisx->focus.rot.y;
            s16 inputY;
            s16 inputX;
            s16 newYaw; // from base position shape.rot.y

            // @recomp Add in the analog camera Y input. Clamp to prevent moving the camera twice as fast if both sticks are held.
            // Pitch:
            inputY = CLAMP(sPlayerControlInput->rel.stick_y + analog_y, -60, 60) * 4;
            // @recomp Invert the Y axis accordingly (default is inverted, so negate if not inverted).
            if (!inverted_y) {
                inputY = -inputY;
            }
            // Add input, clamped to prevent turning too fast
            thisx->focus.rot.x += CLAMP(inputY, -0x12C, 0x12C);
            // Prevent looking too far up or down
            thisx->focus.rot.x = CLAMP(thisx->focus.rot.x, -0x2EE0, 0x2EE0);

            // @recomp Add in the analog camera X input. Clamp to prevent moving the camera twice as fast if both sticks are held.
            // Yaw: shape.rot.y is used as a fixed starting position
            inputX = CLAMP(sPlayerControlInput->rel.stick_x + analog_x, -60, 60) * -4;
            // @recomp Invert the X axis accordingly.
            if (inverted_x) {
                inputX = -inputX;
            }
            // Start from current position: no input -> no change
            newYaw = thisx->focus.rot.y - thisx->shape.rot.y;
            // Add input, clamped to prevent turning too fast
            newYaw += CLAMP(inputX, -0x12C, 0x12C);
            // Prevent looking too far left or right of base position
            newYaw = CLAMP(newYaw, -0x3E80, 0x3E80);
            thisx->focus.rot.y = thisx->shape.rot.y + newYaw;

            if (play->sceneId == SCENE_00KEIKOKU) {
                f32 focusDeltaX = (s16)(thisx->focus.rot.x - prevFocusX);
                f32 focusDeltaY = (s16)(thisx->focus.rot.y - prevFocusY);

                Audio_PlaySfx_AtPosWithFreq(&gSfxDefaultPos, NA_SE_PL_TELESCOPE_MOVEMENT - SFX_FLAG,
                                            sqrtf(SQ(focusDeltaX) + SQ(focusDeltaY)) / 300.0f);
            }
        }

        if (play->sceneId == SCENE_AYASHIISHOP) {
            camMode = CAM_MODE_DEKUHIDE;
        } else if (CHECK_BTN_ALL(sPlayerControlInput->cur.button, BTN_A)) { // Zoom
            camMode = CAM_MODE_TARGET;
        } else {
            camMode = CAM_MODE_NORMAL;
        }

        // Exit
        if (CHECK_BTN_ALL(sPlayerControlInput->press.button, BTN_B)) {
            Message_CloseTextbox(play);

            if (play->sceneId == SCENE_00KEIKOKU) {
                gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance = ENTRANCE(ASTRAL_OBSERVATORY, 2);
            } else {
                u16 entrance;

                if (play->sceneId == SCENE_AYASHIISHOP) {
                    entrance = ENTRANCE(CURIOSITY_SHOP, 3);
                } else {
                    entrance = ENTRANCE(PIRATES_FORTRESS_INTERIOR, 8);
                }
                gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance = entrance;
            }

            func_80169EFC(&play->state);
            gSaveContext.respawnFlag = -2;
            play->transitionType = TRANS_TYPE_CIRCLE;
        }
    }

    Camera_ChangeSetting(Play_GetCamera(play, CAM_ID_MAIN), CAM_SET_TELESCOPE);
    Camera_ChangeMode(Play_GetCamera(play, CAM_ID_MAIN), camMode);
}

bool no_bow_epona_fix = false;

// @recomp_export void recomp_set_no_bow_epona_fix(bool new_val): Set whether to enable the fix for getting on Epona without a bow.
RECOMP_EXPORT void recomp_set_no_bow_epona_fix(bool new_val) {
    no_bow_epona_fix = new_val;
}

bool h_and_d_no_sword_fix = false;

// @recomp_export void recomp_set_h_and_d_no_sword_fix(bool new_val): Set whether to enable the fix for playing Honey and Darling without a sword.
RECOMP_EXPORT void recomp_set_h_and_d_no_sword_fix(bool new_val) {
    h_and_d_no_sword_fix = new_val;
}

extern s16 sPictoState;
extern s16 sPictoPhotoBeingTaken;
extern void* gWorkBuffer;
u16 func_801A5100(void);

#define ON_EPONA (player->stateFlags1 & PLAYER_STATE1_800000)
#define EPONA_FIX_ACTIVE (no_bow_epona_fix && ON_EPONA)

#define AT_H_AND_D (play->sceneId == SCENE_BOWLING)
#define H_AND_D_FIX_ACTIVE (h_and_d_no_sword_fix && AT_H_AND_D)

// @recomp Patched to call event for extra item slot mods.
RECOMP_PATCH void Interface_UpdateButtonsPart1(PlayState* play) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    Player* player = GET_PLAYER(play);
    s32 pad;
    s32 restoreHudVisibility = false;

    if (gSaveContext.save.cutsceneIndex < 0xFFF0) {
        gSaveContext.hudVisibilityForceButtonAlphasByStatus = false;
        if (ON_EPONA || CHECK_WEEKEVENTREG(WEEKEVENTREG_08_01) ||
            (!CHECK_EVENTINF(EVENTINF_41) && (play->unk_1887C >= 2))) {
            // Riding Epona OR Honey & Darling minigame OR Horseback balloon minigame OR related to swamp boat
            // (non-minigame?)
            if (ON_EPONA && (player->currentMask == PLAYER_MASK_BLAST) &&
                (gSaveContext.bButtonStatus == BTN_DISABLED)) {
                // Riding Epona with blast mask?
                restoreHudVisibility = true;
                gSaveContext.bButtonStatus = BTN_ENABLED;
            }

            if (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) != ITEM_NONE) {
                if ((player->transformation == PLAYER_FORM_DEKU) && CHECK_WEEKEVENTREG(WEEKEVENTREG_08_01)) {
                    gSaveContext.hudVisibilityForceButtonAlphasByStatus = true;
                    if (play->sceneId == SCENE_BOWLING) {
                        if (gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_DISABLED) {
                            gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_ENABLED;
                            gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT] = BTN_DISABLED;
                            gSaveContext.buttonStatus[EQUIP_SLOT_C_DOWN] = BTN_DISABLED;
                            gSaveContext.buttonStatus[EQUIP_SLOT_C_RIGHT] = BTN_DISABLED;
                            recomp_set_extra_item_slot_statuses(play, BTN_DISABLED);
                        }
                    } else if (gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_DISABLED) {
                        gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_ENABLED;
                        gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT] = BTN_ENABLED;
                        gSaveContext.buttonStatus[EQUIP_SLOT_C_DOWN] = BTN_ENABLED;
                        gSaveContext.buttonStatus[EQUIP_SLOT_C_RIGHT] = BTN_ENABLED;
                        recomp_set_extra_item_slot_statuses(play, BTN_ENABLED);
                    }

                    Interface_SetHudVisibility(HUD_VISIBILITY_B_MAGIC);
                } else {
                    if ((BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) != ITEM_BOW) &&
                        (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) != ITEM_BOMB) &&
                        (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) != ITEM_BOMBCHU)) {
                        gSaveContext.hudVisibilityForceButtonAlphasByStatus = true;
                        gSaveContext.buttonStatus[EQUIP_SLOT_B] = BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B);
                        gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT] = BTN_ENABLED;
                        gSaveContext.buttonStatus[EQUIP_SLOT_C_DOWN] = BTN_ENABLED;
                        gSaveContext.buttonStatus[EQUIP_SLOT_C_RIGHT] = BTN_ENABLED;
                        recomp_set_extra_item_slot_statuses(play, BTN_ENABLED);
                        if (play->sceneId == SCENE_BOWLING) {
                            if (CURRENT_DAY == 1) {
                                BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_BOMBCHU;
                            } else if (CURRENT_DAY == 2) {
                                BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_BOMB;
                            } else {
                                BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_BOW;
                            }
                            Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
                            gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT] = BTN_DISABLED;
                            gSaveContext.buttonStatus[EQUIP_SLOT_C_DOWN] = BTN_DISABLED;
                            gSaveContext.buttonStatus[EQUIP_SLOT_C_RIGHT] = BTN_DISABLED;
                            recomp_set_extra_item_slot_statuses(play, BTN_DISABLED);
                        } else {
                            // @recomp_use_export_var no_bow_epona_fix: Part of the no bow Epona fix.
                            if (EPONA_FIX_ACTIVE) {
                                if (gSaveContext.save.saveInfo.inventory.items[SLOT_BOW] == ITEM_BOW) {
                                    BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_BOW;
                                    gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_ENABLED;
                                }
                            } else {
                                BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_BOW;
                            }

                            if (play->unk_1887C >= 2) {
                                Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
                            } else if (gSaveContext.save.saveInfo.inventory.items[SLOT_BOW] == ITEM_NONE) {
                                // @recomp_use_export_var no_bow_epona_fix: Part of the no bow Epona fix.
                                if (EPONA_FIX_ACTIVE) {
                                    gSaveContext.buttonStatus[EQUIP_SLOT_B] = BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B);
                                    gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_DISABLED;
                                } else {
                                    BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_NONE;
                                }
                            } else {
                                Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
                            }

                            // @recomp_use_export_var no_bow_epona_fix: If the B button does not contain a sword, don't disable the UI.
                            if (!EPONA_FIX_ACTIVE || BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) < ITEM_SWORD_KOKIRI ||
                                BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) > ITEM_SWORD_GILDED) {
                                gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT] = BTN_DISABLED;
                                gSaveContext.buttonStatus[EQUIP_SLOT_C_DOWN] = BTN_DISABLED;
                                gSaveContext.buttonStatus[EQUIP_SLOT_C_RIGHT] = BTN_DISABLED;
                                recomp_set_extra_item_slot_statuses(play, BTN_DISABLED);
                                Interface_SetHudVisibility(HUD_VISIBILITY_A_HEARTS_MAGIC_MINIMAP_WITH_OVERWRITE);
                            }
                        }
                    }

                    if (gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_DISABLED && BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_BOW) {
                        gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_ENABLED;
                    }

                    if (play->transitionMode != TRANS_MODE_OFF) {
                        Interface_SetHudVisibility(HUD_VISIBILITY_NONE);
                    } else if ((gSaveContext.minigameStatus == MINIGAME_STATUS_ACTIVE) &&
                               (gSaveContext.save.entrance == ENTRANCE(ROMANI_RANCH, 0)) &&
                               (Cutscene_GetSceneLayer(play) != 0) && (play->transitionTrigger == TRANS_TRIGGER_OFF)) {
                        Interface_SetHudVisibility(HUD_VISIBILITY_A_B_MINIMAP);
                    } else if ((gSaveContext.minigameStatus == MINIGAME_STATUS_ACTIVE) && CHECK_EVENTINF(EVENTINF_35)) {
                        Interface_SetHudVisibility(HUD_VISIBILITY_B_MINIMAP);
                    } else if (!CHECK_WEEKEVENTREG(WEEKEVENTREG_82_08) &&
                               (gSaveContext.minigameStatus == MINIGAME_STATUS_ACTIVE)) {
                        Interface_SetHudVisibility(HUD_VISIBILITY_B);
                    } else if (play->unk_1887C >= 2) {
                        Interface_SetHudVisibility(HUD_VISIBILITY_B);
                    } else if (CHECK_WEEKEVENTREG(WEEKEVENTREG_08_01)) {
                        gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT] = BTN_DISABLED;
                        gSaveContext.buttonStatus[EQUIP_SLOT_C_DOWN] = BTN_DISABLED;
                        gSaveContext.buttonStatus[EQUIP_SLOT_C_RIGHT] = BTN_DISABLED;
                        recomp_set_extra_item_slot_statuses(play, BTN_DISABLED);
                        Interface_SetHudVisibility(HUD_VISIBILITY_A_B_MINIMAP);
                    } else if (ON_EPONA) {
                        Interface_SetHudVisibility(HUD_VISIBILITY_A_B_MINIMAP);
                    }
                }
            } else {
                if (ON_EPONA) {
                    Interface_SetHudVisibility(HUD_VISIBILITY_A_B_MINIMAP);
                }

                if (play->sceneId == SCENE_BOWLING) {
                    if (CURRENT_DAY == 1) {
                        BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_BOMBCHU;
                    } else if (CURRENT_DAY == 2) {
                        BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_BOMB;
                    } else {
                        BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_BOW;
                    }
                    if (h_and_d_no_sword_fix) {
                        gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_ENABLED;
                    }
                    gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT] = BTN_DISABLED;
                    gSaveContext.buttonStatus[EQUIP_SLOT_C_DOWN] = BTN_DISABLED;
                    gSaveContext.buttonStatus[EQUIP_SLOT_C_RIGHT] = BTN_DISABLED;
                    recomp_set_extra_item_slot_statuses(play, BTN_DISABLED);
                } else {
                    // @recomp_use_export_var no_bow_epona_fix: Part of the no bow Epona fix.
                    if (EPONA_FIX_ACTIVE) {
                        if (gSaveContext.save.saveInfo.inventory.items[SLOT_BOW] == ITEM_BOW) {
                            BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_BOW;
                            gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_ENABLED;
                        }
                    } else {
                        BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_BOW;
                    }
                }

                if (play->unk_1887C >= 2) {
                    Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
                } else if (gSaveContext.save.saveInfo.inventory.items[SLOT_BOW] == ITEM_NONE && !H_AND_D_FIX_ACTIVE) {
                    // @recomp_use_export_var no_bow_epona_fix: Part of the no bow Epona fix.
                    if (EPONA_FIX_ACTIVE) {
                        gSaveContext.buttonStatus[EQUIP_SLOT_B] = BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B);
                        gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_DISABLED;
                    } else {
                        BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_NONE;
                    }
                } else {
                    Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
                }

                if (gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_DISABLED) {
                    // @recomp_use_export_var no_bow_epona_fix: Don't enable the B button unless it is being used for the bow.
                    if (!no_bow_epona_fix || BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_BOW) {
                        gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_ENABLED;
                    }

                    // @recomp_use_export_var no_bow_epona_fix: Don't restore hud visibility from Epona without a sword.
                    if (!no_bow_epona_fix || (player->stateFlags1 & PLAYER_STATE1_800000) == 0) {
                        restoreHudVisibility = true;
                    }
                }

                // @recomp_use_export_var no_bow_epona_fix: If the B button does not contain the bow, don't disable the UI.
                if ((!no_bow_epona_fix || BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_BOW) && !H_AND_D_FIX_ACTIVE) {
                    gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT] = BTN_DISABLED;
                    gSaveContext.buttonStatus[EQUIP_SLOT_C_DOWN] = BTN_DISABLED;
                    gSaveContext.buttonStatus[EQUIP_SLOT_C_RIGHT] = BTN_DISABLED;
                    recomp_set_extra_item_slot_statuses(play, BTN_DISABLED);
                    Interface_SetHudVisibility(HUD_VISIBILITY_A_HEARTS_MAGIC_MINIMAP_WITH_OVERWRITE);
                }

                if (play->transitionMode != TRANS_MODE_OFF) {
                    Interface_SetHudVisibility(HUD_VISIBILITY_NONE);
                } else if ((gSaveContext.minigameStatus == MINIGAME_STATUS_ACTIVE) &&
                           (gSaveContext.save.entrance == ENTRANCE(ROMANI_RANCH, 0)) &&
                           (Cutscene_GetSceneLayer(play) != 0) && (play->transitionTrigger == TRANS_TRIGGER_OFF)) {
                    Interface_SetHudVisibility(HUD_VISIBILITY_A_B_MINIMAP);
                } else if (gSaveContext.minigameStatus == MINIGAME_STATUS_ACTIVE) {
                    Interface_SetHudVisibility(HUD_VISIBILITY_B);
                } else if (play->unk_1887C >= 2) {
                    Interface_SetHudVisibility(HUD_VISIBILITY_B);
                } else if (CHECK_WEEKEVENTREG(WEEKEVENTREG_08_01)) {
                    gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT] = BTN_DISABLED;
                    gSaveContext.buttonStatus[EQUIP_SLOT_C_DOWN] = BTN_DISABLED;
                    gSaveContext.buttonStatus[EQUIP_SLOT_C_RIGHT] = BTN_DISABLED;
                    recomp_set_extra_item_slot_statuses(play, BTN_DISABLED);
                    Interface_SetHudVisibility(HUD_VISIBILITY_A_B_MINIMAP);
                } else if (ON_EPONA) {
                    Interface_SetHudVisibility(HUD_VISIBILITY_A_B_MINIMAP);
                } else if (H_AND_D_FIX_ACTIVE) {
                    Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
                    Interface_SetHudVisibility(HUD_VISIBILITY_B);
                }
            }
        } else if (sPictoState != PICTO_BOX_STATE_OFF) {
            // Related to pictograph
            if (sPictoState == PICTO_BOX_STATE_LENS) {
                if (!(play->actorCtx.flags & ACTORCTX_FLAG_PICTO_BOX_ON)) {
                    Play_CompressI8ToI5((play->pictoPhotoI8 != NULL) ? play->pictoPhotoI8 : gWorkBuffer,
                                        (u8*)((void)0, gSaveContext.pictoPhotoI5),
                                        PICTO_PHOTO_WIDTH * PICTO_PHOTO_HEIGHT);
                    interfaceCtx->unk_222 = interfaceCtx->unk_224 = 0;
                    restoreHudVisibility = true;
                    sPictoState = PICTO_BOX_STATE_OFF;
                } else if (CHECK_BTN_ALL(CONTROLLER1(&play->state)->press.button, BTN_B)) {
                    play->actorCtx.flags &= ~ACTORCTX_FLAG_PICTO_BOX_ON;
                    interfaceCtx->unk_222 = interfaceCtx->unk_224 = 0;
                    restoreHudVisibility = true;
                    sPictoState = PICTO_BOX_STATE_OFF;
                } else if (CHECK_BTN_ALL(CONTROLLER1(&play->state)->press.button, BTN_A) ||
                            (AudioVoice_GetWord() == VOICE_WORD_ID_CHEESE)) {
                    if (!CHECK_EVENTINF(EVENTINF_41) ||
                        (CHECK_EVENTINF(EVENTINF_41) && (CutsceneManager_GetCurrentCsId() == CS_ID_NONE))) {
                        Audio_PlaySfx(NA_SE_SY_CAMERA_SHUTTER);
                        SREG(89) = 1;
                        play->haltAllActors = true;
                        sPictoState = PICTO_BOX_STATE_SETUP_PHOTO;
                        sPictoPhotoBeingTaken = true;
                    }
                }
            } else if ((sPictoState >= PICTO_BOX_STATE_SETUP_PHOTO) && (Message_GetState(&play->msgCtx) == 4) &&
                       Message_ShouldAdvance(play)) {
                play->haltAllActors = false;
                player->stateFlags1 &= ~PLAYER_STATE1_200;
                Message_CloseTextbox(play);
                if (play->msgCtx.choiceIndex != 0) {
                    Audio_PlaySfx_MessageCancel();
                    func_80115844(play, DO_ACTION_STOP);
                    Interface_SetHudVisibility(HUD_VISIBILITY_A_B);
                    sPictoState = PICTO_BOX_STATE_LENS;
                    REMOVE_QUEST_ITEM(QUEST_PICTOGRAPH);
                } else {
                    Audio_PlaySfx_MessageDecide();
                    interfaceCtx->unk_222 = interfaceCtx->unk_224 = 0;
                    restoreHudVisibility = true;
                    Interface_SetHudVisibility(HUD_VISIBILITY_ALL);
                    sPictoState = PICTO_BOX_STATE_OFF;
                    if (sPictoPhotoBeingTaken) {
                        Play_CompressI8ToI5((play->pictoPhotoI8 != NULL) ? play->pictoPhotoI8 : gWorkBuffer,
                                            (u8*)((void)0, gSaveContext.pictoPhotoI5),
                                            PICTO_PHOTO_WIDTH * PICTO_PHOTO_HEIGHT);
                        Snap_RecordPictographedActors(play);
                    }
                    play->actorCtx.flags &= ~ACTORCTX_FLAG_PICTO_BOX_ON;
                    SET_QUEST_ITEM(QUEST_PICTOGRAPH);
                    sPictoPhotoBeingTaken = false;
                }
            }
        } else if ((gSaveContext.minigameStatus == MINIGAME_STATUS_ACTIVE) &&
                   (gSaveContext.save.entrance == ENTRANCE(WATERFALL_RAPIDS, 1)) &&
                   (play->transitionTrigger == TRANS_TRIGGER_OFF) && (play->transitionMode == TRANS_MODE_OFF)) {
            // Beaver race minigame
            gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT] = BTN_DISABLED;
            gSaveContext.buttonStatus[EQUIP_SLOT_C_DOWN] = BTN_DISABLED;
            gSaveContext.buttonStatus[EQUIP_SLOT_C_RIGHT] = BTN_DISABLED;
            recomp_set_extra_item_slot_statuses(play, BTN_DISABLED);
            Interface_SetHudVisibility(HUD_VISIBILITY_A_B_MINIMAP);
        } else if ((gSaveContext.save.entrance == ENTRANCE(GORON_RACETRACK, 1)) &&
                   (play->transitionTrigger == TRANS_TRIGGER_OFF) && (play->transitionMode == TRANS_MODE_OFF)) {
            // Goron race minigame
            gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT] = BTN_DISABLED;
            gSaveContext.buttonStatus[EQUIP_SLOT_C_DOWN] = BTN_DISABLED;
            gSaveContext.buttonStatus[EQUIP_SLOT_C_RIGHT] = BTN_DISABLED;
            recomp_set_extra_item_slot_statuses(play, BTN_DISABLED);
            Interface_SetHudVisibility(HUD_VISIBILITY_A_B_HEARTS_MAGIC_MINIMAP);
        } else if (play->actorCtx.flags & ACTORCTX_FLAG_PICTO_BOX_ON) {
            // Related to pictograph
            if (!CHECK_QUEST_ITEM(QUEST_PICTOGRAPH)) {
                func_80115844(play, DO_ACTION_STOP);
                Interface_SetHudVisibility(HUD_VISIBILITY_A_B);
                sPictoState = PICTO_BOX_STATE_LENS;
            } else {
                Play_DecompressI5ToI8((u8*)((void)0, gSaveContext.pictoPhotoI5),
                                      (play->pictoPhotoI8 != NULL) ? play->pictoPhotoI8 : gWorkBuffer,
                                      PICTO_PHOTO_WIDTH * PICTO_PHOTO_HEIGHT);
                play->haltAllActors = true;
                sPictoState = PICTO_BOX_STATE_SETUP_PHOTO;
            }
        } else {
            // Continue processing the remaining cases
            Interface_UpdateButtonsPart2(play);
        }
    }

    if (restoreHudVisibility) {
        gSaveContext.hudVisibility = HUD_VISIBILITY_IDLE;
        if ((play->transitionTrigger == TRANS_TRIGGER_OFF) && (play->transitionMode == TRANS_MODE_OFF)) {
            Interface_SetHudVisibility(HUD_VISIBILITY_ALL);
        }
    }
}

bool fd_anywhere = false;

// @recomp_export void recomp_set_fd_anywhere(bool new_val): Set whether the Fierce Deity's Mask has scene restrictions.
RECOMP_EXPORT void recomp_set_fd_anywhere(bool new_val) {
    fd_anywhere = new_val;
}

/**
 * A continuation of the if-else chain from Interface_UpdateButtonsPart1
 * Also used directly when opening the pause menu i.e. skips part 1
 */
// @recomp Patched in the same way as Interface_UpdateButtonsPart1
RECOMP_PATCH void Interface_UpdateButtonsPart2(PlayState* play) {
    MessageContext* msgCtx = &play->msgCtx;
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    Player* player = GET_PLAYER(play);
    s16 i;
    s16 restoreHudVisibility = false;

    if (CHECK_EVENTINF(EVENTINF_41)) {
        // Related to swamp boat (non-minigame)?
        for (i = EQUIP_SLOT_C_LEFT; i <= EQUIP_SLOT_C_RIGHT; i++) {
            if ((GET_CUR_FORM_BTN_ITEM(i) != ITEM_PICTOGRAPH_BOX) || (msgCtx->msgMode != MSGMODE_NONE)) {
                if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                    restoreHudVisibility = true;
                }
                gSaveContext.buttonStatus[i] = BTN_DISABLED;
            } else {
                if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                    restoreHudVisibility = true;
                }
                gSaveContext.buttonStatus[i] = BTN_ENABLED;
            }
        }

        if (sPictoState == PICTO_BOX_STATE_OFF) {
            if (gSaveContext.buttonStatus[EQUIP_SLOT_B] != BTN_DISABLED) {
                restoreHudVisibility = true;
            }
            gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_DISABLED;
        } else {
            if (gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_DISABLED) {
                restoreHudVisibility = true;
            }
            gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_ENABLED;
        }
    } else if (CHECK_WEEKEVENTREG(WEEKEVENTREG_90_20)) {
        // Fishermans's jumping minigame
        for (i = EQUIP_SLOT_C_LEFT; i <= EQUIP_SLOT_C_RIGHT; i++) {
            if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                gSaveContext.buttonStatus[i] = BTN_DISABLED;
            }
        }

        Interface_SetHudVisibility(HUD_VISIBILITY_B);
    } else if (CHECK_WEEKEVENTREG(WEEKEVENTREG_82_08)) {
        // Swordsman's log minigame
        for (i = EQUIP_SLOT_C_LEFT; i <= EQUIP_SLOT_C_RIGHT; i++) {
            if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                gSaveContext.buttonStatus[i] = BTN_DISABLED;
            }
        }

        Interface_SetHudVisibility(HUD_VISIBILITY_A_B_HEARTS_MAGIC_MINIMAP);
    } else if (CHECK_WEEKEVENTREG(WEEKEVENTREG_84_20)) {
        // Related to moon child
        if (player->currentMask == PLAYER_MASK_FIERCE_DEITY) {
            for (i = EQUIP_SLOT_C_LEFT; i <= EQUIP_SLOT_C_RIGHT; i++) {
                if ((GET_CUR_FORM_BTN_ITEM(i) == ITEM_MASK_FIERCE_DEITY) ||
                    ((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_BOTTLE) && (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_OBABA_DRINK))) {
                    if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                        restoreHudVisibility = true;
                        gSaveContext.buttonStatus[i] = BTN_ENABLED;
                    }
                } else {
                    if (gSaveContext.buttonStatus[i] != BTN_DISABLED) {
                        gSaveContext.buttonStatus[i] = BTN_DISABLED;
                        restoreHudVisibility = true;
                    }
                }
            }
        } else {
            for (i = EQUIP_SLOT_C_LEFT; i <= EQUIP_SLOT_C_RIGHT; i++) {
                if ((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_MASK_DEKU) && (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_MASK_ZORA)) {
                    if (gSaveContext.buttonStatus[i] != BTN_DISABLED) {
                        restoreHudVisibility = true;
                    }
                    gSaveContext.buttonStatus[i] = BTN_DISABLED;
                } else {
                    if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                        restoreHudVisibility = true;
                    }
                    gSaveContext.buttonStatus[i] = BTN_ENABLED;
                }
            }
        }
    } else if ((play->sceneId == SCENE_SPOT00) && (gSaveContext.sceneLayer == 6)) {
        // Unknown cutscene
        for (i = EQUIP_SLOT_C_LEFT; i <= EQUIP_SLOT_C_RIGHT; i++) {
            if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                restoreHudVisibility = true;
            }
            gSaveContext.buttonStatus[i] = BTN_DISABLED;
        }
    } else if (CHECK_EVENTINF(EVENTINF_34)) {
        // Deku playground minigame
        if (player->stateFlags3 & PLAYER_STATE3_1000000) {
            if (gSaveContext.save.saveInfo.inventory.items[SLOT_DEKU_NUT] == ITEM_DEKU_NUT) {
                BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_DEKU_NUT;
                Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
            } else {
                gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_DISABLED;
                restoreHudVisibility = true;
            }
        } else {
            if (gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_DISABLED) {
                gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_ENABLED;
                restoreHudVisibility = true;
            }

            for (i = EQUIP_SLOT_C_LEFT; i <= EQUIP_SLOT_C_RIGHT; i++) {
                if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                    restoreHudVisibility = true;
                }
                gSaveContext.buttonStatus[i] = BTN_DISABLED;
            }
        }

        if (restoreHudVisibility || (gSaveContext.hudVisibility != HUD_VISIBILITY_A_B_MINIMAP)) {
            gSaveContext.hudVisibility = HUD_VISIBILITY_IDLE;
            Interface_SetHudVisibility(HUD_VISIBILITY_A_B_MINIMAP);
            restoreHudVisibility = false;
        }
    } else if (player->stateFlags3 & PLAYER_STATE3_1000000) {
        // Nuts on B (from flying as Deku Link)
        if (gSaveContext.save.saveInfo.inventory.items[SLOT_DEKU_NUT] == ITEM_DEKU_NUT) {
            if (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) != ITEM_DEKU_NUT) {
                BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_DEKU_NUT;
                Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
                gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_ENABLED;
                restoreHudVisibility = true;
            }
        } else if (gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_ENABLED) {
            gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_DISABLED;
            restoreHudVisibility = true;
        }

        if (restoreHudVisibility) {
            gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT] = BTN_DISABLED;
            gSaveContext.buttonStatus[EQUIP_SLOT_C_DOWN] = BTN_DISABLED;
            gSaveContext.buttonStatus[EQUIP_SLOT_C_RIGHT] = BTN_DISABLED;
            recomp_set_extra_item_slot_statuses(play, BTN_DISABLED);
        }
    } else if (!gSaveContext.save.saveInfo.playerData.isMagicAcquired && (CUR_FORM == PLAYER_FORM_DEKU) &&
               (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_DEKU_NUT)) {
        // Nuts on B (as Deku Link)
        BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_FD;
        gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_DISABLED;
    } else if ((Player_GetEnvironmentalHazard(play) >= PLAYER_ENV_HAZARD_UNDERWATER_FLOOR) &&
               (Player_GetEnvironmentalHazard(play) <= PLAYER_ENV_HAZARD_UNDERWATER_FREE)) {
        // Swimming underwater
        if (CUR_FORM != PLAYER_FORM_ZORA) {
            if ((player->currentMask == PLAYER_MASK_BLAST) && (player->blastMaskTimer == 0)) {
                if (gSaveContext.bButtonStatus == BTN_DISABLED) {
                    restoreHudVisibility = true;
                }
                gSaveContext.bButtonStatus = BTN_ENABLED;
            } else if ((interfaceCtx->bButtonDoAction == DO_ACTION_EXPLODE) &&
                       (player->currentMask == PLAYER_MASK_BLAST)) {
                if (gSaveContext.bButtonStatus != BTN_DISABLED) {
                    gSaveContext.bButtonStatus = BTN_DISABLED;
                    restoreHudVisibility = true;
                }
            } else {
                if (gSaveContext.buttonStatus[EQUIP_SLOT_B] != BTN_DISABLED) {
                    restoreHudVisibility = true;
                }
                gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_DISABLED;
            }
        } else {
            if (gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_DISABLED) {
                restoreHudVisibility = true;
            }
            gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_ENABLED;
        }

        for (i = EQUIP_SLOT_C_LEFT; i <= EQUIP_SLOT_C_RIGHT; i++) {
            if (GET_CUR_FORM_BTN_ITEM(i) != ITEM_MASK_ZORA) {
                if (Player_GetEnvironmentalHazard(play) == PLAYER_ENV_HAZARD_UNDERWATER_FLOOR) {
                    if (!((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_BOTTLE) &&
                          (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_OBABA_DRINK))) {
                        if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                            restoreHudVisibility = true;
                        }
                        gSaveContext.buttonStatus[i] = BTN_DISABLED;
                    } else {
                        if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                            restoreHudVisibility = true;
                        }
                        gSaveContext.buttonStatus[i] = BTN_ENABLED;
                    }
                } else {
                    if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                        restoreHudVisibility = true;
                    }
                    gSaveContext.buttonStatus[i] = BTN_DISABLED;
                }
            } else if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                gSaveContext.buttonStatus[i] = BTN_ENABLED;
                restoreHudVisibility = true;
            }
        }

        if (restoreHudVisibility) {
            gSaveContext.hudVisibility = HUD_VISIBILITY_IDLE;
        }

        if ((play->transitionTrigger == TRANS_TRIGGER_OFF) && (play->transitionMode == TRANS_MODE_OFF)) {
            if (CutsceneManager_GetCurrentCsId() == CS_ID_NONE) {
                Interface_SetHudVisibility(HUD_VISIBILITY_ALL);
            }
        }
    } else if (player->stateFlags1 & PLAYER_STATE1_200000) {
        // First person view
        for (i = EQUIP_SLOT_C_LEFT; i <= EQUIP_SLOT_C_RIGHT; i++) {
            if (GET_CUR_FORM_BTN_ITEM(i) != ITEM_LENS_OF_TRUTH) {
                if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                    restoreHudVisibility = true;
                }
                gSaveContext.buttonStatus[i] = BTN_DISABLED;
            } else {
                if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                    restoreHudVisibility = true;
                }
                gSaveContext.buttonStatus[i] = BTN_ENABLED;
            }
        }

        if (gSaveContext.buttonStatus[EQUIP_SLOT_B] != BTN_DISABLED) {
            gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_DISABLED;
            restoreHudVisibility = true;
        }
    } else if (player->stateFlags1 & PLAYER_STATE1_2000) {
        // Hanging from a ledge
        if (gSaveContext.buttonStatus[EQUIP_SLOT_B] != BTN_DISABLED) {
            gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_DISABLED;
            gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT] = BTN_DISABLED;
            gSaveContext.buttonStatus[EQUIP_SLOT_C_DOWN] = BTN_DISABLED;
            gSaveContext.buttonStatus[EQUIP_SLOT_C_RIGHT] = BTN_DISABLED;
            recomp_set_extra_item_slot_statuses(play, BTN_DISABLED);
            restoreHudVisibility = true;
            Interface_SetHudVisibility(HUD_VISIBILITY_ALL);
        }
    } else {
        // End of special event cases

        // B button
        if ((interfaceCtx->bButtonDoAction == DO_ACTION_EXPLODE) && (player->currentMask == PLAYER_MASK_BLAST) &&
            (player->blastMaskTimer != 0)) {
            // Cooldown period for blast mask
            if (gSaveContext.bButtonStatus != BTN_DISABLED) {
                gSaveContext.bButtonStatus = BTN_DISABLED;
                restoreHudVisibility = true;
            }
        } else {
            // default to enabled
            if (gSaveContext.bButtonStatus == BTN_DISABLED) {
                gSaveContext.bButtonStatus = BTN_ENABLED;
                restoreHudVisibility = true;
            }

            // Apply B button restriction
            if (interfaceCtx->restrictions.bButton == 0) {
                if ((BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_BOW) ||
                    (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_BOMB) ||
                    (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_BOMBCHU)) {
                    if (GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) == EQUIP_VALUE_SWORD_NONE) {
                        gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_DISABLED;
                    }

                    if (gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_ENABLED) {
                        gSaveContext.buttonStatus[EQUIP_SLOT_B] =
                            ITEM_SWORD_KOKIRI + GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) - EQUIP_VALUE_SWORD_KOKIRI;
                    }

                    BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = gSaveContext.buttonStatus[EQUIP_SLOT_B];

                    if (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) != ITEM_NONE) {
                        Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
                    }
                    restoreHudVisibility = true;
                } else if (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_NONE) {
                    if (interfaceCtx->bButtonDoAction != 0) {
                        if (gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_DISABLED) {
                            restoreHudVisibility = true;
                            gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_ENABLED;
                        }
                    } else {
                        if (gSaveContext.buttonStatus[EQUIP_SLOT_B] != BTN_DISABLED) {
                            restoreHudVisibility = true;
                            gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_DISABLED;
                        }
                    }
                } else if (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_NONE) {
                    if (gSaveContext.buttonStatus[EQUIP_SLOT_B] != BTN_DISABLED) {
                        restoreHudVisibility = true;
                        gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_DISABLED;
                    }
                } else {
                    if (gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_DISABLED) {
                        restoreHudVisibility = true;
                        gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_ENABLED;
                    }
                }
            } else if (interfaceCtx->restrictions.bButton != 0) {
                if ((BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_BOW) ||
                    (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_BOMB) ||
                    (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_BOMBCHU)) {
                    if (GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) == EQUIP_VALUE_SWORD_NONE) {
                        gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_DISABLED;
                    }

                    BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = gSaveContext.buttonStatus[EQUIP_SLOT_B];

                    if (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) != ITEM_NONE) {
                        Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
                    }
                    restoreHudVisibility = true;
                }
                if (gSaveContext.buttonStatus[EQUIP_SLOT_B] != BTN_DISABLED) {
                    gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_DISABLED;
                    restoreHudVisibility = true;
                }
            }
        }

        // C buttons
        if (GET_PLAYER_FORM == player->transformation) {
            for (i = EQUIP_SLOT_C_LEFT; i <= EQUIP_SLOT_C_RIGHT; i++) {
                // Individual C button
                if (!gPlayerFormItemRestrictions[GET_PLAYER_FORM][GET_CUR_FORM_BTN_ITEM(i)]) {
                    // Item not usable in current playerForm
                    if (gSaveContext.buttonStatus[i] != BTN_DISABLED) {
                        gSaveContext.buttonStatus[i] = BTN_DISABLED;
                        restoreHudVisibility = true;
                    }
                } else if (player->actor.id != ACTOR_PLAYER) {
                    // Currently not playing as the main player
                    if (gSaveContext.buttonStatus[i] != BTN_DISABLED) {
                        gSaveContext.buttonStatus[i] = BTN_DISABLED;
                        restoreHudVisibility = true;
                    }
                } else if (player->currentMask == PLAYER_MASK_GIANT) {
                    // Currently wearing Giant's Mask
                    if (GET_CUR_FORM_BTN_ITEM(i) != ITEM_MASK_GIANT) {
                        if (gSaveContext.buttonStatus[i] != BTN_DISABLED) {
                            gSaveContext.buttonStatus[i] = BTN_DISABLED;
                            restoreHudVisibility = true;
                        }
                    } else if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                        restoreHudVisibility = true;
                        gSaveContext.buttonStatus[i] = BTN_ENABLED;
                    }
                } else if (GET_CUR_FORM_BTN_ITEM(i) == ITEM_MASK_GIANT) {
                    // Giant's Mask is equipped
                    if (play->sceneId != SCENE_INISIE_BS) {
                        if (gSaveContext.buttonStatus[i] != BTN_DISABLED) {
                            gSaveContext.buttonStatus[i] = BTN_DISABLED;
                            restoreHudVisibility = true;
                        }
                    } else if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                        restoreHudVisibility = true;
                        gSaveContext.buttonStatus[i] = BTN_ENABLED;
                    }
                } else if (GET_CUR_FORM_BTN_ITEM(i) == ITEM_MASK_FIERCE_DEITY) {
                    // Fierce Deity's Mask is equipped
                    // @recomp_use_export_var fd_anywhere: Allow the player to use the Fierce Deity's Mask anywhere if mods enable it.
                    if (!fd_anywhere && (play->sceneId != SCENE_MITURIN_BS) && (play->sceneId != SCENE_HAKUGIN_BS) &&
                        (play->sceneId != SCENE_SEA_BS) && (play->sceneId != SCENE_INISIE_BS) &&
                        (play->sceneId != SCENE_LAST_BS)) {
                        if (gSaveContext.buttonStatus[i] != BTN_DISABLED) {
                            gSaveContext.buttonStatus[i] = BTN_DISABLED;
                            restoreHudVisibility = true;
                        }
                    } else if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                        restoreHudVisibility = true;
                        gSaveContext.buttonStatus[i] = BTN_ENABLED;
                    }
                } else {
                    // End of special item cases. Apply restrictions to buttons
                    if (interfaceCtx->restrictions.tradeItems != 0) {
                        if (((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_MOONS_TEAR) &&
                             (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_PENDANT_OF_MEMORIES)) ||
                            ((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_BOTTLE) &&
                             (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_OBABA_DRINK)) ||
                            (GET_CUR_FORM_BTN_ITEM(i) == ITEM_OCARINA_OF_TIME)) {
                            if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                                restoreHudVisibility = true;
                            }
                            gSaveContext.buttonStatus[i] = BTN_DISABLED;
                        }
                    } else if (interfaceCtx->restrictions.tradeItems == 0) {
                        if (((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_MOONS_TEAR) &&
                             (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_PENDANT_OF_MEMORIES)) ||
                            ((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_BOTTLE) &&
                             (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_OBABA_DRINK)) ||
                            (GET_CUR_FORM_BTN_ITEM(i) == ITEM_OCARINA_OF_TIME)) {
                            if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                                restoreHudVisibility = true;
                            }
                            gSaveContext.buttonStatus[i] = BTN_ENABLED;
                        }
                    }

                    if (interfaceCtx->restrictions.masks != 0) {
                        if ((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_MASK_DEKU) &&
                            (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_MASK_GIANT)) {
                            if (!gSaveContext.buttonStatus[i]) { // == BTN_ENABLED
                                restoreHudVisibility = true;
                            }
                            gSaveContext.buttonStatus[i] = BTN_DISABLED;
                        }
                    } else if (interfaceCtx->restrictions.masks == 0) {
                        if ((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_MASK_DEKU) &&
                            (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_MASK_GIANT)) {
                            if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                                restoreHudVisibility = true;
                            }
                            gSaveContext.buttonStatus[i] = BTN_ENABLED;
                        }
                    }

                    if (interfaceCtx->restrictions.pictoBox != 0) {
                        if (GET_CUR_FORM_BTN_ITEM(i) == ITEM_PICTOGRAPH_BOX) {
                            if (!gSaveContext.buttonStatus[i]) { // == BTN_ENABLED
                                restoreHudVisibility = true;
                            }
                            gSaveContext.buttonStatus[i] = BTN_DISABLED;
                        }
                    } else if (interfaceCtx->restrictions.pictoBox == 0) {
                        if (GET_CUR_FORM_BTN_ITEM(i) == ITEM_PICTOGRAPH_BOX) {
                            if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                                restoreHudVisibility = true;
                            }
                            gSaveContext.buttonStatus[i] = BTN_ENABLED;
                        }
                    }

                    if (interfaceCtx->restrictions.all != 0) {
                        if (!((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_MOONS_TEAR) &&
                              (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_PENDANT_OF_MEMORIES)) &&
                            !((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_BOTTLE) &&
                              (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_OBABA_DRINK)) &&
                            (GET_CUR_FORM_BTN_ITEM(i) != ITEM_OCARINA_OF_TIME) &&
                            !((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_MASK_DEKU) &&
                              (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_MASK_GIANT)) &&
                            (GET_CUR_FORM_BTN_ITEM(i) != ITEM_PICTOGRAPH_BOX)) {

                            if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                                restoreHudVisibility = true;
                                gSaveContext.buttonStatus[i] = BTN_DISABLED;
                            }
                        }
                    } else if (interfaceCtx->restrictions.all == 0) {
                        if (!((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_MOONS_TEAR) &&
                              (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_PENDANT_OF_MEMORIES)) &&
                            !((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_BOTTLE) &&
                              (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_OBABA_DRINK)) &&
                            (GET_CUR_FORM_BTN_ITEM(i) != ITEM_OCARINA_OF_TIME) &&
                            !((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_MASK_DEKU) &&
                              (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_MASK_GIANT)) &&
                            (GET_CUR_FORM_BTN_ITEM(i) != ITEM_PICTOGRAPH_BOX)) {

                            if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                                restoreHudVisibility = true;
                                gSaveContext.buttonStatus[i] = BTN_ENABLED;
                            }
                        }
                    }
                }
            }
        }
    }

    if (restoreHudVisibility && (play->activeCamId == CAM_ID_MAIN) && (play->transitionTrigger == TRANS_TRIGGER_OFF) &&
        (play->transitionMode == TRANS_MODE_OFF)) {
        gSaveContext.hudVisibility = HUD_VISIBILITY_IDLE;
        Interface_SetHudVisibility(HUD_VISIBILITY_ALL);
    }
}

typedef struct {
    /* 0x0 */ s8 x;
    /* 0x1 */ s8 y;
} OcarinaControlStick; // size = 0x2

typedef enum {
    /* 0x0 */ SFX_CHANNEL_PLAYER0, // SfxPlayerBank
    /* 0x1 */ SFX_CHANNEL_PLAYER1,
    /* 0x2 */ SFX_CHANNEL_PLAYER2,
    /* 0x3 */ SFX_CHANNEL_ITEM0, // SfxItemBank
    /* 0x4 */ SFX_CHANNEL_ITEM1,
    /* 0x5 */ SFX_CHANNEL_ENV0, // SfxEnvironmentBank
    /* 0x6 */ SFX_CHANNEL_ENV1,
    /* 0x7 */ SFX_CHANNEL_ENV2,
    /* 0x8 */ SFX_CHANNEL_ENEMY0, // SfxEnemyBank
    /* 0x9 */ SFX_CHANNEL_ENEMY1,
    /* 0xA */ SFX_CHANNEL_ENEMY2,
    /* 0xB */ SFX_CHANNEL_SYSTEM0, // SfxSystemBank
    /* 0xC */ SFX_CHANNEL_SYSTEM1,
    /* 0xD */ SFX_CHANNEL_OCARINA, // SfxOcarinaBank
    /* 0xE */ SFX_CHANNEL_VOICE0,  // SfxVoiceBank
    /* 0xF */ SFX_CHANNEL_VOICE1
} SfxChannelIndex; // seqPlayerIndex = 2

extern u32 sOcarinaFlags;
extern u8 sOcarinaDropInputTimer;
extern u32 sOcarinaInputButtonStart;
extern u32 sOcarinaInputButtonCur;
extern u8 sCurOcarinaPitch;
extern u8 sCurOcarinaButtonIndex;
extern u32 sOcarinaInputButtonPrev;
extern s32 sOcarinaInputButtonPress; 
extern u8 sRecordingState;
extern s8 sCurOcarinaBendIndex;
extern f32 sCurOcarinaBendFreq;
extern s8 sCurOcarinaVibrato;
extern OcarinaControlStick sOcarinaInputStickRel;
extern u8 sPrevOcarinaPitch;
extern f32 sDefaultOcarinaVolume;
extern s8 sOcarinaInstrumentId;
extern f32 AudioOcarina_BendPitchTwoSemitones(s8 bendIndex);

// @recomp Patch the function in order to read DPad inputs for the ocarina as well as CButton inputs. 
RECOMP_PATCH void AudioOcarina_PlayControllerInput(u8 isOcarinaSfxSuppressedWhenCancelled) {
    u32 ocarinaBtnsHeld;

    // Prevents two different ocarina notes from being played on two consecutive frames
    if ((sOcarinaFlags != 0) && (sOcarinaDropInputTimer != 0)) {
        sOcarinaDropInputTimer--;
        return;
    }

    // Ensures the button pressed to start the ocarina does not also play an ocarina note
    // @recomp Check for DPad inputs as well.
    if ((sOcarinaInputButtonStart == 0) ||
        ((sOcarinaInputButtonStart & (BTN_A | BTN_CRIGHT | BTN_CLEFT | BTN_CDOWN | BTN_CUP | BTN_DRIGHT | BTN_DLEFT | BTN_DDOWN | BTN_DUP)) !=
         (sOcarinaInputButtonCur & (BTN_A | BTN_CRIGHT | BTN_CLEFT | BTN_CDOWN | BTN_CUP | BTN_DRIGHT | BTN_DLEFT | BTN_DDOWN | BTN_DUP)))) {
        sOcarinaInputButtonStart = 0;
        if (1) {}
        sCurOcarinaPitch = OCARINA_PITCH_NONE;
        sCurOcarinaButtonIndex = OCARINA_BTN_INVALID;
        // @recomp Check for DPad inputs as well.
        ocarinaBtnsHeld = (sOcarinaInputButtonCur & (BTN_A | BTN_CRIGHT | BTN_CLEFT | BTN_CDOWN | BTN_CUP | BTN_DRIGHT | BTN_DLEFT | BTN_DDOWN | BTN_DUP)) &
                          (sOcarinaInputButtonPrev & (BTN_A | BTN_CRIGHT | BTN_CLEFT | BTN_CDOWN | BTN_CUP | BTN_DRIGHT | BTN_DLEFT | BTN_DDOWN | BTN_DUP));

        if (!(sOcarinaInputButtonPress & ocarinaBtnsHeld) && (sOcarinaInputButtonCur != 0)) {
            sOcarinaInputButtonPress = sOcarinaInputButtonCur;
        } else {
            sOcarinaInputButtonPress &= ocarinaBtnsHeld;
        }

        // Interprets and transforms controller input into ocarina buttons and notes
        if (CHECK_BTN_ANY(sOcarinaInputButtonPress, BTN_A)) {
            sCurOcarinaPitch = OCARINA_PITCH_D4;
            sCurOcarinaButtonIndex = OCARINA_BTN_A;

        // @recomp Check for DPad down input as well.
        } else if (CHECK_BTN_ANY(sOcarinaInputButtonPress, (BTN_CDOWN | BTN_DDOWN))) {
            sCurOcarinaPitch = OCARINA_PITCH_F4;
            sCurOcarinaButtonIndex = OCARINA_BTN_C_DOWN;

        // @recomp Check for DPad right input as well.
        } else if (CHECK_BTN_ANY(sOcarinaInputButtonPress, BTN_CRIGHT | BTN_DRIGHT)) {
            sCurOcarinaPitch = OCARINA_PITCH_A4;
            sCurOcarinaButtonIndex = OCARINA_BTN_C_RIGHT;
            
        // @recomp Check for DPad left input as well.
        } else if (CHECK_BTN_ANY(sOcarinaInputButtonPress, BTN_CLEFT | BTN_DLEFT)) {
            sCurOcarinaPitch = OCARINA_PITCH_B4;
            sCurOcarinaButtonIndex = OCARINA_BTN_C_LEFT;

        // @recomp Check for DPad up input as well.
        } else if (CHECK_BTN_ANY(sOcarinaInputButtonPress, BTN_CUP | BTN_DUP)) {
            sCurOcarinaPitch = OCARINA_PITCH_D5;
            sCurOcarinaButtonIndex = OCARINA_BTN_C_UP;
        }

        if (sOcarinaInputButtonCur) {}

        // Pressing the R Button will raise the pitch by 1 semitone
        if ((sCurOcarinaPitch != OCARINA_PITCH_NONE) && CHECK_BTN_ANY(sOcarinaInputButtonCur, BTN_R) &&
            (sRecordingState != OCARINA_RECORD_SCARECROW_SPAWN)) {
            sCurOcarinaButtonIndex += OCARINA_BUTTON_FLAG_BFLAT_RAISE; // Flag to resolve B Flat 4
            sCurOcarinaPitch++;                                        // Raise the pitch by 1 semitone
        }

        // Pressing the Z Button will lower the pitch by 1 semitone
        if ((sCurOcarinaPitch != OCARINA_PITCH_NONE) && CHECK_BTN_ANY(sOcarinaInputButtonCur, BTN_Z) &&
            (sRecordingState != OCARINA_RECORD_SCARECROW_SPAWN)) {
            sCurOcarinaButtonIndex += OCARINA_BUTTON_FLAG_BFLAT_LOWER; // Flag to resolve B Flat 4
            sCurOcarinaPitch--;                                        // Lower the pitch by 1 semitone
        }

        if (sRecordingState != OCARINA_RECORD_SCARECROW_SPAWN) {
            // Bend the pitch of the note based on y control stick
            sCurOcarinaBendIndex = sOcarinaInputStickRel.y;
            sCurOcarinaBendFreq = AudioOcarina_BendPitchTwoSemitones(sCurOcarinaBendIndex);

            // Add vibrato of the ocarina note based on the x control stick
            sCurOcarinaVibrato = ABS_ALT(sOcarinaInputStickRel.x) >> 2;
            // Sets vibrato to io port 6
            AUDIOCMD_CHANNEL_SET_IO(SEQ_PLAYER_SFX, SFX_CHANNEL_OCARINA, 6, sCurOcarinaVibrato);
        } else {
            // no bending or vibrato for recording state OCARINA_RECORD_SCARECROW_SPAWN
            sCurOcarinaBendIndex = 0;
            sCurOcarinaVibrato = 0;
            sCurOcarinaBendFreq = 1.0f; // No bend
        }

        // Processes new and valid notes
        if ((sCurOcarinaPitch != OCARINA_PITCH_NONE) && (sPrevOcarinaPitch != sCurOcarinaPitch)) {
            // Sets ocarina instrument Id to io port 7, which is used
            // as an index in seq 0 to get the true instrument Id
            AUDIOCMD_CHANNEL_SET_IO(SEQ_PLAYER_SFX, SFX_CHANNEL_OCARINA, 7, sOcarinaInstrumentId - 1);
            // Sets pitch to io port 5
            AUDIOCMD_CHANNEL_SET_IO(SEQ_PLAYER_SFX, SFX_CHANNEL_OCARINA, 5, sCurOcarinaPitch);
            AudioSfx_PlaySfx(NA_SE_OC_OCARINA, &gSfxDefaultPos, 4, &sCurOcarinaBendFreq, &sDefaultOcarinaVolume,
                             &gSfxDefaultReverb);
        } else if ((sPrevOcarinaPitch != OCARINA_PITCH_NONE) && (sCurOcarinaPitch == OCARINA_PITCH_NONE) &&
                   !isOcarinaSfxSuppressedWhenCancelled) {
            // Stops ocarina sound when transitioning from playing to not playing a note
            AudioSfx_StopById(NA_SE_OC_OCARINA);
        }
    }
}

extern u8 sOcarinaHasStartedSong;
extern u8 sOcarinaStaffPlayingPos;
extern u8 sCurOcarinaSongWithoutMusicStaff[8];
extern u8 sOcarinaWithoutMusicStaffPos;
extern u8 sFirstOcarinaSongIndex;
extern u32 sOcarinaAvailableSongFlags;
extern u8 sLastOcarinaSongIndex;
extern u8 sButtonToPitchMap[5];
extern u8 sPlayedOcarinaSongIndexPlusOne;
extern u8 sIsOcarinaInputEnabled;
extern void AudioOcarina_CheckIfStartedSong(void);
extern void AudioOcarina_UpdateCurOcarinaSong(void);

// @recomp Patch the L button check (for free ocarina playing) to account for DPad ocarina.
RECOMP_PATCH void AudioOcarina_CheckSongsWithoutMusicStaff(void) {
    u32 pitch;
    u8 ocarinaStaffPlayingPosStart;
    u8 songIndex;
    u8 j;
    u8 k;

    // @recomp Add the DPad inputs to the check.
    if (CHECK_BTN_ANY(sOcarinaInputButtonCur, BTN_L) &&
        CHECK_BTN_ANY(sOcarinaInputButtonCur, BTN_A | BTN_CRIGHT | BTN_CLEFT | BTN_CDOWN | BTN_CUP | BTN_DRIGHT | BTN_DLEFT | BTN_DDOWN | BTN_DUP)) {
        AudioOcarina_StartDefault(sOcarinaFlags);
        return;
    }

    AudioOcarina_CheckIfStartedSong();

    if (!sOcarinaHasStartedSong) {
        return;
    }

    ocarinaStaffPlayingPosStart = sOcarinaStaffPlayingPos;
    if ((sPrevOcarinaPitch != sCurOcarinaPitch) && (sCurOcarinaPitch != OCARINA_PITCH_NONE)) {
        sOcarinaStaffPlayingPos++;
        if (sOcarinaStaffPlayingPos > ARRAY_COUNT(sCurOcarinaSongWithoutMusicStaff)) {
            sOcarinaStaffPlayingPos = 1;
        }

        AudioOcarina_UpdateCurOcarinaSong();

        if ((ABS_ALT(sCurOcarinaBendIndex) > 20) && (ocarinaStaffPlayingPosStart != sOcarinaStaffPlayingPos)) {
            sCurOcarinaSongWithoutMusicStaff[sOcarinaWithoutMusicStaffPos - 1] = OCARINA_PITCH_NONE;
        } else {
            sCurOcarinaSongWithoutMusicStaff[sOcarinaWithoutMusicStaffPos - 1] = sCurOcarinaPitch;
        }

        // This nested for-loop tests to see if the notes from the ocarina are identical
        // to any of the songIndex from sFirstOcarinaSongIndex to sLastOcarinaSongIndex

        // Loop through each of the songs
        for (songIndex = sFirstOcarinaSongIndex; songIndex < sLastOcarinaSongIndex; songIndex++) {
            // Checks to see if the song is available to be played
            if ((u32)sOcarinaAvailableSongFlags & (1 << songIndex)) {
                // Loops through all possible starting indices?
                // Loops through the notes of the song?
                for (j = 0, k = 0; (j < gOcarinaSongButtons[songIndex].numButtons) && (k == 0) &&
                                   (sOcarinaWithoutMusicStaffPos >= gOcarinaSongButtons[songIndex].numButtons);) {

                    pitch = sCurOcarinaSongWithoutMusicStaff[(sOcarinaWithoutMusicStaffPos -
                                                              gOcarinaSongButtons[songIndex].numButtons) +
                                                             j];

                    if (pitch == sButtonToPitchMap[gOcarinaSongButtons[songIndex].buttonIndex[j]]) {
                        j++;
                    } else {
                        k++;
                    }
                }

                // This conditional is true if songIndex = i is detected
                if (j == gOcarinaSongButtons[songIndex].numButtons) {
                    sPlayedOcarinaSongIndexPlusOne = songIndex + 1;
                    sIsOcarinaInputEnabled = false;
                    sOcarinaFlags = 0;
                }
            }
        }
    }
}

extern s32 Player_GetMovementSpeedAndYaw(Player* this, f32* outSpeedTarget, s16* outYawTarget, f32 speedMode,
                                  PlayState* play);
extern bool get_analog_cam_active();
extern void skip_analog_cam_once();

// @recomp Updates yaw while inside of deku flower.
RECOMP_PATCH void func_80855F9C(PlayState* play, Player* this) {
    f32 speedTarget;
    s16 yawTarget;

    this->stateFlags2 |= PLAYER_STATE2_20;
    Player_GetMovementSpeedAndYaw(this, &speedTarget, &yawTarget, 0.018f, play);

    // @recomp If left stick inputs are occurring, prevent analog cam.
    if ((play->state.input[0].rel.stick_y != 0 || play->state.input[0].rel.stick_x != 0)) {
        skip_analog_cam_once();
    }

    if (get_analog_cam_active()) {
        // @recomp set current yaw to active camera's yaw.
        this->currentYaw = Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));
    } else {
        Math_ScaledStepToS(&this->currentYaw, yawTarget, 0x258);
    }
}

extern void set_analog_cam_active(bool isActive);
extern void Player_Action_4(Player* this, PlayState* play);
extern s32 Player_SetAction(PlayState* play, Player* this, PlayerActionFunc actionFunc, s32 arg3);
extern LinkAnimationHeader gPlayerAnim_pg_maru_change;

RECOMP_PATCH s32 func_80857950(PlayState* play, Player* this) {
    // @recomp track if newly going from non-spike roll to spike roll (spike rolling when this->unk_B86[1] == 1)
    static bool wasOff = true;
    bool isOff = this->unk_B86[1] == 0;
    if (wasOff && !isOff) {
        // @recomp set analog cam to be active now that rolling has started
        set_analog_cam_active(false);
    }
    wasOff = isOff;

    if (((this->unk_B86[1] == 0) && !CHECK_BTN_ALL(sPlayerControlInput->cur.button, BTN_A)) ||
        ((this->av1.actionVar1 == 3) && (this->actor.velocity.y < 0.0f))) {
        Player_SetAction(play, this, Player_Action_4, 1);
        Math_Vec3f_Copy(&this->actor.world.pos, &this->actor.prevPos);
        PlayerAnimation_Change(play, &this->skelAnime, &gPlayerAnim_pg_maru_change, -2.0f / 3.0f, 7.0f, 0.0f,
                               ANIMMODE_ONCE, 0.0f);
        Player_PlaySfx(this, NA_SE_PL_BALL_TO_GORON);
        wasOff = true;
        return true;
    }

    return false;
}

typedef PlayerAnimationHeader* D_8085BE84_t[PLAYER_ANIMTYPE_MAX]; 
extern PlayerAnimationHeader* D_8085BE84[PLAYER_ANIMGROUP_MAX][PLAYER_ANIMTYPE_MAX];
extern LinkAnimationHeader gPlayerAnim_link_normal_backspace;

extern s32 func_80832F24(Player* this);
extern s32 func_8083FE38(Player* this, PlayState* play);
extern s32 Player_ActionChange_11(Player* this, PlayState* play);
extern void func_8083A98C(Actor* thisx, PlayState* play2);
extern void func_80836A98(Player* this, PlayerAnimationHeader* anim, PlayState* play);
extern void func_80830B38(Player* this);
extern void Player_AnimationPlayLoop(PlayState* play, Player* this, PlayerAnimationHeader* anim);
extern s32 Player_UpdateUpperBody(Player* this, PlayState* play);
extern void func_8082F164(Player* this, u16 button);
extern s32 func_808401F4(PlayState* play, Player* this);
extern void func_8082FA5C(PlayState* play, Player* this, PlayerMeleeWeaponState meleeWeaponState);
extern s32 func_8083FD80(Player* this, PlayState* play);
extern void func_8082DC38(Player* this);
extern void func_80836A5C(Player* this, PlayState* play);

// @recomp Patch the shielding function to respect the aiming axis inversion setting.
RECOMP_PATCH void Player_Action_18(Player* this, PlayState* play) {
    func_80832F24(this);

    if (this->transformation == PLAYER_FORM_GORON) {
        SkelAnime_Update(&this->unk_2C8);

        if (!func_8083FE38(this, play)) {
            if (!Player_ActionChange_11(this, play)) {
                this->stateFlags1 &= ~PLAYER_STATE1_400000;

                if (this->itemAction <= PLAYER_IA_MINUS1) {
                    func_80123C58(this);
                }

                func_80836A98(this, D_8085BE84[PLAYER_ANIMGROUP_defense_end][this->modelAnimType], play);
                func_80830B38(this);
            } else {
                this->stateFlags1 |= PLAYER_STATE1_400000;
            }
        }

        return;
    }

    if (PlayerAnimation_Update(play, &this->skelAnime)) {
        if (!Player_IsGoronOrDeku(this)) {
            Player_AnimationPlayLoop(play, this, D_8085BE84[PLAYER_ANIMGROUP_defense_wait][this->modelAnimType]);
        }

        this->av2.actionVar2 = 1;
        this->av1.actionVar1 = 0;
    }

    if (!Player_IsGoronOrDeku(this)) {
        this->stateFlags1 |= PLAYER_STATE1_400000;
        Player_UpdateUpperBody(this, play);
        this->stateFlags1 &= ~PLAYER_STATE1_400000;
        if (this->transformation == PLAYER_FORM_ZORA) {
            func_8082F164(this, BTN_R | BTN_B);
        }
    }

    if (this->av2.actionVar2 != 0) {
        f32 yStick = sPlayerControlInput->rel.stick_y * 180;
        f32 xStick = sPlayerControlInput->rel.stick_x * -120;
        s16 temp_a0 = this->actor.shape.rot.y - Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));
        s16 var_a1;
        s16 temp_ft5;
        s16 var_a2;
        s16 var_a3;
        // @recomp Get the aiming camera inversion state.
        s32 inverted_x, inverted_y;
        recomp_get_inverted_axes(&inverted_x, &inverted_y);

        // @recomp Invert the Y and X stick values based on the inverted aiming setting.
        if (!inverted_y) {
            yStick = -yStick;
        }
        if (inverted_x) {
            xStick = -xStick;
        }

        var_a1 = (yStick * Math_CosS(temp_a0)) + (Math_SinS(temp_a0) * xStick);
        temp_ft5 = (xStick * Math_CosS(temp_a0)) - (Math_SinS(temp_a0) * yStick);

        var_a1 = CLAMP_MAX(var_a1, 0xDAC);

        var_a2 = ABS_ALT(var_a1 - this->actor.focus.rot.x) * 0.25f;
        var_a2 = CLAMP_MIN(var_a2, 0x64);
        

        var_a3 = ABS_ALT(temp_ft5 - this->upperLimbRot.y) * 0.25f;
        var_a3 = CLAMP_MIN(var_a3, 0x32);

        Math_ScaledStepToS(&this->actor.focus.rot.x, var_a1, var_a2);

        this->upperLimbRot.x = this->actor.focus.rot.x;
        Math_ScaledStepToS(&this->upperLimbRot.y, temp_ft5, var_a3);

        if (this->av1.actionVar1 != 0) {
            if (!func_808401F4(play, this)) {
                if (this->skelAnime.curFrame < 2.0f) {
                    func_8082FA5C(play, this, PLAYER_MELEE_WEAPON_STATE_1);
                }
            } else {
                this->av2.actionVar2 = 1;
                this->av1.actionVar1 = 0;
            }
        } else if (!func_8083FE38(this, play)) {
            if (Player_ActionChange_11(this, play)) {
                func_8083FD80(this, play);
            } else {
                this->stateFlags1 &= ~PLAYER_STATE1_400000;
                func_8082DC38(this);

                if (Player_IsGoronOrDeku(this)) {
                    func_80836A5C(this, play);
                    PlayerAnimation_Change(play, &this->skelAnime, this->skelAnime.animation, 1.0f,
                                           Animation_GetLastFrame(this->skelAnime.animation), 0.0f, 2, 0.0f);
                } else {
                    if (this->itemAction <= PLAYER_IA_MINUS1) {
                        func_80123C58(this);
                    }

                    func_80836A98(this, D_8085BE84[PLAYER_ANIMGROUP_defense_end][this->modelAnimType], play);
                }

                Player_PlaySfx(this, NA_SE_IT_SHIELD_REMOVE);
                return;
            }
        } else {
            return;
        }
    }

    this->stateFlags1 |= PLAYER_STATE1_400000;
    Player_SetModelsForHoldingShield(this);
    this->unk_AA6 |= 0xC1;
}
