#include "patches.h"
#include "input.h"
#include "z64snap.h"
// Decomp rename, TODO update decomp and remove this
#define AudioVoice_GetWord func_801A5100
#include "z64voice.h"
#include "audiothread_cmd.h"

s32 func_80847190(PlayState* play, Player* this, s32 arg2);
s16 func_80832754(Player* this, s32 arg1);
s32 func_8082EF20(Player* this);

// @recomp Patched to add gyro and mouse aiming.
RECOMP_PATCH s32 func_80847190(PlayState* play, Player* this, s32 arg2) {
    s32 pad;
    s16 var_s0;
    // @recomp Get the aiming camera inversion state.
    s32 inverted_x, inverted_y;
    recomp_get_inverted_axes(&inverted_x, &inverted_y);
    // @recomp Get the analog camera input values if analog cam is enabled.
    s32 analog_x = 0;
    s32 analog_y = 0;
    if (recomp_analog_cam_enabled()) {
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

    if (!func_800B7128(this) && !func_8082EF20(this) && !arg2) {
        // @recomp Add in the analog camera Y input. Clamp to prevent moving the camera twice as fast if both sticks are held.
        var_s0 = CLAMP(play->state.input[0].rel.stick_y + analog_y, -61, 61) * 0xF0;
        
        // @recomp Invert the Y axis accordingly (default is inverted, so negate if not inverted).
        if (!inverted_y) {
            var_s0 = -var_s0;
        }
        Math_SmoothStepToS(&this->actor.focus.rot.x, var_s0, 0xE, 0xFA0, 0x1E);

        // @recomp Add in the analog camera X input. Clamp to prevent moving the camera twice as fast if both sticks are held.
        var_s0 = CLAMP(play->state.input[0].rel.stick_x + analog_x, -61, 61) * -0x10;

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
        s32 stick_y = CLAMP(play->state.input[0].rel.stick_y + analog_y, -61, 61);
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
        s32 stick_x = CLAMP(play->state.input[0].rel.stick_x + analog_x, -61, 61);
        if (inverted_x) {
            stick_x = -stick_x;
        }
        temp3 = ((stick_x >= 0) ? 1 : -1) *
            (s32)((1.0f - Math_CosS(stick_x * 0xC8)) * -1500.0f);
        var_s0 += temp3 + (s32)(target_aim_y - applied_aim_y);
        applied_aim_y = target_aim_y;

        this->actor.focus.rot.y = CLAMP(var_s0, -0x4AAA, 0x4AAA) + this->actor.shape.rot.y;
    }

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
    if (recomp_analog_cam_enabled()) {
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

u32 sPlayerItemButtons[] = {
    BTN_B,
    BTN_CLEFT,
    BTN_CDOWN,
    BTN_CRIGHT,
};

// D-Pad items

#define EXTRA_ITEM_SLOT_COUNT 4
#define TOTAL_SLOT_COUNT (3 + EXTRA_ITEM_SLOT_COUNT)
u8 extra_item_slot_statuses[EXTRA_ITEM_SLOT_COUNT];
s16 extra_item_slot_alphas[EXTRA_ITEM_SLOT_COUNT];
u8 extra_button_items[4][EXTRA_ITEM_SLOT_COUNT] = {
    { ITEM_MASK_DEKU, ITEM_MASK_GORON, ITEM_MASK_ZORA, ITEM_OCARINA_OF_TIME },
    { ITEM_MASK_DEKU, ITEM_MASK_GORON, ITEM_MASK_ZORA, ITEM_OCARINA_OF_TIME },
    { ITEM_MASK_DEKU, ITEM_MASK_GORON, ITEM_MASK_ZORA, ITEM_OCARINA_OF_TIME },
    { ITEM_MASK_DEKU, ITEM_MASK_GORON, ITEM_MASK_ZORA, ITEM_OCARINA_OF_TIME },
};

#define EQUIP_SLOT_EX_START ARRAY_COUNT(gSaveContext.buttonStatus)

typedef enum {
    EQUIP_SLOT_EX_DUP = EQUIP_SLOT_EX_START,
    EQUIP_SLOT_EX_DLEFT,
    EQUIP_SLOT_EX_DRIGHT,
    EQUIP_SLOT_EX_DDOWN,
} EquipSlotEx;

struct ExButtonMapping {
    u32 button;
    EquipSlotEx slot;
};

// These are negated to avoid a check where the game clamps the button to B if it's greater than 
struct ExButtonMapping buttons_to_extra_slot[] = {
    {BTN_DLEFT,  -EQUIP_SLOT_EX_DLEFT},
    {BTN_DRIGHT, -EQUIP_SLOT_EX_DRIGHT},
    {BTN_DUP,    -EQUIP_SLOT_EX_DUP},
    {BTN_DDOWN,  -EQUIP_SLOT_EX_DDOWN},
};

#undef BUTTON_ITEM_EQUIP
#undef GET_CUR_FORM_BTN_ITEM

#define BUTTON_ITEM_EQUIP(form, button) (*get_button_item_equip_ptr((form), (button)))
#define GET_CUR_FORM_BTN_ITEM(btn) ((u8)((btn) == EQUIP_SLOT_B ? BUTTON_ITEM_EQUIP(CUR_FORM, btn) : BUTTON_ITEM_EQUIP(0, btn)))
#define BUTTON_STATUS(btn) (*get_slot_status_ptr(btn))

// Analog to C_BTN_ITEM for extra item slots
#define EXTRA_BTN_ITEM(btn)                                 \
    ((extra_item_slot_statuses[(btn) - EQUIP_SLOT_EX_START] != BTN_DISABLED) \
         ? BUTTON_ITEM_EQUIP(0, (btn))                  \
         : ((gSaveContext.hudVisibility == HUD_VISIBILITY_A_B_C) ? BUTTON_ITEM_EQUIP(0, (btn)) : ITEM_NONE))

         
void set_extra_item_slot_status(u8 status) {
    for (int i = 0; i < EXTRA_ITEM_SLOT_COUNT; i++) {
        extra_item_slot_statuses[i] = status;
    }
}

u8* get_slot_status_ptr(u32 slot) {
    if (slot >= EQUIP_SLOT_EX_START) {
        return &extra_item_slot_statuses[slot - EQUIP_SLOT_EX_START];
    }
    else {
        return &gSaveContext.buttonStatus[slot];
    }
}

// Converts an ex slot index into the actual slot index.
u8 to_slot_index(u32 ex_slot) {
    if (ex_slot < 3) {
        return ex_slot + EQUIP_SLOT_C_LEFT;
    }
    else {
        return ex_slot - 3 + EQUIP_SLOT_EX_START;
    }
}

u8* get_button_item_equip_ptr(u32 form, u32 button) {
    if (button >= EQUIP_SLOT_EX_START) {
        return &extra_button_items[form][button - EQUIP_SLOT_EX_START];
    }
    else {
        return &gSaveContext.save.saveInfo.equips.buttonItems[form][button];
    }
}

// Return currently-pressed button, in order of priority D-Pad, B, CLEFT, CDOWN, CRIGHT.
RECOMP_PATCH EquipSlot func_8082FDC4(void) {
    EquipSlot i;

    for (int extra_slot_index = 0; extra_slot_index < ARRAY_COUNT(buttons_to_extra_slot); extra_slot_index++) {
        if (CHECK_BTN_ALL(sPlayerControlInput->press.button, buttons_to_extra_slot[extra_slot_index].button)) {
            return (EquipSlot)buttons_to_extra_slot[extra_slot_index].slot;
        }
    }

    for (i = 0; i < ARRAY_COUNT(sPlayerItemButtons); i++) {
        if (CHECK_BTN_ALL(sPlayerControlInput->press.button, sPlayerItemButtons[i])) {
            break;
        }
    }

    return i;
}

RECOMP_PATCH ItemId Player_GetItemOnButton(PlayState* play, Player* player, EquipSlot slot) {
    if (slot >= EQUIP_SLOT_A) {
        return ITEM_NONE;
    }

    // @recomp Check for extra item slots.
    if (slot <= -EQUIP_SLOT_EX_START) {
        ItemId item = EXTRA_BTN_ITEM(-slot);

        // Ensure the item was valid and has been obtained.
        if ((item != ITEM_NONE) && (INV_CONTENT(item) == item)) {
            return item;
        }
        else {
            return ITEM_NONE;
        }
    }

    if (slot == EQUIP_SLOT_B) {
        ItemId item = Inventory_GetBtnBItem(play);

        if (item >= ITEM_FD) {
            return item;
        }

        if ((player->currentMask == PLAYER_MASK_BLAST) && (play->interfaceCtx.bButtonDoAction == DO_ACTION_EXPLODE)) {
            return ITEM_F0;
        }

        if ((player->currentMask == PLAYER_MASK_BREMEN) && (play->interfaceCtx.bButtonDoAction == DO_ACTION_MARCH)) {
            return ITEM_F1;
        }

        if ((player->currentMask == PLAYER_MASK_KAMARO) && (play->interfaceCtx.bButtonDoAction == DO_ACTION_DANCE)) {
            return ITEM_F2;
        }

        return item;
    }

    if (slot == EQUIP_SLOT_C_LEFT) {
        return C_BTN_ITEM(EQUIP_SLOT_C_LEFT);
    }

    if (slot == EQUIP_SLOT_C_DOWN) {
        return C_BTN_ITEM(EQUIP_SLOT_C_DOWN);
    }

    // EQUIP_SLOT_C_RIGHT

    return C_BTN_ITEM(EQUIP_SLOT_C_RIGHT);
}

typedef struct struct_8085D910 {
    /* 0x0 */ u8 unk_0;
    /* 0x1 */ u8 unk_1;
    /* 0x2 */ u8 unk_2;
    /* 0x3 */ u8 unk_3;
} struct_8085D910; // size = 0x4

u16 D_8085D908[] = {
    WEEKEVENTREG_30_80, // PLAYER_FORM_FIERCE_DEITY
    WEEKEVENTREG_30_20, // PLAYER_FORM_GORON
    WEEKEVENTREG_30_40, // PLAYER_FORM_ZORA
    WEEKEVENTREG_30_10, // PLAYER_FORM_DEKU
};
struct_8085D910 D_8085D910[] = {
    { 0x10, 0xA, 0x3B, 0x3F },
    { 9, 0x32, 0xA, 0xD },
};

bool func_808323C0(Player *this, s16 csId);
void func_80855218(PlayState *play, Player *this, struct_8085D910 **arg2);
void func_808550D0(PlayState *play, Player *this, f32 arg2, f32 arg3, s32 arg4);

RECOMP_PATCH void Player_Action_86(Player *this, PlayState *play) {
    struct_8085D910 *sp4C = D_8085D910;
    s32 sp48 = false;

    func_808323C0(this, play->playerCsIds[PLAYER_CS_ID_MASK_TRANSFORMATION]);
    sPlayerControlInput = play->state.input;

    Camera_ChangeMode(GET_ACTIVE_CAM(play),
        (this->transformation == PLAYER_FORM_HUMAN) ? CAM_MODE_NORMAL : CAM_MODE_JUMP);
    this->stateFlags2 |= PLAYER_STATE2_40;
    this->actor.shape.rot.y = Camera_GetCamDirYaw(GET_ACTIVE_CAM(play)) + 0x8000;

    func_80855218(play, this, &sp4C);

    if (this->av1.actionVar1 == 0x14) {
        Play_EnableMotionBlurPriority(100);
    }

    if (R_PLAY_FILL_SCREEN_ON != 0) {
        R_PLAY_FILL_SCREEN_ALPHA += R_PLAY_FILL_SCREEN_ON;
        if (R_PLAY_FILL_SCREEN_ALPHA > 255) {
            R_PLAY_FILL_SCREEN_ALPHA = 255;
            this->actor.update = func_8012301C;
            this->actor.draw = NULL;
            this->av1.actionVar1 = 0;
            Play_DisableMotionBlurPriority();
            SET_WEEKEVENTREG(D_8085D908[GET_PLAYER_FORM]);
        }
    }
    else if ((this->av1.actionVar1++ > ((this->transformation == PLAYER_FORM_HUMAN) ? 0x53 : 0x37)) ||
        ((this->av1.actionVar1 >= 5) &&
            (sp48 =
                ((this->transformation != PLAYER_FORM_HUMAN) || CHECK_WEEKEVENTREG(D_8085D908[GET_PLAYER_FORM])) &&
                // @recomp Patched to also check for d-pad buttons for skipping the transformation cutscene.
                CHECK_BTN_ANY(play->state.input[0].press.button,
                    BTN_CRIGHT | BTN_CLEFT | BTN_CDOWN | BTN_CUP | BTN_B | BTN_A | BTN_DRIGHT | BTN_DLEFT | BTN_DDOWN | BTN_DUP)))) {
        R_PLAY_FILL_SCREEN_ON = 45;
        R_PLAY_FILL_SCREEN_R = 220;
        R_PLAY_FILL_SCREEN_G = 220;
        R_PLAY_FILL_SCREEN_B = 220;
        R_PLAY_FILL_SCREEN_ALPHA = 0;

        if (sp48) {
            if (CutsceneManager_GetCurrentCsId() == this->csId) {
                func_800E0348(Play_GetCamera(play, CutsceneManager_GetCurrentSubCamId(this->csId)));
            }

            if (this->transformation == PLAYER_FORM_HUMAN) {
                AudioSfx_StopById(NA_SE_PL_TRANSFORM_VOICE);
                AudioSfx_StopById(NA_SE_IT_TRANSFORM_MASK_BROKEN);
            }
            else {
                AudioSfx_StopById(NA_SE_PL_FACE_CHANGE);
            }
        }

        Player_PlaySfx(this, NA_SE_SY_TRANSFORM_MASK_FLASH);
    }

    if (this->av1.actionVar1 >= sp4C->unk_0) {
        if (this->av1.actionVar1 < sp4C->unk_2) {
            Math_StepToF(&this->unk_B10[4], 1.0f, sp4C->unk_1 / 100.0f);
        }
        else if (this->av1.actionVar1 < sp4C->unk_3) {
            if (this->av1.actionVar1 == sp4C->unk_2) {
                Lib_PlaySfx_2(NA_SE_EV_LIGHTNING_HARD);
            }

            Math_StepToF(&this->unk_B10[4], 2.0f, 0.5f);
        }
        else {
            Math_StepToF(&this->unk_B10[4], 3.0f, 0.2f);
        }
    }

    if (this->av1.actionVar1 >= 0x10) {
        if (this->av1.actionVar1 < 0x40) {
            Math_StepToF(&this->unk_B10[5], 1.0f, 0.2f);
        }
        else if (this->av1.actionVar1 < 0x37) {
            Math_StepToF(&this->unk_B10[5], 2.0f, 1.0f);
        }
        else {
            Math_StepToF(&this->unk_B10[5], 3.0f, 0.55f);
        }
    }

    func_808550D0(play, this, this->unk_B10[4], this->unk_B10[5], (this->transformation == PLAYER_FORM_HUMAN) ? 0 : 1);
}

extern s16 sPictoState;
extern s16 sPictoPhotoBeingTaken;
extern void* gWorkBuffer;
u16 func_801A5100(void);

// @recomp Patched to update status of extra buttons via set_extra_item_slot_status.
RECOMP_PATCH void Interface_UpdateButtonsPart1(PlayState* play) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    Player* player = GET_PLAYER(play);
    s32 pad;
    s32 restoreHudVisibility = false;

    if (gSaveContext.save.cutsceneIndex < 0xFFF0) {
        gSaveContext.hudVisibilityForceButtonAlphasByStatus = false;
        if ((player->stateFlags1 & PLAYER_STATE1_800000) || CHECK_WEEKEVENTREG(WEEKEVENTREG_08_01) ||
            (!CHECK_EVENTINF(EVENTINF_41) && (play->unk_1887C >= 2))) {
            // Riding Epona OR Honey & Darling minigame OR Horseback balloon minigame OR related to swamp boat
            // (non-minigame?)
            if ((player->stateFlags1 & PLAYER_STATE1_800000) && (player->currentMask == PLAYER_MASK_BLAST) &&
                (gSaveContext.bButtonStatus == BTN_DISABLED)) {
                // Riding Epona with blast mask?
                restoreHudVisibility = true;
                gSaveContext.bButtonStatus = BTN_ENABLED;
            }

            if (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) != ITEM_NONE) {
                if ((player->transformation == PLAYER_FORM_DEKU) && CHECK_WEEKEVENTREG(WEEKEVENTREG_08_01)) {
                    gSaveContext.hudVisibilityForceButtonAlphasByStatus = true;
                    if (play->sceneId == SCENE_BOWLING) {
                        if (BUTTON_STATUS(EQUIP_SLOT_B) == BTN_DISABLED) {
                            BUTTON_STATUS(EQUIP_SLOT_B) = BTN_ENABLED;
                            BUTTON_STATUS(EQUIP_SLOT_C_LEFT) = BTN_DISABLED;
                            BUTTON_STATUS(EQUIP_SLOT_C_DOWN) = BTN_DISABLED;
                            BUTTON_STATUS(EQUIP_SLOT_C_RIGHT) = BTN_DISABLED;
                            set_extra_item_slot_status(BTN_DISABLED);
                        }
                    } else if (BUTTON_STATUS(EQUIP_SLOT_B) == BTN_DISABLED) {
                        BUTTON_STATUS(EQUIP_SLOT_B) = BTN_ENABLED;
                        BUTTON_STATUS(EQUIP_SLOT_C_LEFT) = BTN_ENABLED;
                        BUTTON_STATUS(EQUIP_SLOT_C_DOWN) = BTN_ENABLED;
                        BUTTON_STATUS(EQUIP_SLOT_C_RIGHT) = BTN_ENABLED;
                        set_extra_item_slot_status(BTN_ENABLED);
                    }

                    Interface_SetHudVisibility(HUD_VISIBILITY_B_MAGIC);
                } else {
                    if ((BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) != ITEM_BOW) &&
                        (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) != ITEM_BOMB) &&
                        (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) != ITEM_BOMBCHU)) {
                        gSaveContext.hudVisibilityForceButtonAlphasByStatus = true;
                        BUTTON_STATUS(EQUIP_SLOT_B) = BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B);
                        BUTTON_STATUS(EQUIP_SLOT_C_LEFT) = BTN_ENABLED;
                        BUTTON_STATUS(EQUIP_SLOT_C_DOWN) = BTN_ENABLED;
                        BUTTON_STATUS(EQUIP_SLOT_C_RIGHT) = BTN_ENABLED;
                        set_extra_item_slot_status(BTN_ENABLED);
                        if (play->sceneId == SCENE_BOWLING) {
                            if (CURRENT_DAY == 1) {
                                BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_BOMBCHU;
                            } else if (CURRENT_DAY == 2) {
                                BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_BOMB;
                            } else {
                                BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_BOW;
                            }
                            Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
                            BUTTON_STATUS(EQUIP_SLOT_C_LEFT) = BTN_DISABLED;
                            BUTTON_STATUS(EQUIP_SLOT_C_DOWN) = BTN_DISABLED;
                            BUTTON_STATUS(EQUIP_SLOT_C_RIGHT) = BTN_DISABLED;
                            set_extra_item_slot_status(BTN_DISABLED);
                        } else {
                            BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_BOW;

                            if (play->unk_1887C >= 2) {
                                Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
                            } else if (gSaveContext.save.saveInfo.inventory.items[SLOT_BOW] == ITEM_NONE) {
                                BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_NONE;
                            } else {
                                Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
                            }

                            BUTTON_STATUS(EQUIP_SLOT_C_LEFT) = BTN_DISABLED;
                            BUTTON_STATUS(EQUIP_SLOT_C_DOWN) = BTN_DISABLED;
                            BUTTON_STATUS(EQUIP_SLOT_C_RIGHT) = BTN_DISABLED;
                            set_extra_item_slot_status(BTN_DISABLED);
                            Interface_SetHudVisibility(HUD_VISIBILITY_A_HEARTS_MAGIC_MINIMAP_WITH_OVERWRITE);
                        }
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
                        BUTTON_STATUS(EQUIP_SLOT_C_LEFT) = BTN_DISABLED;
                        BUTTON_STATUS(EQUIP_SLOT_C_DOWN) = BTN_DISABLED;
                        BUTTON_STATUS(EQUIP_SLOT_C_RIGHT) = BTN_DISABLED;
                        set_extra_item_slot_status(BTN_DISABLED);
                        Interface_SetHudVisibility(HUD_VISIBILITY_A_B_MINIMAP);
                    } else if (player->stateFlags1 & PLAYER_STATE1_800000) {
                        Interface_SetHudVisibility(HUD_VISIBILITY_A_B_MINIMAP);
                    }
                }
            } else {
                if (player->stateFlags1 & PLAYER_STATE1_800000) {
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
                    BUTTON_STATUS(EQUIP_SLOT_C_LEFT) = BTN_DISABLED;
                    BUTTON_STATUS(EQUIP_SLOT_C_DOWN) = BTN_DISABLED;
                    BUTTON_STATUS(EQUIP_SLOT_C_RIGHT) = BTN_DISABLED;
                    set_extra_item_slot_status(BTN_DISABLED);
                } else {
                    BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_BOW;
                }

                if (play->unk_1887C >= 2) {
                    Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
                } else if (gSaveContext.save.saveInfo.inventory.items[SLOT_BOW] == ITEM_NONE) {
                    BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_NONE;
                } else {
                    Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
                }

                if (BUTTON_STATUS(EQUIP_SLOT_B) == BTN_DISABLED) {
                    BUTTON_STATUS(EQUIP_SLOT_B) = BTN_ENABLED;
                    restoreHudVisibility = true;
                }

                BUTTON_STATUS(EQUIP_SLOT_C_LEFT) = BTN_DISABLED;
                BUTTON_STATUS(EQUIP_SLOT_C_DOWN) = BTN_DISABLED;
                BUTTON_STATUS(EQUIP_SLOT_C_RIGHT) = BTN_DISABLED;
                set_extra_item_slot_status(BTN_DISABLED);
                Interface_SetHudVisibility(HUD_VISIBILITY_A_HEARTS_MAGIC_MINIMAP_WITH_OVERWRITE);

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
                    BUTTON_STATUS(EQUIP_SLOT_C_LEFT) = BTN_DISABLED;
                    BUTTON_STATUS(EQUIP_SLOT_C_DOWN) = BTN_DISABLED;
                    BUTTON_STATUS(EQUIP_SLOT_C_RIGHT) = BTN_DISABLED;
                    set_extra_item_slot_status(BTN_DISABLED);
                    Interface_SetHudVisibility(HUD_VISIBILITY_A_B_MINIMAP);
                } else if (player->stateFlags1 & PLAYER_STATE1_800000) {
                    Interface_SetHudVisibility(HUD_VISIBILITY_A_B_MINIMAP);
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
            BUTTON_STATUS(EQUIP_SLOT_C_LEFT) = BTN_DISABLED;
            BUTTON_STATUS(EQUIP_SLOT_C_DOWN) = BTN_DISABLED;
            BUTTON_STATUS(EQUIP_SLOT_C_RIGHT) = BTN_DISABLED;
            set_extra_item_slot_status(BTN_DISABLED);
            Interface_SetHudVisibility(HUD_VISIBILITY_A_B_MINIMAP);
        } else if ((gSaveContext.save.entrance == ENTRANCE(GORON_RACETRACK, 1)) &&
                   (play->transitionTrigger == TRANS_TRIGGER_OFF) && (play->transitionMode == TRANS_MODE_OFF)) {
            // Goron race minigame
            BUTTON_STATUS(EQUIP_SLOT_C_LEFT) = BTN_DISABLED;
            BUTTON_STATUS(EQUIP_SLOT_C_DOWN) = BTN_DISABLED;
            BUTTON_STATUS(EQUIP_SLOT_C_RIGHT) = BTN_DISABLED;
            set_extra_item_slot_status(BTN_DISABLED);
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

/**
 * A continuation of the if-else chain from Interface_UpdateButtonsPart1
 * Also used directly when opening the pause menu i.e. skips part 1
 */
// @recomp Patched in the same way as Interface_UpdateButtonsPart1
RECOMP_PATCH void Interface_UpdateButtonsPart2(PlayState* play) {
    MessageContext* msgCtx = &play->msgCtx;
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    Player* player = GET_PLAYER(play);
    s16 e;
    s16 restoreHudVisibility = false;

    if (CHECK_EVENTINF(EVENTINF_41)) {
        // Related to swamp boat (non-minigame)?
        for (e = 0; e < TOTAL_SLOT_COUNT; e++) {
            s16 i = to_slot_index(e);
            if ((GET_CUR_FORM_BTN_ITEM(i) != ITEM_PICTOGRAPH_BOX) || (msgCtx->msgMode != MSGMODE_NONE)) {
                if (BUTTON_STATUS(i) == BTN_ENABLED) {
                    restoreHudVisibility = true;
                }
                BUTTON_STATUS(i) = BTN_DISABLED;
            } else {
                if (BUTTON_STATUS(i) == BTN_DISABLED) {
                    restoreHudVisibility = true;
                }
                BUTTON_STATUS(i) = BTN_ENABLED;
            }
        }

        if (sPictoState == PICTO_BOX_STATE_OFF) {
            if (BUTTON_STATUS(EQUIP_SLOT_B) != BTN_DISABLED) {
                restoreHudVisibility = true;
            }
            BUTTON_STATUS(EQUIP_SLOT_B) = BTN_DISABLED;
        } else {
            if (BUTTON_STATUS(EQUIP_SLOT_B) == BTN_DISABLED) {
                restoreHudVisibility = true;
            }
            BUTTON_STATUS(EQUIP_SLOT_B) = BTN_ENABLED;
        }
    } else if (CHECK_WEEKEVENTREG(WEEKEVENTREG_90_20)) {
        // Fishermans's jumping minigame
        for (e = 0; e < TOTAL_SLOT_COUNT; e++) {
            s16 i = to_slot_index(e);
            if (BUTTON_STATUS(i) == BTN_ENABLED) {
                BUTTON_STATUS(i) = BTN_DISABLED;
            }
        }

        Interface_SetHudVisibility(HUD_VISIBILITY_B);
    } else if (CHECK_WEEKEVENTREG(WEEKEVENTREG_82_08)) {
        // Swordsman's log minigame
        for (e = 0; e < TOTAL_SLOT_COUNT; e++) {
            s16 i = to_slot_index(e);
            if (BUTTON_STATUS(i) == BTN_ENABLED) {
                BUTTON_STATUS(i) = BTN_DISABLED;
            }
        }

        Interface_SetHudVisibility(HUD_VISIBILITY_A_B_HEARTS_MAGIC_MINIMAP);
    } else if (CHECK_WEEKEVENTREG(WEEKEVENTREG_84_20)) {
        // Related to moon child
        if (player->currentMask == PLAYER_MASK_FIERCE_DEITY) {
            for (e = 0; e < TOTAL_SLOT_COUNT; e++) {
                s16 i = to_slot_index(e);
                if ((GET_CUR_FORM_BTN_ITEM(i) == ITEM_MASK_FIERCE_DEITY) ||
                    ((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_BOTTLE) && (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_OBABA_DRINK))) {
                    if (BUTTON_STATUS(i) == BTN_DISABLED) {
                        restoreHudVisibility = true;
                        BUTTON_STATUS(i) = BTN_ENABLED;
                    }
                } else {
                    if (BUTTON_STATUS(i) != BTN_DISABLED) {
                        BUTTON_STATUS(i) = BTN_DISABLED;
                        restoreHudVisibility = true;
                    }
                }
            }
        } else {
            for (e = 0; e < TOTAL_SLOT_COUNT; e++) {
                s16 i = to_slot_index(e);
                if ((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_MASK_DEKU) && (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_MASK_ZORA)) {
                    if (BUTTON_STATUS(i) != BTN_DISABLED) {
                        restoreHudVisibility = true;
                    }
                    BUTTON_STATUS(i) = BTN_DISABLED;
                } else {
                    if (BUTTON_STATUS(i) == BTN_DISABLED) {
                        restoreHudVisibility = true;
                    }
                    BUTTON_STATUS(i) = BTN_ENABLED;
                }
            }
        }
    } else if ((play->sceneId == SCENE_SPOT00) && (gSaveContext.sceneLayer == 6)) {
        // Unknown cutscene
        for (e = 0; e < EQUIP_SLOT_C_RIGHT; e++) {
            s16 i = to_slot_index(e);
            if (BUTTON_STATUS(i) == BTN_ENABLED) {
                restoreHudVisibility = true;
            }
            BUTTON_STATUS(i) = BTN_DISABLED;
        }
    } else if (CHECK_EVENTINF(EVENTINF_34)) {
        // Deku playground minigame
        if (player->stateFlags3 & PLAYER_STATE3_1000000) {
            if (gSaveContext.save.saveInfo.inventory.items[SLOT_DEKU_NUT] == ITEM_DEKU_NUT) {
                BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_DEKU_NUT;
                Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
            } else {
                BUTTON_STATUS(EQUIP_SLOT_B) = BTN_DISABLED;
                restoreHudVisibility = true;
            }
        } else {
            if (BUTTON_STATUS(EQUIP_SLOT_B) == BTN_DISABLED) {
                BUTTON_STATUS(EQUIP_SLOT_B) = BTN_ENABLED;
                restoreHudVisibility = true;
            }

            for (e = 0; e < TOTAL_SLOT_COUNT; e++) {
                s16 i = to_slot_index(e);
                if (BUTTON_STATUS(i) == BTN_ENABLED) {
                    restoreHudVisibility = true;
                }
                BUTTON_STATUS(i) = BTN_DISABLED;
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
                BUTTON_STATUS(EQUIP_SLOT_B) = BTN_ENABLED;
                restoreHudVisibility = true;
            }
        } else if (BUTTON_STATUS(EQUIP_SLOT_B) == BTN_ENABLED) {
            BUTTON_STATUS(EQUIP_SLOT_B) = BTN_DISABLED;
            restoreHudVisibility = true;
        }

        if (restoreHudVisibility) {
            BUTTON_STATUS(EQUIP_SLOT_C_LEFT) = BTN_DISABLED;
            BUTTON_STATUS(EQUIP_SLOT_C_DOWN) = BTN_DISABLED;
            BUTTON_STATUS(EQUIP_SLOT_C_RIGHT) = BTN_DISABLED;
            set_extra_item_slot_status(BTN_DISABLED);
        }
    } else if (!gSaveContext.save.saveInfo.playerData.isMagicAcquired && (CUR_FORM == PLAYER_FORM_DEKU) &&
               (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_DEKU_NUT)) {
        // Nuts on B (as Deku Link)
        BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_FD;
        BUTTON_STATUS(EQUIP_SLOT_B) = BTN_DISABLED;
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
                if (BUTTON_STATUS(EQUIP_SLOT_B) != BTN_DISABLED) {
                    restoreHudVisibility = true;
                }
                BUTTON_STATUS(EQUIP_SLOT_B) = BTN_DISABLED;
            }
        } else {
            if (BUTTON_STATUS(EQUIP_SLOT_B) == BTN_DISABLED) {
                restoreHudVisibility = true;
            }
            BUTTON_STATUS(EQUIP_SLOT_B) = BTN_ENABLED;
        }

        for (e = 0; e < TOTAL_SLOT_COUNT; e++) {
            s16 i = to_slot_index(e);
            if (GET_CUR_FORM_BTN_ITEM(i) != ITEM_MASK_ZORA) {
                if (Player_GetEnvironmentalHazard(play) == PLAYER_ENV_HAZARD_UNDERWATER_FLOOR) {
                    if (!((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_BOTTLE) &&
                          (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_OBABA_DRINK))) {
                        if (BUTTON_STATUS(i) == BTN_ENABLED) {
                            restoreHudVisibility = true;
                        }
                        BUTTON_STATUS(i) = BTN_DISABLED;
                    } else {
                        if (BUTTON_STATUS(i) == BTN_DISABLED) {
                            restoreHudVisibility = true;
                        }
                        BUTTON_STATUS(i) = BTN_ENABLED;
                    }
                } else {
                    if (BUTTON_STATUS(i) == BTN_ENABLED) {
                        restoreHudVisibility = true;
                    }
                    BUTTON_STATUS(i) = BTN_DISABLED;
                }
            } else if (BUTTON_STATUS(i) == BTN_DISABLED) {
                BUTTON_STATUS(i) = BTN_ENABLED;
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
        for (e = 0; e < TOTAL_SLOT_COUNT; e++) {
            s16 i = to_slot_index(e);
            if (GET_CUR_FORM_BTN_ITEM(i) != ITEM_LENS_OF_TRUTH) {
                if (BUTTON_STATUS(i) == BTN_ENABLED) {
                    restoreHudVisibility = true;
                }
                BUTTON_STATUS(i) = BTN_DISABLED;
            } else {
                if (BUTTON_STATUS(i) == BTN_DISABLED) {
                    restoreHudVisibility = true;
                }
                BUTTON_STATUS(i) = BTN_ENABLED;
            }
        }

        if (BUTTON_STATUS(EQUIP_SLOT_B) != BTN_DISABLED) {
            BUTTON_STATUS(EQUIP_SLOT_B) = BTN_DISABLED;
            restoreHudVisibility = true;
        }
    } else if (player->stateFlags1 & PLAYER_STATE1_2000) {
        // Hanging from a ledge
        if (BUTTON_STATUS(EQUIP_SLOT_B) != BTN_DISABLED) {
            BUTTON_STATUS(EQUIP_SLOT_B) = BTN_DISABLED;
            BUTTON_STATUS(EQUIP_SLOT_C_LEFT) = BTN_DISABLED;
            BUTTON_STATUS(EQUIP_SLOT_C_DOWN) = BTN_DISABLED;
            BUTTON_STATUS(EQUIP_SLOT_C_RIGHT) = BTN_DISABLED;
            set_extra_item_slot_status(BTN_DISABLED);
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
                        BUTTON_STATUS(EQUIP_SLOT_B) = BTN_DISABLED;
                    }

                    if (BUTTON_STATUS(EQUIP_SLOT_B) == BTN_ENABLED) {
                        BUTTON_STATUS(EQUIP_SLOT_B) =
                            ITEM_SWORD_KOKIRI + GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) - EQUIP_VALUE_SWORD_KOKIRI;
                    }

                    BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = BUTTON_STATUS(EQUIP_SLOT_B);

                    if (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) != ITEM_NONE) {
                        Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
                    }
                    restoreHudVisibility = true;
                } else if (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_NONE) {
                    if (interfaceCtx->bButtonDoAction != 0) {
                        if (BUTTON_STATUS(EQUIP_SLOT_B) == BTN_DISABLED) {
                            restoreHudVisibility = true;
                            BUTTON_STATUS(EQUIP_SLOT_B) = BTN_ENABLED;
                        }
                    } else {
                        if (BUTTON_STATUS(EQUIP_SLOT_B) != BTN_DISABLED) {
                            restoreHudVisibility = true;
                            BUTTON_STATUS(EQUIP_SLOT_B) = BTN_DISABLED;
                        }
                    }
                } else if (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_NONE) {
                    if (BUTTON_STATUS(EQUIP_SLOT_B) != BTN_DISABLED) {
                        restoreHudVisibility = true;
                        BUTTON_STATUS(EQUIP_SLOT_B) = BTN_DISABLED;
                    }
                } else {
                    if (BUTTON_STATUS(EQUIP_SLOT_B) == BTN_DISABLED) {
                        restoreHudVisibility = true;
                        BUTTON_STATUS(EQUIP_SLOT_B) = BTN_ENABLED;
                    }
                }
            } else if (interfaceCtx->restrictions.bButton != 0) {
                if ((BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_BOW) ||
                    (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_BOMB) ||
                    (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) == ITEM_BOMBCHU)) {
                    if (GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) == EQUIP_VALUE_SWORD_NONE) {
                        BUTTON_STATUS(EQUIP_SLOT_B) = BTN_DISABLED;
                    }

                    BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = BUTTON_STATUS(EQUIP_SLOT_B);

                    if (BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) != ITEM_NONE) {
                        Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
                    }
                    restoreHudVisibility = true;
                }
                if (BUTTON_STATUS(EQUIP_SLOT_B) != BTN_DISABLED) {
                    BUTTON_STATUS(EQUIP_SLOT_B) = BTN_DISABLED;
                    restoreHudVisibility = true;
                }
            }
        }

        // C buttons
        if (GET_PLAYER_FORM == player->transformation) {
            for (e = 0; e < TOTAL_SLOT_COUNT; e++) {
                s16 i = to_slot_index(e);
                // Individual C button
                if (!gPlayerFormItemRestrictions[GET_PLAYER_FORM][GET_CUR_FORM_BTN_ITEM(i)]) {
                    // Item not usable in current playerForm
                    if (BUTTON_STATUS(i) != BTN_DISABLED) {
                        BUTTON_STATUS(i) = BTN_DISABLED;
                        restoreHudVisibility = true;
                    }
                } else if (player->actor.id != ACTOR_PLAYER) {
                    // Currently not playing as the main player
                    if (BUTTON_STATUS(i) != BTN_DISABLED) {
                        BUTTON_STATUS(i) = BTN_DISABLED;
                        restoreHudVisibility = true;
                    }
                } else if (player->currentMask == PLAYER_MASK_GIANT) {
                    // Currently wearing Giant's Mask
                    if (GET_CUR_FORM_BTN_ITEM(i) != ITEM_MASK_GIANT) {
                        if (BUTTON_STATUS(i) != BTN_DISABLED) {
                            BUTTON_STATUS(i) = BTN_DISABLED;
                            restoreHudVisibility = true;
                        }
                    } else if (BUTTON_STATUS(i) == BTN_DISABLED) {
                        restoreHudVisibility = true;
                        BUTTON_STATUS(i) = BTN_ENABLED;
                    }
                } else if (GET_CUR_FORM_BTN_ITEM(i) == ITEM_MASK_GIANT) {
                    // Giant's Mask is equipped
                    if (play->sceneId != SCENE_INISIE_BS) {
                        if (BUTTON_STATUS(i) != BTN_DISABLED) {
                            BUTTON_STATUS(i) = BTN_DISABLED;
                            restoreHudVisibility = true;
                        }
                    } else if (BUTTON_STATUS(i) == BTN_DISABLED) {
                        restoreHudVisibility = true;
                        BUTTON_STATUS(i) = BTN_ENABLED;
                    }
                } else if (GET_CUR_FORM_BTN_ITEM(i) == ITEM_MASK_FIERCE_DEITY) {
                    // Fierce Deity's Mask is equipped
                    if ((play->sceneId != SCENE_MITURIN_BS) && (play->sceneId != SCENE_HAKUGIN_BS) &&
                        (play->sceneId != SCENE_SEA_BS) && (play->sceneId != SCENE_INISIE_BS) &&
                        (play->sceneId != SCENE_LAST_BS)) {
                        if (BUTTON_STATUS(i) != BTN_DISABLED) {
                            BUTTON_STATUS(i) = BTN_DISABLED;
                            restoreHudVisibility = true;
                        }
                    } else if (BUTTON_STATUS(i) == BTN_DISABLED) {
                        restoreHudVisibility = true;
                        BUTTON_STATUS(i) = BTN_ENABLED;
                    }
                } else {
                    // End of special item cases. Apply restrictions to buttons
                    if (interfaceCtx->restrictions.tradeItems != 0) {
                        if (((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_MOONS_TEAR) &&
                             (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_PENDANT_OF_MEMORIES)) ||
                            ((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_BOTTLE) &&
                             (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_OBABA_DRINK)) ||
                            (GET_CUR_FORM_BTN_ITEM(i) == ITEM_OCARINA_OF_TIME)) {
                            if (BUTTON_STATUS(i) == BTN_ENABLED) {
                                restoreHudVisibility = true;
                            }
                            BUTTON_STATUS(i) = BTN_DISABLED;
                        }
                    } else if (interfaceCtx->restrictions.tradeItems == 0) {
                        if (((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_MOONS_TEAR) &&
                             (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_PENDANT_OF_MEMORIES)) ||
                            ((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_BOTTLE) &&
                             (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_OBABA_DRINK)) ||
                            (GET_CUR_FORM_BTN_ITEM(i) == ITEM_OCARINA_OF_TIME)) {
                            if (BUTTON_STATUS(i) == BTN_DISABLED) {
                                restoreHudVisibility = true;
                            }
                            BUTTON_STATUS(i) = BTN_ENABLED;
                        }
                    }

                    if (interfaceCtx->restrictions.masks != 0) {
                        if ((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_MASK_DEKU) &&
                            (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_MASK_GIANT)) {
                            if (!BUTTON_STATUS(i)) { // == BTN_ENABLED
                                restoreHudVisibility = true;
                            }
                            BUTTON_STATUS(i) = BTN_DISABLED;
                        }
                    } else if (interfaceCtx->restrictions.masks == 0) {
                        if ((GET_CUR_FORM_BTN_ITEM(i) >= ITEM_MASK_DEKU) &&
                            (GET_CUR_FORM_BTN_ITEM(i) <= ITEM_MASK_GIANT)) {
                            if (BUTTON_STATUS(i) == BTN_DISABLED) {
                                restoreHudVisibility = true;
                            }
                            BUTTON_STATUS(i) = BTN_ENABLED;
                        }
                    }

                    if (interfaceCtx->restrictions.pictoBox != 0) {
                        if (GET_CUR_FORM_BTN_ITEM(i) == ITEM_PICTOGRAPH_BOX) {
                            if (!BUTTON_STATUS(i)) { // == BTN_ENABLED
                                restoreHudVisibility = true;
                            }
                            BUTTON_STATUS(i) = BTN_DISABLED;
                        }
                    } else if (interfaceCtx->restrictions.pictoBox == 0) {
                        if (GET_CUR_FORM_BTN_ITEM(i) == ITEM_PICTOGRAPH_BOX) {
                            if (BUTTON_STATUS(i) == BTN_DISABLED) {
                                restoreHudVisibility = true;
                            }
                            BUTTON_STATUS(i) = BTN_ENABLED;
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

                            if (BUTTON_STATUS(i) == BTN_ENABLED) {
                                restoreHudVisibility = true;
                                BUTTON_STATUS(i) = BTN_DISABLED;
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

                            if (BUTTON_STATUS(i) == BTN_DISABLED) {
                                restoreHudVisibility = true;
                                BUTTON_STATUS(i) = BTN_ENABLED;
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


/**
 * Sets the button alphas to be dimmed for disabled buttons, or to the requested alpha for non-disabled buttons
 */
// @recomp Patched to also set extra slot alpha values.
RECOMP_PATCH void Interface_UpdateButtonAlphasByStatus(PlayState* play, s16 risingAlpha) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;

    if ((gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_DISABLED) || (gSaveContext.bButtonStatus == BTN_DISABLED)) {
        if (interfaceCtx->bAlpha != 70) {
            interfaceCtx->bAlpha = 70;
        }
    } else {
        if (interfaceCtx->bAlpha != 255) {
            interfaceCtx->bAlpha = risingAlpha;
        }
    }

    if (gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT] == BTN_DISABLED) {
        if (interfaceCtx->cLeftAlpha != 70) {
            interfaceCtx->cLeftAlpha = 70;
        }
    } else {
        if (interfaceCtx->cLeftAlpha != 255) {
            interfaceCtx->cLeftAlpha = risingAlpha;
        }
    }

    if (gSaveContext.buttonStatus[EQUIP_SLOT_C_DOWN] == BTN_DISABLED) {
        if (interfaceCtx->cDownAlpha != 70) {
            interfaceCtx->cDownAlpha = 70;
        }
    } else {
        if (interfaceCtx->cDownAlpha != 255) {
            interfaceCtx->cDownAlpha = risingAlpha;
        }
    }

    if (gSaveContext.buttonStatus[EQUIP_SLOT_C_RIGHT] == BTN_DISABLED) {
        if (interfaceCtx->cRightAlpha != 70) {
            interfaceCtx->cRightAlpha = 70;
        }
    } else {
        if (interfaceCtx->cRightAlpha != 255) {
            interfaceCtx->cRightAlpha = risingAlpha;
        }
    }

    if (gSaveContext.buttonStatus[EQUIP_SLOT_A] == BTN_DISABLED) {
        if (interfaceCtx->aAlpha != 70) {
            interfaceCtx->aAlpha = 70;
        }
    } else {
        if (interfaceCtx->aAlpha != 255) {
            interfaceCtx->aAlpha = risingAlpha;
        }
    }

    for (int i = 0; i < EXTRA_ITEM_SLOT_COUNT; i++) {
        if (extra_item_slot_statuses[i] == BTN_DISABLED) {
            if (extra_item_slot_alphas[i] != 70) {
                extra_item_slot_alphas[i] = 70;
            }
        }
        else {
            if (extra_item_slot_alphas[i] != 255) {
                extra_item_slot_alphas[i] = risingAlpha;
            }
        }
    }
}

/**
 * Lower button alphas on the HUD to the requested value
 * If (gSaveContext.hudVisibilityForceButtonAlphasByStatus), then instead update button alphas
 * depending on button status
 */
// @recomp Patched to also set extra slot alpha values.
RECOMP_PATCH void Interface_UpdateButtonAlphas(PlayState* play, s16 dimmingAlpha, s16 risingAlpha) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;

    if (gSaveContext.hudVisibilityForceButtonAlphasByStatus) {
        Interface_UpdateButtonAlphasByStatus(play, risingAlpha);
        return;
    }

    if ((interfaceCtx->bAlpha != 0) && (interfaceCtx->bAlpha > dimmingAlpha)) {
        interfaceCtx->bAlpha = dimmingAlpha;
    }

    if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
        interfaceCtx->aAlpha = dimmingAlpha;
    }

    if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
        interfaceCtx->cLeftAlpha = dimmingAlpha;
    }

    if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
        interfaceCtx->cDownAlpha = dimmingAlpha;
    }

    if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
        interfaceCtx->cRightAlpha = dimmingAlpha;
    }
            
    // @recomp
    for (int i = 0; i < EXTRA_ITEM_SLOT_COUNT; i++) {
        if ((extra_item_slot_alphas[i] != 0) && (extra_item_slot_alphas[i] > dimmingAlpha)) {
            extra_item_slot_alphas[i] = dimmingAlpha;
        }
    }
}

