#include "patches.h"
#include "transform_ids.h"

extern Mtx* sSkyboxDrawMatrix;

void Skybox_Draw(SkyboxContext* skyboxCtx, GraphicsContext* gfxCtx, s16 skyboxId, s16 blend, f32 x, f32 y, f32 z) {
    // @recomp skip drawing skyboxes with null textures as they hurt performance due to the accidental framebuffer effects they incur.
    if (skyboxCtx->staticSegments[0] == NULL || skyboxCtx->staticSegments[1] == NULL) {
        return;
    }

    OPEN_DISPS(gfxCtx);

    Gfx_SetupDL40_Opa(gfxCtx);

    gSPSegment(POLY_OPA_DISP++, 0x0B, skyboxCtx->paletteStaticSegment);
    gSPTexture(POLY_OPA_DISP++, 0x8000, 0x8000, 0, G_TX_RENDERTILE, G_ON);

    sSkyboxDrawMatrix = GRAPH_ALLOC(gfxCtx, sizeof(Mtx));

    Matrix_Translate(x, y, z, MTXMODE_NEW);
    Matrix_Scale(1.0f, 1.0f, 1.0f, MTXMODE_APPLY);
    Matrix_RotateXFApply(skyboxCtx->rot.x);
    Matrix_RotateYF(skyboxCtx->rot.y, MTXMODE_APPLY);
    Matrix_RotateZF(skyboxCtx->rot.z, MTXMODE_APPLY);
    Matrix_ToMtx(sSkyboxDrawMatrix);

    gSPMatrix(POLY_OPA_DISP++, sSkyboxDrawMatrix, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    
    // @recomp Tag the skybox's matrix.
    gEXMatrixGroup(POLY_OPA_DISP++, SKYBOX_TRANSFORM_ID_START, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_ORDER_LINEAR);
    
    gDPSetColorDither(POLY_OPA_DISP++, G_CD_MAGICSQ);
    gDPSetTextureFilter(POLY_OPA_DISP++, G_TF_BILERP);
    gDPLoadTLUT_pal256(POLY_OPA_DISP++, skyboxCtx->paletteStaticSegment);
    gDPSetTextureLUT(POLY_OPA_DISP++, G_TT_RGBA16);
    gDPSetTextureConvert(POLY_OPA_DISP++, G_TC_FILT);
    gDPSetCombineLERP(POLY_OPA_DISP++, TEXEL1, TEXEL0, PRIMITIVE_ALPHA, TEXEL0, TEXEL1, TEXEL0, PRIMITIVE, TEXEL0,
                      PRIMITIVE, ENVIRONMENT, COMBINED, ENVIRONMENT, 0, 0, 0, COMBINED);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, skyboxCtx->prim.r, skyboxCtx->prim.g, skyboxCtx->prim.b, blend);
    gDPSetEnvColor(POLY_OPA_DISP++, skyboxCtx->env.r, skyboxCtx->env.g, skyboxCtx->env.b, 0);

    gSPDisplayList(POLY_OPA_DISP++, &skyboxCtx->dListBuf[0]);
    gSPDisplayList(POLY_OPA_DISP++, &skyboxCtx->dListBuf[2]);
    gSPDisplayList(POLY_OPA_DISP++, &skyboxCtx->dListBuf[4]);
    gSPDisplayList(POLY_OPA_DISP++, &skyboxCtx->dListBuf[6]);
    gSPDisplayList(POLY_OPA_DISP++, &skyboxCtx->dListBuf[8]);

    if (skyboxId == SKYBOX_CUTSCENE_MAP) {
        gSPDisplayList(POLY_OPA_DISP++, &skyboxCtx->dListBuf[10]);
    }

    gDPPipeSync(POLY_OPA_DISP++);

    // @recomp Pop the skybox's matrix tag.
    gEXPopMatrixGroup(POLY_OPA_DISP++);

    CLOSE_DISPS(gfxCtx);
}
