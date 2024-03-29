#include "patches.h"
#include "graphics.h"
#include "sys_cfb.h"
#include "z64view.h"
#include "transform_ids.h"

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
        s32 original_alpha = alpha;
        f32 exponent = 20.0f / recomp_get_target_framerate(gFramerateDivisor);
        f32 alpha_float = recomp_powf(alpha / 255.0f, exponent);
        // Clamp the blur alpha, which ensures that the output color converges to within a reasonable delta of the target color
        // when using an R8G8B8A8 framebuffer as RT64 currently does. Although this makes the effect less noticeable at high framerates,
        // not clamping leads to noticeable image retention.
        alpha_float = MIN(alpha_float, 0.825f);
        alpha = (s32)(alpha_float * 255.0f);

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

// @recomp Patched to increase the scale based on the aspect ratio.
void Actor_DrawLensOverlay(Gfx** gfxP, s32 lensMaskSize) {
    // @recomp Calculate the increase in aspect ratio.
    f32 original_aspect_ratio = (float)SCREEN_WIDTH / SCREEN_HEIGHT;
    f32 aspect_ratio_scale = recomp_get_aspect_ratio(original_aspect_ratio) / original_aspect_ratio;

    // @recomp Increase the circle's scale based on the aspect ratio scale. Also increase the base scaling
    // from 0.003f to 0.004f to account for overscan removal.
    TransitionCircle_LoadAndSetTexture(gfxP, gCircleTex, 4, 0, 6, 6,
                                       ((LENS_MASK_ACTIVE_SIZE - lensMaskSize) * 0.004f * aspect_ratio_scale) + 1.0f);
}