// @recomp Patched to also set extra slot alpha values.
RECOMP_PATCH void Interface_UpdateHudAlphas(PlayState* play, s16 dimmingAlpha) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    s16 risingAlpha = 255 - dimmingAlpha;

    switch (gSaveContext.nextHudVisibility) {
        case HUD_VISIBILITY_NONE:
        case HUD_VISIBILITY_NONE_ALT:
        case HUD_VISIBILITY_B:
            if (gSaveContext.nextHudVisibility == HUD_VISIBILITY_B) {
                if (interfaceCtx->bAlpha != 255) {
                    interfaceCtx->bAlpha = risingAlpha;
                }
            } else {
                if ((interfaceCtx->bAlpha != 0) && (interfaceCtx->bAlpha > dimmingAlpha)) {
                    interfaceCtx->bAlpha = dimmingAlpha;
                }
            }

            if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
                interfaceCtx->aAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
                interfaceCtx->cLeftAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
                interfaceCtx->cDownAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
                interfaceCtx->cRightAlpha = dimmingAlpha;
            }
            
            // @recomp
            for (int i = 0; i < EXTRA_ITEM_SLOT_COUNT; i++) {
                if ((extra_item_slot_alphas[i] != 0) && (extra_item_slot_alphas[i] > dimmingAlpha)) {
                    extra_item_slot_alphas[i] = dimmingAlpha;
                }
            }

            if ((interfaceCtx->healthAlpha != 0) && (interfaceCtx->healthAlpha > dimmingAlpha)) {
                interfaceCtx->healthAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->magicAlpha != 0) && (interfaceCtx->magicAlpha > dimmingAlpha)) {
                interfaceCtx->magicAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            break;

        case HUD_VISIBILITY_HEARTS_WITH_OVERWRITE:
            // aAlpha is immediately overwritten in Interface_UpdateButtonAlphas
            if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
                interfaceCtx->aAlpha = dimmingAlpha;
            }

            Interface_UpdateButtonAlphas(play, dimmingAlpha, risingAlpha + 0);

            if ((interfaceCtx->magicAlpha != 0) && (interfaceCtx->magicAlpha > dimmingAlpha)) {
                interfaceCtx->magicAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_A:
            if ((interfaceCtx->bAlpha != 0) && (interfaceCtx->bAlpha > dimmingAlpha)) {
                interfaceCtx->bAlpha = dimmingAlpha;
            }

            // aAlpha is immediately overwritten below
            if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
                interfaceCtx->aAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
                interfaceCtx->cLeftAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
                interfaceCtx->cDownAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
                interfaceCtx->cRightAlpha = dimmingAlpha;
            }
            
            // @recomp
            for (int i = 0; i < EXTRA_ITEM_SLOT_COUNT; i++) {
                if ((extra_item_slot_alphas[i] != 0) && (extra_item_slot_alphas[i] > dimmingAlpha)) {
                    extra_item_slot_alphas[i] = dimmingAlpha;
                }
            }

            if ((interfaceCtx->healthAlpha != 0) && (interfaceCtx->healthAlpha > dimmingAlpha)) {
                interfaceCtx->healthAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->magicAlpha != 0) && (interfaceCtx->magicAlpha > dimmingAlpha)) {
                interfaceCtx->magicAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            if (interfaceCtx->aAlpha != 255) {
                interfaceCtx->aAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_A_HEARTS_MAGIC_WITH_OVERWRITE:
            Interface_UpdateButtonAlphas(play, dimmingAlpha, risingAlpha);

            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            // aAlpha overwrites the value set in Interface_UpdateButtonAlphas
            if (interfaceCtx->aAlpha != 255) {
                interfaceCtx->aAlpha = risingAlpha;
            }

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            if (interfaceCtx->magicAlpha != 255) {
                interfaceCtx->magicAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_A_HEARTS_MAGIC_MINIMAP_WITH_OVERWRITE:
            Interface_UpdateButtonAlphas(play, dimmingAlpha, risingAlpha);

            // aAlpha overwrites the value set in Interface_UpdateButtonAlphas
            if (interfaceCtx->aAlpha != 255) {
                interfaceCtx->aAlpha = risingAlpha;
            }

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            if (interfaceCtx->magicAlpha != 255) {
                interfaceCtx->magicAlpha = risingAlpha;
            }

            if (play->sceneId == SCENE_SPOT00) {
                if (interfaceCtx->minimapAlpha < 170) {
                    interfaceCtx->minimapAlpha = risingAlpha;
                } else {
                    interfaceCtx->minimapAlpha = 170;
                }
            } else if (interfaceCtx->minimapAlpha != 255) {
                interfaceCtx->minimapAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_ALL_NO_MINIMAP_W_DISABLED:
            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            Interface_UpdateButtonAlphasByStatus(play, risingAlpha);

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            if (interfaceCtx->magicAlpha != 255) {
                interfaceCtx->magicAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_HEARTS_MAGIC:
            if ((interfaceCtx->bAlpha != 0) && (interfaceCtx->bAlpha > dimmingAlpha)) {
                interfaceCtx->bAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
                interfaceCtx->aAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
                interfaceCtx->cLeftAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
                interfaceCtx->cDownAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
                interfaceCtx->cRightAlpha = dimmingAlpha;
            }
            
            // @recomp
            for (int i = 0; i < EXTRA_ITEM_SLOT_COUNT; i++) {
                if ((extra_item_slot_alphas[i] != 0) && (extra_item_slot_alphas[i] > dimmingAlpha)) {
                    extra_item_slot_alphas[i] = dimmingAlpha;
                }
            }

            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            if (interfaceCtx->magicAlpha != 255) {
                interfaceCtx->magicAlpha = risingAlpha;
            }

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_B_ALT:
            if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
                interfaceCtx->aAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
                interfaceCtx->cLeftAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
                interfaceCtx->cDownAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
                interfaceCtx->cRightAlpha = dimmingAlpha;
            }
            
            // @recomp
            for (int i = 0; i < EXTRA_ITEM_SLOT_COUNT; i++) {
                if ((extra_item_slot_alphas[i] != 0) && (extra_item_slot_alphas[i] > dimmingAlpha)) {
                    extra_item_slot_alphas[i] = dimmingAlpha;
                }
            }

            if ((interfaceCtx->healthAlpha != 0) && (interfaceCtx->healthAlpha > dimmingAlpha)) {
                interfaceCtx->healthAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->magicAlpha != 0) && (interfaceCtx->magicAlpha > dimmingAlpha)) {
                interfaceCtx->magicAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            if (interfaceCtx->bAlpha != 255) {
                interfaceCtx->bAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_HEARTS:
            if ((interfaceCtx->bAlpha != 0) && (interfaceCtx->bAlpha > dimmingAlpha)) {
                interfaceCtx->bAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
                interfaceCtx->aAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
                interfaceCtx->cLeftAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
                interfaceCtx->cDownAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
                interfaceCtx->cRightAlpha = dimmingAlpha;
            }
            
            // @recomp
            for (int i = 0; i < EXTRA_ITEM_SLOT_COUNT; i++) {
                if ((extra_item_slot_alphas[i] != 0) && (extra_item_slot_alphas[i] > dimmingAlpha)) {
                    extra_item_slot_alphas[i] = dimmingAlpha;
                }
            }

            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->magicAlpha != 0) && (interfaceCtx->magicAlpha > dimmingAlpha)) {
                interfaceCtx->magicAlpha = dimmingAlpha;
            }

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_A_B_MINIMAP:
            if (interfaceCtx->aAlpha != 255) {
                interfaceCtx->aAlpha = risingAlpha;
            }

            if ((gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_DISABLED) ||
                (gSaveContext.bButtonStatus == ITEM_NONE)) {
                if (interfaceCtx->bAlpha != 70) {
                    interfaceCtx->bAlpha = 70;
                }
            } else {
                if (interfaceCtx->bAlpha != 255) {
                    interfaceCtx->bAlpha = risingAlpha;
                }
            }

            if (interfaceCtx->minimapAlpha != 255) {
                interfaceCtx->minimapAlpha = risingAlpha;
            }

            if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
                interfaceCtx->cLeftAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
                interfaceCtx->cDownAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
                interfaceCtx->cRightAlpha = dimmingAlpha;
            }
            
            // @recomp
            for (int i = 0; i < EXTRA_ITEM_SLOT_COUNT; i++) {
                if ((extra_item_slot_alphas[i] != 0) && (extra_item_slot_alphas[i] > dimmingAlpha)) {
                    extra_item_slot_alphas[i] = dimmingAlpha;
                }
            }

            if ((interfaceCtx->magicAlpha != 0) && (interfaceCtx->magicAlpha > dimmingAlpha)) {
                interfaceCtx->magicAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->healthAlpha != 0) && (interfaceCtx->healthAlpha > dimmingAlpha)) {
                interfaceCtx->healthAlpha = dimmingAlpha;
            }

            break;

        case HUD_VISIBILITY_HEARTS_MAGIC_WITH_OVERWRITE:
            Interface_UpdateButtonAlphas(play, dimmingAlpha, risingAlpha);

            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            // aAlpha overwrites the value set in Interface_UpdateButtonAlphas
            if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
                interfaceCtx->aAlpha = dimmingAlpha;
            }

            if (interfaceCtx->magicAlpha != 255) {
                interfaceCtx->magicAlpha = risingAlpha;
            }

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_HEARTS_MAGIC_C:
            if ((interfaceCtx->bAlpha != 0) && (interfaceCtx->bAlpha > dimmingAlpha)) {
                interfaceCtx->bAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
                interfaceCtx->aAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            if (interfaceCtx->cLeftAlpha != 255) {
                interfaceCtx->cLeftAlpha = risingAlpha;
            }

            if (interfaceCtx->cDownAlpha != 255) {
                interfaceCtx->cDownAlpha = risingAlpha;
            }

            if (interfaceCtx->cRightAlpha != 255) {
                interfaceCtx->cRightAlpha = risingAlpha;
            }
            
            // @recomp
            for (int i = 0; i < EXTRA_ITEM_SLOT_COUNT; i++) {
                if (extra_item_slot_alphas[i] != 255) {
                    extra_item_slot_alphas[i] = risingAlpha;
                }
            }

            if (interfaceCtx->magicAlpha != 255) {
                interfaceCtx->magicAlpha = risingAlpha;
            }

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_ALL_NO_MINIMAP:
            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            if (interfaceCtx->bAlpha != 255) {
                interfaceCtx->bAlpha = risingAlpha;
            }

            if (interfaceCtx->aAlpha != 255) {
                interfaceCtx->aAlpha = risingAlpha;
            }

            if (interfaceCtx->cLeftAlpha != 255) {
                interfaceCtx->cLeftAlpha = risingAlpha;
            }

            if (interfaceCtx->cDownAlpha != 255) {
                interfaceCtx->cDownAlpha = risingAlpha;
            }

            if (interfaceCtx->cRightAlpha != 255) {
                interfaceCtx->cRightAlpha = risingAlpha;
            }
            
            // @recomp
            for (int i = 0; i < EXTRA_ITEM_SLOT_COUNT; i++) {
                if (extra_item_slot_alphas[i] != 255) {
                    extra_item_slot_alphas[i] = risingAlpha;
                }
            }

            if (interfaceCtx->magicAlpha != 255) {
                interfaceCtx->magicAlpha = risingAlpha;
            }

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_A_B_C:
            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->magicAlpha != 0) && (interfaceCtx->magicAlpha > dimmingAlpha)) {
                interfaceCtx->magicAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->healthAlpha != 0) && (interfaceCtx->healthAlpha > dimmingAlpha)) {
                interfaceCtx->healthAlpha = dimmingAlpha;
            }

            if (interfaceCtx->bAlpha != 255) {
                interfaceCtx->bAlpha = risingAlpha;
            }

            if (interfaceCtx->aAlpha != 255) {
                interfaceCtx->aAlpha = risingAlpha;
            }

            if (interfaceCtx->cLeftAlpha != 255) {
                interfaceCtx->cLeftAlpha = risingAlpha;
            }

            if (interfaceCtx->cDownAlpha != 255) {
                interfaceCtx->cDownAlpha = risingAlpha;
            }

            if (interfaceCtx->cRightAlpha != 255) {
                interfaceCtx->cRightAlpha = risingAlpha;
            }
            
            // @recomp
            for (int i = 0; i < EXTRA_ITEM_SLOT_COUNT; i++) {
                if (extra_item_slot_alphas[i] != 255) {
                    extra_item_slot_alphas[i] = risingAlpha;
                }
            }

            break;

        case HUD_VISIBILITY_B_MINIMAP:
            if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
                interfaceCtx->aAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
                interfaceCtx->cLeftAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
                interfaceCtx->cDownAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
                interfaceCtx->cRightAlpha = dimmingAlpha;
            }
            
            // @recomp
            for (int i = 0; i < EXTRA_ITEM_SLOT_COUNT; i++) {
                if ((extra_item_slot_alphas[i] != 0) && (extra_item_slot_alphas[i] > dimmingAlpha)) {
                    extra_item_slot_alphas[i] = dimmingAlpha;
                }
            }

            if ((interfaceCtx->magicAlpha != 0) && (interfaceCtx->magicAlpha > dimmingAlpha)) {
                interfaceCtx->magicAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->healthAlpha != 0) && (interfaceCtx->healthAlpha > dimmingAlpha)) {
                interfaceCtx->healthAlpha = dimmingAlpha;
            }

            if (interfaceCtx->bAlpha != 255) {
                interfaceCtx->bAlpha = risingAlpha;
            }

            if (interfaceCtx->minimapAlpha != 255) {
                interfaceCtx->minimapAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_HEARTS_MAGIC_MINIMAP:
            if ((interfaceCtx->bAlpha != 0) && (interfaceCtx->bAlpha > dimmingAlpha)) {
                interfaceCtx->bAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
                interfaceCtx->aAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
                interfaceCtx->cLeftAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
                interfaceCtx->cDownAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
                interfaceCtx->cRightAlpha = dimmingAlpha;
            }
            
            // @recomp
            for (int i = 0; i < EXTRA_ITEM_SLOT_COUNT; i++) {
                if ((extra_item_slot_alphas[i] != 0) && (extra_item_slot_alphas[i] > dimmingAlpha)) {
                    extra_item_slot_alphas[i] = dimmingAlpha;
                }
            }

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            if (interfaceCtx->magicAlpha != 255) {
                interfaceCtx->magicAlpha = risingAlpha;
            }

            if (interfaceCtx->minimapAlpha != 255) {
                interfaceCtx->minimapAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_A_HEARTS_MAGIC_MINIMAP:
            if ((interfaceCtx->bAlpha != 0) && (interfaceCtx->bAlpha > dimmingAlpha)) {
                interfaceCtx->bAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
                interfaceCtx->cLeftAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
                interfaceCtx->cDownAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
                interfaceCtx->cRightAlpha = dimmingAlpha;
            }
            
            // @recomp
            for (int i = 0; i < EXTRA_ITEM_SLOT_COUNT; i++) {
                if ((extra_item_slot_alphas[i] != 0) && (extra_item_slot_alphas[i] > dimmingAlpha)) {
                    extra_item_slot_alphas[i] = dimmingAlpha;
                }
            }

            if (interfaceCtx->aAlpha != 255) {
                interfaceCtx->aAlpha = risingAlpha;
            }

            if (interfaceCtx->minimapAlpha != 255) {
                interfaceCtx->minimapAlpha = risingAlpha;
            }

            if (interfaceCtx->magicAlpha != 255) {
                interfaceCtx->magicAlpha = risingAlpha;
            }

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_B_MAGIC:
            if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
                interfaceCtx->aAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
                interfaceCtx->cLeftAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
                interfaceCtx->cDownAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
                interfaceCtx->cRightAlpha = dimmingAlpha;
            }
            
            // @recomp
            for (int i = 0; i < EXTRA_ITEM_SLOT_COUNT; i++) {
                if ((extra_item_slot_alphas[i] != 0) && (extra_item_slot_alphas[i] > dimmingAlpha)) {
                    extra_item_slot_alphas[i] = dimmingAlpha;
                }
            }

            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->healthAlpha != 0) && (interfaceCtx->healthAlpha > dimmingAlpha)) {
                interfaceCtx->healthAlpha = dimmingAlpha;
            }

            if (interfaceCtx->bAlpha != 255) {
                interfaceCtx->bAlpha = risingAlpha;
            }

            if (interfaceCtx->magicAlpha != 255) {
                interfaceCtx->magicAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_A_B:
            if (interfaceCtx->aAlpha != 255) {
                interfaceCtx->aAlpha = risingAlpha;
            }

            if (interfaceCtx->bAlpha != 255) {
                interfaceCtx->bAlpha = risingAlpha;
            }

            if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
                interfaceCtx->cLeftAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
                interfaceCtx->cDownAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
                interfaceCtx->cRightAlpha = dimmingAlpha;
            }
            
            // @recomp
            for (int i = 0; i < EXTRA_ITEM_SLOT_COUNT; i++) {
                if ((extra_item_slot_alphas[i] != 0) && (extra_item_slot_alphas[i] > dimmingAlpha)) {
                    extra_item_slot_alphas[i] = dimmingAlpha;
                }
            }

            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->magicAlpha != 0) && (interfaceCtx->magicAlpha > dimmingAlpha)) {
                interfaceCtx->magicAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->healthAlpha != 0) && (interfaceCtx->healthAlpha > dimmingAlpha)) {
                interfaceCtx->healthAlpha = dimmingAlpha;
            }

            break;

        case HUD_VISIBILITY_A_B_HEARTS_MAGIC_MINIMAP:
            if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
                interfaceCtx->cLeftAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
                interfaceCtx->cDownAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
                interfaceCtx->cRightAlpha = dimmingAlpha;
            }
            
            // @recomp
            for (int i = 0; i < EXTRA_ITEM_SLOT_COUNT; i++) {
                if ((extra_item_slot_alphas[i] != 0) && (extra_item_slot_alphas[i] > dimmingAlpha)) {
                    extra_item_slot_alphas[i] = dimmingAlpha;
                }
            }

            if (interfaceCtx->bAlpha != 255) {
                interfaceCtx->bAlpha = risingAlpha;
            }

            if (interfaceCtx->aAlpha != 255) {
                interfaceCtx->aAlpha = risingAlpha;
            }

            if (interfaceCtx->minimapAlpha != 255) {
                interfaceCtx->minimapAlpha = risingAlpha;
            }

            if (interfaceCtx->magicAlpha != 255) {
                interfaceCtx->magicAlpha = risingAlpha;
            }

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            break;
    }

    if ((play->roomCtx.curRoom.behaviorType1 == ROOM_BEHAVIOR_TYPE1_1) && (interfaceCtx->minimapAlpha >= 255)) {
        interfaceCtx->minimapAlpha = 255;
    }
}

