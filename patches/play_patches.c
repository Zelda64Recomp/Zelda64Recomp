#include "play_patches.h"
#include "z64debug_display.h"
#include "input.h"

extern Input D_801F6C18;

void controls_play_update(PlayState* play) {
    gSaveContext.options.zTargetSetting = recomp_get_targeting_mode();
}

// @recomp Patched to add hooks for various added functionality.
RECOMP_PATCH void Play_Main(GameState* thisx) {
    static Input* prevInput = NULL;
    PlayState* this = (PlayState*)thisx;

    // @recomp
    debug_play_update(this);
    controls_play_update(this);
    analog_cam_pre_play_update(this);
    matrix_play_update(this);
    
    // @recomp avoid unused variable warning
    (void)prevInput;

    prevInput = CONTROLLER1(&this->state);
    DebugDisplay_Init();

    {
        GraphicsContext* gfxCtx = this->state.gfxCtx;

        if (1) {
            this->state.gfxCtx = NULL;
        }
        camera_pre_play_update(this);
        Play_Update(this);
        camera_post_play_update(this);
        analog_cam_post_play_update(this);
        autosave_post_play_update(this);
        this->state.gfxCtx = gfxCtx;
    }

    {
        Input input = *CONTROLLER1(&this->state);

        if (1) {
            *CONTROLLER1(&this->state) = D_801F6C18;
        }
        Play_Draw(this);
        *CONTROLLER1(&this->state) = input;
    }

    CutsceneManager_Update();
    CutsceneManager_ClearWaiting();
}

// @recomp Patched to add load a hook for loading rooms.
RECOMP_PATCH s32 Room_HandleLoadCallbacks(PlayState* play, RoomContext* roomCtx) {
    if (roomCtx->status == 1) {
        if (osRecvMesg(&roomCtx->loadQueue, NULL, OS_MESG_NOBLOCK) == 0) {
            roomCtx->status = 0;
            roomCtx->curRoom.segment = roomCtx->activeRoomVram;
            gSegments[3] = OS_K0_TO_PHYSICAL(roomCtx->activeRoomVram);

            // @recomp Call the room load hook.
            room_load_hook(play, &roomCtx->curRoom);

            Scene_ExecuteCommands(play, roomCtx->curRoom.segment);
            func_80123140(play, GET_PLAYER(play));
            Actor_SpawnTransitionActors(play, &play->actorCtx);

            if (((play->sceneId != SCENE_IKANA) || (roomCtx->curRoom.num != 1)) && (play->sceneId != SCENE_IKNINSIDE)) {
                play->envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
                play->envCtx.lightBlendOverride = LIGHT_BLEND_OVERRIDE_NONE;
            }
            func_800FEAB0();
            if (Environment_GetStormState(play) == STORM_STATE_OFF) {
                Environment_StopStormNatureAmbience(play);
            }
        } else {
            return 0;
        }
    }

    return 1;
}
