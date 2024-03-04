#include "play_patches.h"
#include "z64debug_display.h"
#include "input.h"

extern Input D_801F6C18;

void controls_play_update(PlayState* play) {
    gSaveContext.options.zTargetSetting = recomp_get_targeting_mode();
}

// @recomp Patched to add hooks for various added functionality.
void Play_Main(GameState* thisx) {
    static Input* prevInput = NULL;
    PlayState* this = (PlayState*)thisx;

    // @recomp
    debug_play_update(this);
    controls_play_update(this);
    
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
