#include "patches.h"
#include "graphics.h"

extern TransitionOverlay gTransitionOverlayTable[];
extern Gfx sTransWipe3DL[];

#define THIS ((TransitionWipe3*)thisx)
// @recomp patched to scale the transition based on aspect ratio
void TransitionWipe3_Draw(void* thisx, Gfx** gfxP) {
    Gfx* gfx = *gfxP;
    Mtx* modelView = &THIS->modelView[THIS->frame];
    f32 scale = 14.8f;
    Gfx* texScroll;
    
    // @recomp Modify the scale based on the aspect ratio to make sure the transition circle covers the whole screen
    float original_aspect_ratio = ((float)SCREEN_WIDTH) / ((float)SCREEN_HEIGHT);
    scale *= recomp_get_aspect_ratio(original_aspect_ratio) / original_aspect_ratio;

    THIS->frame ^= 1;
    gDPPipeSync(gfx++);
    texScroll = Gfx_BranchTexScroll(&gfx, THIS->scrollX, THIS->scrollY, 16, 64);
    gSPSegment(gfx++, 0x09, texScroll);
    gSPSegment(gfx++, 0x08, THIS->curTexture);
    gDPSetColor(gfx++, G_SETPRIMCOLOR, THIS->color.rgba);
    gDPSetColor(gfx++, G_SETENVCOLOR, THIS->color.rgba);
    gSPMatrix(gfx++, &THIS->projection, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    gSPPerspNormalize(gfx++, THIS->normal);
    gSPMatrix(gfx++, &THIS->lookAt, G_MTX_NOPUSH | G_MTX_MUL | G_MTX_PROJECTION);

    if (scale != 1.0f) {
        guScale(modelView, scale, scale, 1.0f);
        gSPMatrix(gfx++, modelView, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    }
    // sTransWipe3DL is an overlay symbol, so its addresses need to be offset to get the actual loaded vram address.
    // TODO remove this once the recompiler is able to handle overlay symbols automatically for patch functions.
    ptrdiff_t reloc_offset;
    TransitionOverlay* overlay_entry = &gTransitionOverlayTable[FBDEMO_WIPE3];
    reloc_offset = (uintptr_t)Lib_PhysicalToVirtual(overlay_entry->loadInfo.addr) - (uintptr_t)overlay_entry->vramStart;
    gSPDisplayList(gfx++, (Gfx*)((u8*)sTransWipe3DL + reloc_offset));
    gDPPipeSync(gfx++);
    *gfxP = gfx;
}

#undef THIS


typedef enum {
    /* 0 */ MOTION_BLUR_OFF,
    /* 1 */ MOTION_BLUR_SETUP,
    /* 2 */ MOTION_BLUR_PROCESS
} MotionBlurStatus;

extern u8 sMotionBlurStatus;
extern s32 gFramerateDivisor;

// @recomp Motion blur works fine normally, but when running at a higher framerate the effect is much less pronounced
// as the previous frames decay quicker due to there being more frames drawn in the same period of time.
void Play_DrawMotionBlur(PlayState* this) {
    GraphicsContext* gfxCtx = this->state.gfxCtx;
    s32 alpha;
    Gfx* gfx;
    Gfx* gfxHead;

    if (R_MOTION_BLUR_PRIORITY_ENABLED) {
        alpha = R_MOTION_BLUR_PRIORITY_ALPHA;

        if (sMotionBlurStatus == MOTION_BLUR_OFF) {
            sMotionBlurStatus = MOTION_BLUR_SETUP;
        }
    } else if (R_MOTION_BLUR_ENABLED) {
        alpha = R_MOTION_BLUR_ALPHA;

        if (sMotionBlurStatus == MOTION_BLUR_OFF) {
            sMotionBlurStatus = MOTION_BLUR_SETUP;
        }
    } else {
        alpha = 0;
        sMotionBlurStatus = MOTION_BLUR_OFF;
    }

    if (sMotionBlurStatus != MOTION_BLUR_OFF) {
        OPEN_DISPS(gfxCtx);

        gfxHead = POLY_OPA_DISP;
        gfx = Graph_GfxPlusOne(gfxHead);

        gSPDisplayList(OVERLAY_DISP++, gfx);

        this->pauseBgPreRender.fbuf = gfxCtx->curFrameBuffer;
        this->pauseBgPreRender.fbufSave = this->unk_18E64;

        // @recomp Scale alpha based on the target framerate so that the blur effect decays at an equivalent rate
        // to how it does in the original game's framerate.
        alpha = (s32)(recomp_powf(alpha / 255.0f, 20.0f / recomp_get_target_framerate(gFramerateDivisor)) * 255.0f);

        if (sMotionBlurStatus == MOTION_BLUR_PROCESS) {
            func_80170AE0(&this->pauseBgPreRender, &gfx, alpha);
        } else {
            sMotionBlurStatus = MOTION_BLUR_PROCESS;
        }

        PreRender_SaveFramebuffer(&this->pauseBgPreRender, &gfx);

        gSPEndDisplayList(gfx++);

        Graph_BranchDlist(gfxHead, gfx);

        POLY_OPA_DISP = gfx;

        CLOSE_DISPS(gfxCtx);
    }
}