// @recomp Patched to use ortho tris for interpolation and to prevent the telescope and lens effects from getting stretched wide.
void TransitionCircle_LoadAndSetTexture(Gfx** gfxp, TexturePtr texture, s32 fmt, s32 arg3, s32 masks, s32 maskt,
                                        f32 arg6) {
    Gfx* gfx = *gfxp;
    s32 xh = gCfbWidth;
    s32 yh = gCfbHeight;
    s32 width = 1 << masks;
    s32 height = 1 << maskt;
    f32 s;
    f32 t;
    s32 dtdy;
    s32 dsdx;

    gDPLoadTextureBlock_4b(gfx++, texture, fmt, width, height, 0, G_TX_MIRROR | G_TX_CLAMP, G_TX_MIRROR | G_TX_CLAMP,
                           masks, maskt, G_TX_NOLOD, G_TX_NOLOD);
    gDPSetTileSize(gfx++, G_TX_RENDERTILE, 0, 0, ((width * 2) - 1) << 2, ((height * 2) - 1) << 2);

    s = ((1.0f - (1.0f / arg6)) * (SCREEN_WIDTH / 2)) + 70.0f;
    t = ((1.0f - (1.0f / arg6)) * (SCREEN_HEIGHT / 2)) + 50.0f;

    if (s < -1023.0f) {
        s = -1023.0f;
    }
    if (t < -1023.0f) {
        t = -1023.0f;
    }

    if ((s <= -1023.0f) || (t <= -1023.0f)) {
        dsdx = 0;
        dtdy = 0;
    } else {
        dsdx = ((SCREEN_WIDTH - (2.0f * s)) / gScreenWidth) * (1 << 10);
        dtdy = ((SCREEN_HEIGHT - (2.0f * t)) / gScreenHeight) * (1 << 10);
    }

    // Push the old RDP/RSP params.
    gEXPushProjectionMatrix(gfx++);
    gEXPushGeometryMode(gfx++);
    gEXMatrixGroupSimple(gfx++, CIRCLE_OVERLAY_TRANSFORM_PROJECTION_ID, G_EX_PUSH, G_MTX_PROJECTION,
        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);
    gEXMatrixGroupSimple(gfx++, CIRCLE_OVERLAY_TRANSFORM_ID, G_EX_PUSH, G_MTX_MODELVIEW,
        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);

    // Allocate a matrix and vertices in the displaylist because there's no handle to the GfxContext here.
    Mtx* ortho_matrix = (Mtx*)(gfx + 1);
    Mtx* model_matrix = ortho_matrix + 1;
    Gfx* after_matrix = (Gfx*)(model_matrix + 4);
    gSPBranchList(gfx++, after_matrix);
    gfx = after_matrix;

    // Set up an ortho projection matrix.
    guOrtho(ortho_matrix, -SCREEN_WIDTH / 2, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, -SCREEN_HEIGHT / 2, -1.0f, 1.0f, 1.0f);
    gSPMatrix(gfx++, ortho_matrix, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);

    // Set up a scale model matrix, using the original texcoord scaling to calculate the matrix's scale.
    guScale(model_matrix, 1024.0f / dsdx, 1024.0f / dtdy, 1.0f);
    gSPMatrix(gfx++, model_matrix, G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    
    // Enable texturing and set geometry mode.
    gSPTexture(gfx++, 0x8000 * width / 64, 0x8000 * height / 64, 0, G_TX_RENDERTILE, G_ON);
    gSPLoadGeometryMode(gfx++, 0);

    static Vtx overlay_verts[] = {
        // The quad that holds the lens itself.
        {{{  -64,   -64, 0}, 0, {       0,        0}, {0, 0, 0, 255}}},
        {{{   64,   -64, 0}, 0, {512 << 5,        0}, {0, 0, 0, 255}}},
        {{{  -64,    64, 0}, 0, {       0, 512 << 5}, {0, 0, 0, 255}}},
        {{{   64,    64, 0}, 0, {512 << 5, 512 << 5}, {0, 0, 0, 255}}},
        // The top verts of the quad above the lens.
        {{{  -64, -1000, 0}, 0, {       0,        0}, {0, 0, 0, 255}}},
        {{{   64, -1000, 0}, 0, {512 << 5,        0}, {0, 0, 0, 255}}},
        // The bottom verts of the quad below the lens.
        {{{  -64,  1000, 0}, 0, {       0, 512 << 5}, {0, 0, 0, 255}}},
        {{{   64,  1000, 0}, 0, {512 << 5, 512 << 5}, {0, 0, 0, 255}}},
        // The left verts of the quad to the left of the lens.
        {{{-4000, -1000, 0}, 0, {       0,        0}, {0, 0, 0, 255}}},
        {{{-4000,  1000, 0}, 0, {       0, 512 << 5}, {0, 0, 0, 255}}},
        // The right verts of the quad to the right of the lens.
        {{{ 4000, -1000, 0}, 0, {512 << 5,        0}, {0, 0, 0, 255}}},
        {{{ 4000,  1000, 0}, 0, {512 << 5, 512 << 5}, {0, 0, 0, 255}}},
    };

    // 8  4  5  10
    //    0  1
    //    2  3
    // 9  6  7  11

    // Load the verts.
    gSPVertex(gfx++, overlay_verts, ARRAY_COUNT(overlay_verts), 0);
    // Draw the quad containing the lens overlay.
    gSP2Triangles(gfx++, 0, 1, 3, 0x0, 0, 3, 2, 0x0);
    // Draw the quad above the lens overlay.
    gSP2Triangles(gfx++, 4, 5, 1, 0x0, 4, 1, 0, 0x0);
    // Draw the quad below the lens overlay.
    gSP2Triangles(gfx++, 2, 3, 7, 0x0, 2, 7, 6, 0x0);
    // Draw the quad to the left of the lens overlay.
    gSP2Triangles(gfx++, 8, 4, 6, 0x0, 8, 6, 9, 0x0);
    // Draw the quad to the right of the lens overlay.
    gSP2Triangles(gfx++, 5, 10, 11, 0x0, 5, 11, 7, 0x0);

    // Restore the old RDP/RSP params.
    gEXPopProjectionMatrix(gfx++);
    gEXPopGeometryMode(gfx++);
    gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
    gEXPopMatrixGroup(gfx++, G_MTX_MODELVIEW);
    gEXPopMatrixGroup(gfx++, G_MTX_PROJECTION);

    gDPPipeSync(gfx++);

    *gfxp = gfx;
}
