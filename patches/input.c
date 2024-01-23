#include "patches.h"
#include "input.h"


s32 func_80847190(PlayState* play, Player* this, s32 arg2);
s16 func_80832754(Player* this, s32 arg1);
s32 func_8082EF20(Player* this);

// Patched to add gyro aiming
s32 func_80847190(PlayState* play, Player* this, s32 arg2) {
    s32 pad;
    s16 var_s0;

    if (!func_800B7128(this) && !func_8082EF20(this) && !arg2) {
        var_s0 = play->state.input[0].rel.stick_y * 0xF0;
        Math_SmoothStepToS(&this->actor.focus.rot.x, var_s0, 0xE, 0xFA0, 0x1E);

        var_s0 = play->state.input[0].rel.stick_x * -0x10;
        var_s0 = CLAMP(var_s0, -0xBB8, 0xBB8);
        this->actor.focus.rot.y += var_s0;
    }
    else {
        float gyro_x, gyro_y;
        recomp_get_gyro_deltas(&gyro_x, &gyro_y);

        s16 temp3;

        temp3 = ((play->state.input[0].rel.stick_y >= 0) ? 1 : -1) *
            (s32)((1.0f - Math_CosS(play->state.input[0].rel.stick_y * 0xC8)) * 1500.0f);
        this->actor.focus.rot.x += temp3 + (s32)(gyro_x * -3.0f);

        if (this->stateFlags1 & PLAYER_STATE1_800000) {
            this->actor.focus.rot.x = CLAMP(this->actor.focus.rot.x, -0x1F40, 0xFA0);
        }
        else {
            this->actor.focus.rot.x = CLAMP(this->actor.focus.rot.x, -0x36B0, 0x36B0);
        }

        var_s0 = this->actor.focus.rot.y - this->actor.shape.rot.y;
        temp3 = ((play->state.input[0].rel.stick_x >= 0) ? 1 : -1) *
            (s32)((1.0f - Math_CosS(play->state.input[0].rel.stick_x * 0xC8)) * -1500.0f);
        var_s0 += temp3 + (s32)(gyro_y * 3.0f);

        this->actor.focus.rot.y = CLAMP(var_s0, -0x4AAA, 0x4AAA) + this->actor.shape.rot.y;
    }

    this->unk_AA6 |= 2;

    return func_80832754(this, (play->unk_1887C != 0) || func_800B7128(this) || func_8082EF20(this));
}

#if 0
u32 sPlayerItemButtons[] = {
    BTN_B,
    BTN_CLEFT,
    BTN_CDOWN,
    BTN_CRIGHT,
};

u32 sPlayerItemButtonsDualAnalog[] = {
    BTN_B,
    BTN_DLEFT,
    BTN_DDOWN,
    BTN_DRIGHT
};

u32 prev_item_buttons = 0;
u32 cur_item_buttons = 0;
u32 pressed_item_buttons = 0;
u32 released_item_buttons = 0;

typedef enum {
    EQUIP_SLOT_EX_DEKU = -2,
    EQUIP_SLOT_EX_GORON = -3,
    EQUIP_SLOT_EX_ZORA = -4,
    EQUIP_SLOT_EX_OCARINA = -5
} EquipSlotEx;

static inline void dup_to_cup(u16* button) {
    if (*button & BTN_DUP) {
        *button |= BTN_CUP; 
    }
}

void GameState_GetInput(GameState* gameState) {
    PadMgr_GetInput(gameState->input, true);

    if (recomp_camera_mode == RECOMP_CAMERA_DUALANALOG) {
        gameState->input[0].cur.button &= ~BTN_CUP;
        gameState->input[0].press.button &= ~BTN_CUP;
        gameState->input[0].rel.button &= ~BTN_CUP;
        dup_to_cup(&gameState->input[0].cur.button);
        dup_to_cup(&gameState->input[0].press.button);
        dup_to_cup(&gameState->input[0].rel.button);
    }

    prev_item_buttons = cur_item_buttons;
    recomp_get_item_inputs(&cur_item_buttons);
    u32 button_diff = prev_item_buttons ^ cur_item_buttons;
    pressed_item_buttons = cur_item_buttons & button_diff;
    released_item_buttons = prev_item_buttons & button_diff;
}

struct SlotMap {
    u32 button;
    EquipSlotEx slot;
};

struct SlotMap exSlotMapping[] = {
    {BTN_DLEFT,  EQUIP_SLOT_EX_GORON},
    {BTN_DRIGHT, EQUIP_SLOT_EX_ZORA},
    {BTN_DUP,    EQUIP_SLOT_EX_DEKU},
    {BTN_DDOWN,  EQUIP_SLOT_EX_OCARINA},
};

// D-Pad items
// TODO restore this once UI is made
// Return currently-pressed button, in order of priority D-Pad, B, CLEFT, CDOWN, CRIGHT.

EquipSlot func_8082FDC4(void) {
    EquipSlot i;

    // for (int mapping_index = 0; mapping_index < ARRAY_COUNT(exSlotMapping); mapping_index++) {
    //     if (pressed_item_buttons & exSlotMapping[mapping_index].button) {
    //         return (EquipSlot)exSlotMapping[mapping_index].slot;
    //     }
    // }

    u32* button_map = recomp_camera_mode == RECOMP_CAMERA_DUALANALOG ? sPlayerItemButtonsDualAnalog : sPlayerItemButtons;

    for (i = 0; i < ARRAY_COUNT(sPlayerItemButtons); i++) {
        if (CHECK_BTN_ALL(pressed_item_buttons, button_map[i])) {
            break;
        }
    }

    return i;
}
/*
ItemId Player_GetItemOnButton(PlayState* play, Player* player, EquipSlot slot) {
    if (slot >= EQUIP_SLOT_A) {
        return ITEM_NONE;
    }

    if (slot <= EQUIP_SLOT_EX_DEKU) {
        ItemId dpad_item = ITEM_NONE;
        switch ((EquipSlotEx)slot) {
            case EQUIP_SLOT_EX_DEKU:
                dpad_item = ITEM_MASK_DEKU;
                break;
            case EQUIP_SLOT_EX_GORON:
                dpad_item = ITEM_MASK_GORON;
                break;
            case EQUIP_SLOT_EX_ZORA:
                dpad_item = ITEM_MASK_ZORA;
                break;
            case EQUIP_SLOT_EX_OCARINA:
                dpad_item = ITEM_OCARINA_OF_TIME;
                break;
        }

        if ((dpad_item != ITEM_NONE) && (INV_CONTENT(dpad_item) == dpad_item)) {
            recomp_printf("Used dpad item %d\n", dpad_item);
            return dpad_item;
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
*/
#endif // #if 0
