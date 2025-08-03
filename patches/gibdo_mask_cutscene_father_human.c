#include "patches.h"
#include "transform_ids.h"
#include "overlays/actors/ovl_En_Hgo/z_en_hgo.h"

typedef enum {
    /* 0 */ HGO_ANIM_ARMS_FOLDED,
    /* 1 */ HGO_ANIM_ASTONISHED,
    /* 2 */ HGO_ANIM_KNEEL_DOWN_AND_HUG,
    /* 3 */ HGO_ANIM_CONSOLE,
    /* 4 */ HGO_ANIM_CONSOLE_HEAD_UP,
    /* 5 */ HGO_ANIM_REACH_DOWN_TO_LIFT,
    /* 6 */ HGO_ANIM_TOSS,
    /* 7 */ HGO_ANIM_MAX
} HgoAnimation;

extern AnimationInfo sPamelasFatherHumanAnimationInfo[];
extern void EnHgo_Draw(Actor* thisx, PlayState* play);
extern void EnHgo_DoNothing(EnHgo* this, PlayState* play);
extern void EnHgo_SetupInitCollision(EnHgo* this);

// @recomp Skip interpolation when the animations change during the cutscene, as the
// animation changes are meant to happen at the same time as the camera cuts.
RECOMP_PATCH s32 EnHgo_HandleCsAction(EnHgo* this, PlayState* play) {
    s32 cueChannel;

    if (Cutscene_IsCueInChannel(play, CS_CMD_ACTOR_CUE_486)) {
        cueChannel = Cutscene_GetCueChannel(play, CS_CMD_ACTOR_CUE_486);
        if (this->cueId != play->csCtx.actorCues[cueChannel]->id) {
            this->cueId = play->csCtx.actorCues[cueChannel]->id;
            switch (play->csCtx.actorCues[cueChannel]->id) {
                case 1:
                    this->animIndex = HGO_ANIM_ARMS_FOLDED;
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sPamelasFatherHumanAnimationInfo, HGO_ANIM_ARMS_FOLDED);
                    break;

                case 2:
                    this->actor.draw = EnHgo_Draw;
                    this->animIndex = HGO_ANIM_ASTONISHED;
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sPamelasFatherHumanAnimationInfo, HGO_ANIM_ASTONISHED);
                    break;

                case 3:
                    this->animIndex = HGO_ANIM_KNEEL_DOWN_AND_HUG;
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sPamelasFatherHumanAnimationInfo, HGO_ANIM_KNEEL_DOWN_AND_HUG);
                    break;

                case 4:
                    this->animIndex = HGO_ANIM_CONSOLE;
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sPamelasFatherHumanAnimationInfo, HGO_ANIM_CONSOLE);
                    break;

                case 5:
                    this->animIndex = HGO_ANIM_CONSOLE_HEAD_UP;
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sPamelasFatherHumanAnimationInfo, HGO_ANIM_CONSOLE_HEAD_UP);
                    break;

                case 6:
                    this->animIndex = HGO_ANIM_REACH_DOWN_TO_LIFT;
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sPamelasFatherHumanAnimationInfo, HGO_ANIM_REACH_DOWN_TO_LIFT);
                    break;

                default:
                    break;
            }
            actor_set_interpolation_skipped(&this->actor);
        } else if (Animation_OnFrame(&this->skelAnime, this->skelAnime.endFrame)) {
            switch (this->animIndex) {
                case HGO_ANIM_ASTONISHED:
                    if (Animation_OnFrame(&this->skelAnime, this->skelAnime.endFrame) && !this->isInCutscene) {
                        this->isInCutscene = true;
                        if ((gSaveContext.sceneLayer == 0) &&
                            ((play->csCtx.scriptIndex == 2) || (play->csCtx.scriptIndex == 4))) {
                            Actor_PlaySfx(&this->actor, NA_SE_VO_GBVO02);
                        }
                    }
                    break;

                case HGO_ANIM_KNEEL_DOWN_AND_HUG:
                    this->animIndex = HGO_ANIM_CONSOLE;
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sPamelasFatherHumanAnimationInfo, HGO_ANIM_CONSOLE);
                    break;

                case HGO_ANIM_REACH_DOWN_TO_LIFT:
                    this->animIndex = HGO_ANIM_TOSS;
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sPamelasFatherHumanAnimationInfo, HGO_ANIM_TOSS);

                default:
                    break;
                
            }
        }

        Cutscene_ActorTranslateAndYaw(&this->actor, play, cueChannel);
        return true;
    }

    if ((play->csCtx.state == CS_STATE_IDLE) && CHECK_WEEKEVENTREG(WEEKEVENTREG_75_20) &&
        (this->actionFunc == EnHgo_DoNothing)) {
        this->actor.shape.rot.y = this->actor.world.rot.y;
        Actor_Spawn(&play->actorCtx, play, ACTOR_ELF_MSG2, this->actor.focus.pos.x, this->actor.focus.pos.y,
                    this->actor.focus.pos.z, 7, 0, 0, 0x7F5A);
        EnHgo_SetupInitCollision(this);
    }

    this->cueId = 99;
    return false;
}
