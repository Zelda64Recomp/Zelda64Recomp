#include "patches.h"
#include "input.h"
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
