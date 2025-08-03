#include "patches.h"
#include "transform_ids.h"
#include "overlays/actors/ovl_En_Hg/z_en_hg.h"

extern AnimationInfo sPamelasFatherGibdoAnimationInfo[];
extern void EnHg_SetupWait(EnHg* this);

typedef enum {
    /* 0 */ HG_ANIM_IDLE,
    /* 1 */ HG_ANIM_LURCH_FORWARD,
    /* 2 */ HG_ANIM_RECOIL,
    /* 3 */ HG_ANIM_LEAN_FORWARD,
    /* 4 */ HG_ANIM_REACH_FORWARD,
    /* 5 */ HG_ANIM_CURL_UP,
    /* 6 */ HG_ANIM_CROUCHED_PANIC,
    /* 7 */ HG_ANIM_PANIC,
    /* 8 */ HG_ANIM_MAX
} HgAnimation;

typedef enum {
    /* 0 */ HG_CS_FIRST_ENCOUNTER,
    /* 1 */ HG_CS_GET_MASK,
    /* 2 */ HG_CS_SUBSEQUENT_ENCOUNTER,
    /* 3 */ HG_CS_SONG_OF_HEALING
} HgCsIndex;

void EnHg_HandleCutscene(EnHg* this, PlayState* play) {
    if (Cutscene_IsCueInChannel(play, CS_CMD_ACTOR_CUE_484)) {
        s32 cueChannel = Cutscene_GetCueChannel(play, CS_CMD_ACTOR_CUE_484);

        if (this->csIdList[3] != play->csCtx.actorCues[cueChannel]->id) {
            this->csIdList[3] = play->csCtx.actorCues[cueChannel]->id;
            switch (play->csCtx.actorCues[cueChannel]->id) {
                case 1:
                    this->animIndex = HG_ANIM_IDLE;
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sPamelasFatherGibdoAnimationInfo, HG_ANIM_IDLE);
                    break;

                case 2:
                    this->csIdList[2] = 0;
                    this->animIndex = HG_ANIM_LEAN_FORWARD;
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sPamelasFatherGibdoAnimationInfo, HG_ANIM_LEAN_FORWARD);
                    break;

                case 3:
                    this->csIdList[2] = 0;
                    this->animIndex = HG_ANIM_CURL_UP;
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sPamelasFatherGibdoAnimationInfo, HG_ANIM_CURL_UP);
                    break;

                case 4:
                    this->csIdList[2] = 0;
                    this->animIndex = HG_ANIM_PANIC;
                    if ((this->csIdIndex == HG_CS_GET_MASK) || (this->csIdIndex == HG_CS_SONG_OF_HEALING)) {
                        Audio_PlaySfx_2(NA_SE_EN_HALF_REDEAD_TRANS);
                    }
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sPamelasFatherGibdoAnimationInfo, HG_ANIM_PANIC);
                    break;

                case 5:
                    this->animIndex = HG_ANIM_LURCH_FORWARD;
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sPamelasFatherGibdoAnimationInfo, HG_ANIM_LURCH_FORWARD);
                    break;

                case 6:
                    SET_WEEKEVENTREG(WEEKEVENTREG_75_20);
                    Actor_Kill(&this->actor);
                    break;

                default:
                    break;
            }
            actor_set_interpolation_skipped(&this->actor);
        } else if (Animation_OnFrame(&this->skelAnime, this->skelAnime.endFrame)) {
            switch (this->animIndex) {
                case HG_ANIM_LEAN_FORWARD:
                    this->animIndex = HG_ANIM_REACH_FORWARD;
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sPamelasFatherGibdoAnimationInfo, HG_ANIM_REACH_FORWARD);
                    break;

                case HG_ANIM_CURL_UP:
                    this->animIndex = HG_ANIM_CROUCHED_PANIC;
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sPamelasFatherGibdoAnimationInfo, HG_ANIM_CROUCHED_PANIC);
                    break;
            
                default:
                    break;
            }
            actor_set_interpolation_skipped(&this->actor);
        }

        switch (this->animIndex) {
            case HG_ANIM_LEAN_FORWARD:
            case HG_ANIM_REACH_FORWARD:
                Actor_PlaySfx_Flagged(&this->actor, NA_SE_EN_HALF_REDEAD_LOOP - SFX_FLAG);
                break;

            case HG_ANIM_CURL_UP:
            case HG_ANIM_CROUCHED_PANIC:
                Actor_PlaySfx_Flagged(&this->actor, NA_SE_EN_HALF_REDEAD_SCREAME - SFX_FLAG);
                break;

            case HG_ANIM_PANIC:
                if ((this->csIdIndex == HG_CS_FIRST_ENCOUNTER) || (this->csIdIndex == HG_CS_SUBSEQUENT_ENCOUNTER)) {
                    Actor_PlaySfx_Flagged(&this->actor, NA_SE_EN_HALF_REDEAD_SCREAME - SFX_FLAG);
                }
                break;

            default:
                break;
        }

        Cutscene_ActorTranslateAndYaw(&this->actor, play, cueChannel);
        return;

    } else if (play->csCtx.state == CS_STATE_IDLE) {
        EnHg_SetupWait(this);
    }

    this->csIdList[3] = 99;
}