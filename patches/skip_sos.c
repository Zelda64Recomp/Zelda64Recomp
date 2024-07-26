#include "patches.h"

#include "overlays/actors/ovl_En_Test7/z_en_test7.h"

#define THIS ((EnTest7*)thisx)

void func_80AF118C(PlayState* play, OwlWarpFeather* feathers, EnTest7* this, s32 arg3, s32 arg4); // EnTest7_UpdateFeathers
void func_80AF2350(EnTest7* this, PlayState* play); // EnTest7_WarpCsWarp

RECOMP_PATCH void EnTest7_Update(Actor* thisx, PlayState* play) {
    EnTest7* this = THIS;

    this->actionFunc(this, play);

    if (this->playerCamFunc != NULL) {
        this->playerCamFunc(this, play);
    }

    this->timer++;

    func_80AF118C(play, this->feathers, this, (this->flags & OWL_WARP_FLAGS_8) != 0,
                           (this->flags & OWL_WARP_FLAGS_10) != 0);

    // @recomp Allow skipping the Song of Soaring cutscene.
    Input* input = CONTROLLER1(&play->state);
    if (CHECK_BTN_ANY(input->press.button, BTN_A | BTN_B) && (OWL_WARP_CS_GET_OCARINA_MODE(&this->actor) != ENTEST7_ARRIVE)) {
        func_80AF2350(thisx, play);
    }
}