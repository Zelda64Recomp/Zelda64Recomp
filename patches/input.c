#include "patches.h"
#include "input.h"

u32 sPlayerItemButtons[] = {
    BTN_B,
    BTN_CLEFT,
    BTN_CDOWN,
    BTN_CRIGHT,
};

// Return currently-pressed button, in order of priority B, CLEFT, CDOWN, CRIGHT.
EquipSlot func_8082FDC4(void) {
    EquipSlot i;
    RecompInputs cur_inputs;
    recomp_get_item_inputs(&cur_inputs);

    for (i = 0; i < ARRAY_COUNT(sPlayerItemButtons); i++) {
        if (CHECK_BTN_ALL(cur_inputs.buttons, sPlayerItemButtons[i])) {
            break;
        }
    }

    return i;
}