INCBIN(dpad_icon, "dpad.rgba32.bin");

#define DPAD_W 18
#define DPAD_H 18

#define DPAD_IMG_W 32
#define DPAD_IMG_H 32

#define DPAD_DSDX (s32)(1024.0f * (float)(DPAD_IMG_W) / (DPAD_W))
#define DPAD_DTDY (s32)(1024.0f * (float)(DPAD_IMG_H) / (DPAD_H))

#define DPAD_CENTER_X 40
#define DPAD_CENTER_Y 84

#define ICON_IMG_SIZE 32
#define ICON_SIZE 16
#define ICON_DIST 14

#define ICON_DSDX (s32)(1024.0f * (float)(ICON_IMG_SIZE) / (ICON_SIZE))
#define ICON_DTDY (s32)(1024.0f * (float)(ICON_IMG_SIZE) / (ICON_SIZE))

s32 dpad_item_icon_positions[4][2] = {
    {           0, -ICON_DIST},
    { -ICON_DIST, 0          },
    {  ICON_DIST, 0          },
    {           0, ICON_DIST - 2 }
};

Gfx* Gfx_DrawRect_DropShadow(Gfx* gfx, s16 rectLeft, s16 rectTop, s16 rectWidth, s16 rectHeight, u16 dsdx, u16 dtdy,
                             s16 r, s16 g, s16 b, s16 a);

