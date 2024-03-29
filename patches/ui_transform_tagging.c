#include "patches.h"
#include "transform_ids.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"

extern s16 sCursorPrimR;
extern s16 sCursorPrimG;
extern s16 sCursorPrimB;
extern s16 sCursorEnvR;
extern s16 sCursorEnvG;
extern s16 sCursorEnvB;
extern f32 sCursorCirclesX[4];
extern f32 sCursorCirclesY[4];

extern u64 gPauseMenuCursorTex[];

s16 kaleido_s16(s16* addr) {
    return *(s16*)KaleidoManager_GetRamAddr(addr);
}

f32 kaleido_f32(f32* addr) {
    return *(f32*)KaleidoManager_GetRamAddr(addr);
}

void KaleidoScope_DrawCursor(PlayState* play) {
    PauseContext* pauseCtx = &play->pauseCtx;
    s16 i;

    OPEN_DISPS(play->state.gfxCtx);

    if ((pauseCtx->mainState == PAUSE_MAIN_STATE_IDLE) ||
        (pauseCtx->mainState == PAUSE_MAIN_STATE_IDLE_CURSOR_ON_SONG) ||
        ((pauseCtx->pageIndex == PAUSE_QUEST) && ((pauseCtx->mainState <= PAUSE_MAIN_STATE_SONG_PLAYBACK) ||
                                                  (pauseCtx->mainState == PAUSE_MAIN_STATE_SONG_PROMPT) ||
                                                  (pauseCtx->mainState == PAUSE_MAIN_STATE_IDLE_CURSOR_ON_SONG)))) {
        gDPPipeSync(POLY_OPA_DISP++);
        gDPSetCombineLERP(POLY_OPA_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0,
                          PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);

        // @recomp manual relocations, TODO remove when recompiler can handle this.
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, kaleido_s16(&sCursorPrimR), kaleido_s16(&sCursorPrimG), kaleido_s16(&sCursorPrimB), 255);
        gDPSetEnvColor(POLY_OPA_DISP++, kaleido_s16(&sCursorEnvR), kaleido_s16(&sCursorEnvG), kaleido_s16(&sCursorEnvB), 255);

        Matrix_Translate(pauseCtx->cursorX, pauseCtx->cursorY, -50.0f, MTXMODE_NEW);
        Matrix_Scale(1.0f, 1.0f, 1.0f, MTXMODE_APPLY);

        for (i = 0; i < 4; i++) {
            Matrix_Push();
            // @recomp manual relocations, TODO remove when recompiler can handle this.
            Matrix_Translate(kaleido_f32(&sCursorCirclesX[i]), kaleido_f32(&sCursorCirclesY[i]), -50.0f, MTXMODE_APPLY);

            // @recomp Tag the current pause cursor segment.
            gEXMatrixGroupDecomposedVerts(POLY_OPA_DISP++, PAUSE_CURSOR_TRANSFORM_ID_START + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);

            gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gDPPipeSync(POLY_OPA_DISP++);
            gDPLoadTextureBlock(POLY_OPA_DISP++, gPauseMenuCursorTex, G_IM_FMT_IA, G_IM_SIZ_8b, 16, 16, 0,
                                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK,
                                G_TX_NOLOD, G_TX_NOLOD);
            gSPVertex(POLY_OPA_DISP++, &pauseCtx->cursorVtx[0], 4, 0);
            gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);

            // @recomp Pop the pause cursor segment's transform id.
            gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);

            Matrix_Pop();
        }

        gDPPipeSync(POLY_OPA_DISP++);
        gDPSetEnvColor(POLY_OPA_DISP++, 0, 0, 0, 255);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}