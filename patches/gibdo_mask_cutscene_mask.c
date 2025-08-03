#include "patches.h"
#include "transform_ids.h"
#include "overlays/actors/ovl_Dm_Char05/z_dm_char05.h"

extern void func_80AADF54(PlayState* play, DmChar05* this);

// @recomp Patched to avoid an interpolation glitch in Pamela's dad's cutscene
// that happens when the mask is meant to teleport offscreen.
RECOMP_PATCH void func_80AADB4C(Actor* thisx, PlayState* play) {
    DmChar05* this = (DmChar05*)thisx;
    if (this->unk_18E == 0) {
        if (Cutscene_IsCueInChannel(play, CS_CMD_ACTOR_CUE_518) &&
            (play->csCtx.actorCues[Cutscene_GetCueChannel(play, CS_CMD_ACTOR_CUE_518)]->id != 1)) {
            // @recomp During this cue the mask does nothing other than teleport offscreen and stay still,
            // so we can just skip interpolation the entire time.
            if (play->csCtx.actorCues[Cutscene_GetCueChannel(play, CS_CMD_ACTOR_CUE_518)]->id == 3) {
                actor_set_interpolation_skipped(thisx);
            }
            Gfx_SetupDL25_Opa(play->state.gfxCtx);
            SkelAnime_DrawFlexOpa(play, this->skelAnime.skeleton, this->skelAnime.jointTable,
                                  this->skelAnime.dListCount, NULL, NULL, &this->actor);
        }
    } else if (this->unk_18E == 1) {
        func_80AADF54(play, this);
    }
}