void draw_dpad(PlayState* play) {
    OPEN_DISPS(play->state.gfxCtx);

    gEXForceUpscale2D(OVERLAY_DISP++, 1);
    gDPLoadTextureBlock(OVERLAY_DISP++, dpad_icon, G_IM_FMT_RGBA, G_IM_SIZ_32b, DPAD_IMG_W, DPAD_IMG_H, 0,
        G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

    // Determine the maximum alpha of all the D-Pad items and use that as the alpha of the D-Pad itself.
    int alpha = 0;
    for (int i = 0; i < 4; i++) {
        int cur_alpha = extra_item_slot_alphas[i];
        alpha = MAX(alpha, cur_alpha);
    }

    // Check if none of the D-Pad items have been obtained and clamp the alpha to 70 if so.
    bool item_obtained = false;
    for (int i = 0; i < 4; i++) {
        s32 item = extra_button_items[0][i];
        if ((item != ITEM_NONE) && (INV_CONTENT(item) == item)) {
            item_obtained = true;
            break;
        }
    }

    if (!item_obtained) {
        alpha = MIN(alpha, 70);
    }

    gDPSetCombineMode(OVERLAY_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    OVERLAY_DISP = Gfx_DrawRect_DropShadow(OVERLAY_DISP, DPAD_CENTER_X - (DPAD_W/2), DPAD_CENTER_Y - (DPAD_W/2), DPAD_W, DPAD_H,
        DPAD_DSDX, DPAD_DTDY,
        255, 255, 255, alpha);
    gEXForceUpscale2D(OVERLAY_DISP++, 0);

    CLOSE_DISPS(play->state.gfxCtx);
}

bool dpad_item_icons_loaded = false;
u8 dpad_item_textures[4][ICON_IMG_SIZE * ICON_IMG_SIZE * 4] __attribute__((aligned(8)));

void draw_dpad_icons(PlayState* play) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;

    if (!dpad_item_icons_loaded) {
        for (int i = 0; i < 4; i++) {
            CmpDma_LoadFile(SEGMENT_ROM_START(icon_item_static_yar), extra_button_items[0][i], dpad_item_textures[i], sizeof(dpad_item_textures[i]));
        }

        dpad_item_icons_loaded = true;
    }

    OPEN_DISPS(play->state.gfxCtx);
    

    gEXForceUpscale2D(OVERLAY_DISP++, 1);
    gDPLoadTextureBlock(OVERLAY_DISP++, dpad_icon, G_IM_FMT_RGBA, G_IM_SIZ_32b, DPAD_IMG_W, DPAD_IMG_H, 0,
        G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        
    gDPSetCombineMode(OVERLAY_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);

    for (int i = 0; i < 4; i++) {
        s32 item = extra_button_items[0][i];
        if ((item != ITEM_NONE) && (INV_CONTENT(item) == item)) {
            gDPLoadTextureBlock(OVERLAY_DISP++, dpad_item_textures[i], G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0, G_TX_NOMIRROR | G_TX_WRAP,
                                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, extra_item_slot_alphas[i]);
            gSPTextureRectangle(OVERLAY_DISP++,
                (dpad_item_icon_positions[i][0] + DPAD_CENTER_X - (ICON_SIZE/2)) * 4, (dpad_item_icon_positions[i][1] + DPAD_CENTER_Y - (ICON_SIZE/2)) * 4,
                (dpad_item_icon_positions[i][0] + DPAD_CENTER_X + (ICON_SIZE/2)) * 4, (dpad_item_icon_positions[i][1] + DPAD_CENTER_Y + (ICON_SIZE/2)) * 4,
                0,
                0, 0,
                ICON_DSDX, ICON_DTDY);
        }
    }

    gEXForceUpscale2D(OVERLAY_DISP++, 0);

    CLOSE_DISPS(play->state.gfxCtx);
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
