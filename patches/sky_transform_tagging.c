#include "patches.h"
#include "graphics.h"
#include "transform_ids.h"
#include "libc64/qrand.h"

Gfx default_star_setup_dl[] = {
    gsDPSetCombineLERP(PRIMITIVE, 0, ENVIRONMENT, 0, PRIMITIVE, 0, ENVIRONMENT, 0, PRIMITIVE, 0, ENVIRONMENT, 0,
                      PRIMITIVE, 0, ENVIRONMENT, 0),
    gsDPSetOtherMode(
                    G_AD_DISABLE | G_CD_DISABLE | G_CK_NONE | G_TC_FILT | G_TF_POINT | G_TT_NONE | G_TL_TILE |
                        G_TD_CLAMP | G_TP_NONE | G_CYC_1CYCLE | G_PM_NPRIMITIVE,
                    G_AC_NONE | G_ZS_PRIM | G_RM_AA_XLU_LINE | G_RM_AA_XLU_LINE2),
    gsSPLoadGeometryMode(0),
    gsSPEndDisplayList(),
};

Gfx* star_setup_dl = default_star_setup_dl;

RECOMP_EXPORT void recomp_set_star_setup_dl(Gfx* g) {
    star_setup_dl = g;
}

static Vtx star_verts[] = {
    {{{ -1, -1, 0 }, 0, {  0 << 6,  0 << 6 }, { 0, 0, 0, 0xFF }}},
    {{{  1, -1, 0 }, 0, { 64 << 6,  0 << 6 }, { 0, 0, 0, 0xFF }}},
    {{{ -1,  1, 0 }, 0, {  0 << 6, 64 << 6 }, { 0, 0, 0, 0xFF }}},
    {{{  1,  1, 0 }, 0, { 64 << 6, 64 << 6 }, { 0, 0, 0, 0xFF }}}
};

Gfx default_star_draw_dl[] = {
    gsSPVertex(star_verts, ARRAY_COUNT(star_verts), 0),
    gsSP2Triangles(0, 1, 2, 0x0, 1, 3, 2, 0x0),
    gsSPEndDisplayList(),
};

Gfx* star_draw_dl = default_star_draw_dl;

RECOMP_EXPORT void recomp_set_star_draw_dl(Gfx* g) {
    star_draw_dl = g;
}

extern Mtx* sSkyboxDrawMatrix;

