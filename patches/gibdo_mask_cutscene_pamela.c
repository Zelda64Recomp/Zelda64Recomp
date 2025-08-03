#include "patches.h"
#include "transform_ids.h"
#include "overlays/actors/ovl_En_Pamera/z_en_pamera.h"

RECOMP_PATCH s32 func_80BD9CB8(EnPamera* this, PlayState* play) {
    s32 cueChannel;

    if (Cutscene_IsCueInChannel(play, CS_CMD_ACTOR_CUE_485)) {
        cueChannel = Cutscene_GetCueChannel(play, CS_CMD_ACTOR_CUE_485);
        if (this->cueId != play->csCtx.actorCues[cueChannel]->id) {
            this->cueId = play->csCtx.actorCues[cueChannel]->id;

            switch (play->csCtx.actorCues[cueChannel]->id) {
                case 1:
                    func_80BD9E88(this);
                    break;

                case 2:
                    if (this->actor.draw == NULL) {
                        this->actor.draw = EnPamera_Draw;
                        this->actor.flags |= ACTOR_FLAG_TARGETABLE;
                    }
                    func_80BD9EE0(this);
                    break;

                case 3:
                    func_80BDA038(this);
                    break;

                case 4:
                    func_80BDA0A0(this);
                    break;

                case 5:
                    func_80BDA170(this);
                    break;

                case 6:
                    func_80BDA288(this);
                    break;

                default:
                    break;
            }
        }
        Cutscene_ActorTranslateAndYaw(&this->actor, play, cueChannel);
        this->setupFunc(this, play);
        return true;
    }
    if ((play->csCtx.state == CS_STATE_IDLE) && CHECK_WEEKEVENTREG(WEEKEVENTREG_75_20)) {
        if ((this->actionFunc != func_80BD994C) && (this->actionFunc != EnPamera_HandleDialogue)) {
            this->actor.shape.rot.y = this->actor.world.rot.y;
            func_80BD9904(this);
            func_80BD9E60(this);
        }
    }
    this->cueId = 99;
    return false;
}