RECOMP_PATCH void Skybox_Draw(SkyboxContext* skyboxCtx, GraphicsContext* gfxCtx, s16 skyboxId, s16 blend, f32 x, f32 y, f32 z) {
    OPEN_DISPS(gfxCtx);

    Gfx_SetupDL40_Opa(gfxCtx);

    gSPSegment(POLY_OPA_DISP++, 0x0B, skyboxCtx->paletteStaticSegment);
    gSPTexture(POLY_OPA_DISP++, 0x8000, 0x8000, 0, G_TX_RENDERTILE, G_ON);

    sSkyboxDrawMatrix = GRAPH_ALLOC(gfxCtx, sizeof(Mtx));

    // @recomp skip drawing skyboxes with null textures as they hurt performance due to the accidental framebuffer effects they incur.
    // This needs to happen after sSkyboxDrawMatrix is allocated, otherwise the game will write to an invalid pointer later on which will cause a crash.
    if (skyboxCtx->staticSegments[0] == NULL || skyboxCtx->staticSegments[1] == NULL) {
        return;
    }

    Matrix_Translate(x, y, z, MTXMODE_NEW);
    Matrix_Scale(1.0f, 1.0f, 1.0f, MTXMODE_APPLY);
    Matrix_RotateXFApply(skyboxCtx->rot.x);
    Matrix_RotateYF(skyboxCtx->rot.y, MTXMODE_APPLY);
    Matrix_RotateZF(skyboxCtx->rot.z, MTXMODE_APPLY);
    Matrix_ToMtx(sSkyboxDrawMatrix);

    gSPMatrix(POLY_OPA_DISP++, sSkyboxDrawMatrix, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    // @recomp Tag the skybox's matrix, skipping interpolation if the camera's interpolation was also skipped.
    if (camera_was_skipped()) {
        gEXMatrixGroupDecomposedSkipAll(POLY_OPA_DISP++, SKYBOX_TRANSFORM_ID_START, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
    }
    else {
        gEXMatrixGroupDecomposedNormal(POLY_OPA_DISP++, SKYBOX_TRANSFORM_ID_START, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
    }

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
    gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);

    CLOSE_DISPS(gfxCtx);
}

// @recomp Draw stars with billboarding to allow for interpolation instead of rects.
void Environment_DrawSkyboxStarBillboard(GraphicsContext* gfxCtx, MtxF* billboard_mtx, Gfx** gfxp, f32 x, f32 y, f32 z, s32 width, s32 height) {
    Gfx* gfx = *gfxp;

    MtxF scale_matrix;
    MtxF mv_matrix;
    Mtx* m = GRAPH_ALLOC(gfxCtx, sizeof(Mtx));

    // Scales down the stars to roughly match what their original rect size was.
    SkinMatrix_SetScale(&scale_matrix, width * 25.0f / 4.0f, height * 25.0f / 4.0f, 1.0f);
    SkinMatrix_MtxFMtxFMult(billboard_mtx, &scale_matrix, &mv_matrix);
    mv_matrix.mf[3][0] = x;
    mv_matrix.mf[3][1] = y;
    mv_matrix.mf[3][2] = z;

    MtxConv_F2L(m, &mv_matrix);

    gSPMatrix(gfx++, m, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);

    gSPDisplayList(gfx++, star_draw_dl);

    gSPPopMatrix(gfx++, G_MTX_MODELVIEW);

    *gfxp = gfx;
}

extern f32 D_801F4F28;
extern s32 sEnvSkyboxNumStars;
s32 Environment_IsSceneUpsideDown(PlayState* play);
void Environment_DrawSkyboxStar(Gfx** gfxp, f32 x, f32 y, s32 width, s32 height);
void View_ViewportToVp(Vp* dest, Viewport* src);

f32 view_aspect_ratio(View* view) {
    f32 width = view->viewport.rightX - view->viewport.leftX;
    f32 height = view->viewport.bottomY - view->viewport.topY;
    f32 aspect = width / height;
    return aspect;
}

// @recomp Patched to set up the RSP for drawing stars with ortho rects and tag star transforms.
RECOMP_PATCH void Environment_DrawSkyboxStarsImpl(PlayState* play, Gfx** gfxP) {
    static const Vec3s D_801DD880[] = {
        { 0x0384, 0x2328, 0xD508 }, { 0x09C4, 0x2328, 0xDA1C }, { 0x0E74, 0x22D8, 0xDA1C }, { 0x1450, 0x2468, 0xD8F0 },
        { 0x1C84, 0x28A0, 0xCBA8 }, { 0x1F40, 0x2134, 0xD8F0 }, { 0x1F40, 0x28A0, 0xDAE4 }, { 0xE4A8, 0x4A38, 0x4A38 },
        { 0xD058, 0x4C2C, 0x3A98 }, { 0xD8F0, 0x36B0, 0x47E0 }, { 0xD954, 0x3264, 0x3E1C }, { 0xD8F0, 0x3070, 0x37DC },
        { 0xD8F0, 0x1F40, 0x5208 }, { 0xD760, 0x1838, 0x27D8 }, { 0x0000, 0x4E20, 0x4A38 }, { 0x076C, 0x2328, 0xDCD8 },
    };
    static const Color_RGBA8_u32 D_801DD8E0[] = {
        { 65, 164, 255, 255 },  { 131, 164, 230, 255 }, { 98, 205, 255, 255 }, { 82, 82, 255, 255 },
        { 123, 164, 164, 255 }, { 98, 205, 255, 255 },  { 98, 164, 230, 255 }, { 255, 90, 0, 255 },
    };
    UNALIGNED static const Color_RGBA8_u32 D_801DD900[] = {
        { 64, 80, 112, 255 },   { 96, 96, 128, 255 },   { 128, 112, 144, 255 }, { 160, 128, 160, 255 },
        { 192, 144, 168, 255 }, { 224, 160, 176, 255 }, { 224, 160, 176, 255 }, { 104, 104, 136, 255 },
        { 136, 120, 152, 255 }, { 168, 136, 168, 255 }, { 200, 152, 184, 255 }, { 232, 168, 184, 255 },
        { 224, 176, 184, 255 }, { 240, 192, 192, 255 }, { 232, 184, 192, 255 }, { 248, 200, 192, 255 },
    };
    Vec3f pos;
    f32 temp;
    f32 imgY;
    f32 imgX;
    Gfx* gfx;
    s32 phi_v1;
    s32 negateY;
    f32 invScale;
    f32 temp_f20;
    Gfx* gfxTemp;
    f32 scale;
    s32 i;
    u32 randInt;
    u32 imgWidth;
    f32* imgXPtr;
    f32* imgYPtr;
    Vec3f* posPtr;
    s32 pad[2];
    f32(*viewProjectionMtxF)[4];
    // @recomp Get the original and actual aspect ratios.
    f32 original_aspect_ratio = view_aspect_ratio(&play->view);
    f32 recomp_aspect_ratio = recomp_get_target_aspect_ratio(original_aspect_ratio);
    f32 recomp_aspect_ratio_scale = recomp_aspect_ratio / original_aspect_ratio;

    // @recomp Store the original billboard matrix.
    MtxF billboard_mtx;
    billboard_mtx = play->billboardMtxF;

    gfx = *gfxP;
    negateY = Environment_IsSceneUpsideDown(play);

    Matrix_MtxToMtxF(play->view.viewingPtr, &play->billboardMtxF);
    Matrix_MtxToMtxF(&play->view.projection, &play->viewProjectionMtxF);
    SkinMatrix_MtxFMtxFMult(&play->viewProjectionMtxF, &play->billboardMtxF, &play->viewProjectionMtxF);

    phi_v1 = 0;

    gDPPipeSync(gfx++);
    gDPSetEnvColor(gfx++, 255, 255, 255, 255.0f * D_801F4F28);

    gSPDisplayList(gfx++, star_setup_dl);

    randInt = ((u32)gSaveContext.save.saveInfo.playerData.playerName[0] << 0x18) ^
              ((u32)gSaveContext.save.saveInfo.playerData.playerName[1] << 0x14) ^
              ((u32)gSaveContext.save.saveInfo.playerData.playerName[2] << 0x10) ^
              ((u32)gSaveContext.save.saveInfo.playerData.playerName[3] << 0xC) ^
              ((u32)gSaveContext.save.saveInfo.playerData.playerName[4] << 8) ^
              ((u32)gSaveContext.save.saveInfo.playerData.playerName[5] << 4) ^
              ((u32)gSaveContext.save.saveInfo.playerData.playerName[6] << 0) ^
              ((u32)gSaveContext.save.saveInfo.playerData.playerName[7] >> 4) ^
              ((u32)gSaveContext.save.saveInfo.playerData.playerName[7] << 0x1C);

    //! FAKE:
    if (play->view.viewingPtr && play->view.viewingPtr && play->view.viewingPtr) {}

    for (i = 0; i < sEnvSkyboxNumStars; i++) {
        if (i < 16) {
            pos.x = play->view.eye.x + (s32)D_801DD880[i].x;
            pos.y = play->view.eye.y + (s32)D_801DD880[i].y;
            pos.z = play->view.eye.z + (s32)D_801DD880[i].z;
            imgWidth = 8;
        } else {
            f32 temp_f22;
            f32 temp_f4;
            f32 temp_f2;

            // temp_f4 = Rand_ZeroOne_Variable(&randInt);
            randInt = (randInt * RAND_MULTIPLIER) + RAND_INCREMENT;
            gRandFloat.i = (randInt >> 9) | 0x3F800000;
            temp = gRandFloat.f;
            temp_f4 = temp - 1.0f;

            // temp_f20 = Rand_ZeroOne_Variable(&randInt);
            randInt = (randInt * RAND_MULTIPLIER) + RAND_INCREMENT;
            gRandFloat.i = (randInt >> 9) | 0x3F800000;
            temp_f20 = ((gRandFloat.f - 1.0f) + temp_f4) * 0.5f;

            // Rand_Next_Variable(&randInt);
            randInt = (randInt * RAND_MULTIPLIER) + RAND_INCREMENT;

            // Set random position
            pos.y = play->view.eye.y + (SQ(temp_f20) * SQ(128.0f)) - 1000.0f;
            pos.x = play->view.eye.x + (Math_SinS(randInt) * (1.2f - temp_f20) * SQ(128.0f));
            pos.z = play->view.eye.z + (Math_CosS(randInt) * (1.2f - temp_f20) * SQ(128.0f));

            // temp_f2 = Rand_ZeroOne_Variable(&randInt);
            randInt = (randInt * RAND_MULTIPLIER) + RAND_INCREMENT;
            gRandFloat.i = ((randInt >> 9) | 0x3F800000);
            temp_f2 = gRandFloat.f - 1.0f;

            // Set random width
            // @recomp Scale down the max star size from 8+2 to 4+2.
            imgWidth = (u32)((SQ(temp_f2) * 4.0f) + 2.0f);
        }

        if (negateY) {
            pos.y = -pos.y;
        }

        if ((i < 15) || ((i == 15) && ((((void)0, gSaveContext.save.day) % 7) == 0))) {
            gDPSetColor(gfx++, G_SETPRIMCOLOR, D_801DD8E0[i % ARRAY_COUNTU(D_801DD8E0)].rgba);
        } else if (((i & 0x3F) == 0) || (i == 16)) {
            gDPSetColor(gfx++, G_SETPRIMCOLOR, D_801DD900[phi_v1 % ARRAY_COUNTU(D_801DD900)].rgba);
            phi_v1++;
        }

        posPtr = &pos;
        imgXPtr = &imgX;
        imgYPtr = &imgY;
        viewProjectionMtxF = play->viewProjectionMtxF.mf;

        if (imgWidth >= 2) {

            // @recomp Tag the star.
            if (camera_was_skipped()) {
                gEXMatrixGroupSimple(gfx++, STAR_TRANSFORM_ID_START + i, G_EX_PUSH, G_MTX_MODELVIEW,
                    G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);
            }
            else {
                gEXMatrixGroupSimple(gfx++, STAR_TRANSFORM_ID_START + i, G_EX_PUSH, G_MTX_MODELVIEW,
                    G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);
            }

            // @recomp Draw the star in 3D with a billboarded matrix.
            Environment_DrawSkyboxStarBillboard(play->state.gfxCtx, &billboard_mtx, &gfx, pos.x, pos.y, pos.z, imgWidth, imgWidth);

            // @recomp Pop the star's transform group.
            gEXPopMatrixGroup(gfx++, G_MTX_MODELVIEW);
        }
    }

    gDPPipeSync(gfx++);
    *gfxP = gfx;
}
