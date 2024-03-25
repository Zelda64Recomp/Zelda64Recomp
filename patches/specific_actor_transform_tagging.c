#include "patches.h"
#include "transform_ids.h"
#include "overlays/actors/ovl_En_Tanron2/z_en_tanron2.h"
#include "overlays/actors/ovl_Boss_04/z_boss_04.h"

extern EnTanron2* D_80BB8458[82];
extern Boss04* D_80BB8450;
extern f32 D_80BB8454;

extern Gfx gWartBubbleModelDL[];
extern Gfx gWartBubbleMaterialDL[];
extern Gfx gWartShadowMaterialDL[];
extern Gfx gEffWaterRippleDL[];

// @recomp Tag Wart's bubbles
void EnTanron2_Draw(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    s32 i;
    s32 j;
    s32 found;
    Actor* tanron2;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    gSPDisplayList(POLY_XLU_DISP++, gWartBubbleMaterialDL);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 150);

    tanron2 = play->actorCtx.actorLists[ACTORCAT_BOSS].first;

    // @recomp Manual relocation, TODO remove this when the recompiler does it automatically.
    EnTanron2** D_80BB8458_relocated = (EnTanron2**)actor_relocate(thisx, D_80BB8458);

    for (i = 0; i < ARRAY_COUNT(D_80BB8458); i++) {
        D_80BB8458_relocated[i] = NULL;
    }

    found = 0;
    while (tanron2 != NULL) {
        if (tanron2->params < 100) {
            D_80BB8458_relocated[found] = (EnTanron2*)tanron2;
            found++;
        }
        tanron2 = tanron2->next;
    }

    for (j = 0; j < found - 1; j++) {
        for (i = 0; i < found - 1; i++) {
            if (D_80BB8458_relocated[i + 1] != NULL) {
                if (D_80BB8458_relocated[i]->actor.projectedPos.z < D_80BB8458_relocated[i + 1]->actor.projectedPos.z) {
                    SWAP(EnTanron2*, D_80BB8458_relocated[i], D_80BB8458_relocated[i + 1]);
                }
            }
        }
    }
    // @recomp Extract this actor's ID.
    u32 actor_id = actor_transform_id(thisx);

    for (i = 0; i < ARRAY_COUNT(D_80BB8458); i++) {
        if (D_80BB8458_relocated[i] != NULL) {
            Matrix_Translate(D_80BB8458_relocated[i]->actor.world.pos.x, D_80BB8458_relocated[i]->actor.world.pos.y,
                             D_80BB8458_relocated[i]->actor.world.pos.z, MTXMODE_NEW);
            Matrix_ReplaceRotation(&play->billboardMtxF);
            Matrix_Scale(D_80BB8458_relocated[i]->actor.scale.x, D_80BB8458_relocated[i]->actor.scale.y, 0.0f, MTXMODE_APPLY);
            Matrix_RotateZS(D_80BB8458_relocated[i]->unk_14A, MTXMODE_APPLY);
            Matrix_Scale(0.13f, 0.14299999f, 0.13f, MTXMODE_APPLY);
            Matrix_RotateZS(-D_80BB8458_relocated[i]->unk_14A, MTXMODE_APPLY);

            // @recomp Tag the transform.
            gEXMatrixGroupSimple(POLY_XLU_DISP++, actor_transform_id(&D_80BB8458_relocated[i]->actor) + 0,
                G_EX_PUSH, G_MTX_MODELVIEW,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, gWartBubbleModelDL);

            // @recomp Pop the transform id.
            gEXPopMatrixGroup(POLY_XLU_DISP++);
        }
    }

    Gfx_SetupDL44_Xlu(play->state.gfxCtx);

    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 0, 0, 0, 150);
    gSPDisplayList(POLY_XLU_DISP++, gWartShadowMaterialDL);
    
    // @recomp Manual relocation, TODO remove this when the recompiler does it automatically.
    Boss04** D_80BB8450_relocated = (Boss04**)actor_relocate(thisx, &D_80BB8450);

    tanron2 = play->actorCtx.actorLists[ACTORCAT_BOSS].first;
    while (tanron2 != NULL) {
        if ((tanron2->params < 100) && (((EnTanron2*)tanron2)->unk_15B != 0)) {
            Matrix_Translate(tanron2->world.pos.x, (*D_80BB8450_relocated)->actor.floorHeight, tanron2->world.pos.z, MTXMODE_NEW);
            Matrix_Scale(0.6f, 0.0f, 0.6f, MTXMODE_APPLY);

            // @recomp Tag the transform.
            gEXMatrixGroupSimple(POLY_XLU_DISP++, actor_transform_id(tanron2) + 1,
                G_EX_PUSH, G_MTX_MODELVIEW,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, gWartShadowModelDL);

            // @recomp Pop the transform id.
            gEXPopMatrixGroup(POLY_XLU_DISP++);
        }
        tanron2 = tanron2->next;
    }

    Gfx_SetupDL60_XluNoCD(play->state.gfxCtx);

    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    gDPSetAlphaDither(POLY_XLU_DISP++, G_AD_NOISE);
    gDPSetColorDither(POLY_XLU_DISP++, G_CD_NOISE);

    tanron2 = play->actorCtx.actorLists[ACTORCAT_BOSS].first;
    while (tanron2 != NULL) {
        if ((tanron2->params < 100) && (((EnTanron2*)tanron2)->unk_15B != 0) &&
            (tanron2->world.pos.y <= tanron2->floorHeight)) {
            Matrix_Translate(tanron2->world.pos.x, (*D_80BB8450_relocated)->actor.floorHeight + 2.0f, tanron2->world.pos.z,
                             MTXMODE_NEW);
                             
            // @recomp Manual relocation, TODO remove this when the recompiler does it automatically.
            f32 D_80BB8454_value = *(f32*)actor_relocate(thisx, &D_80BB8454);
            Matrix_Scale(D_80BB8454_value, 0.0f, D_80BB8454_value, MTXMODE_APPLY);

            // @recomp Tag the transform.
            gEXMatrixGroupSimple(POLY_XLU_DISP++, actor_transform_id(tanron2) + 2,
                G_EX_PUSH, G_MTX_MODELVIEW,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, gEffWaterRippleDL);

            // @recomp Pop the transform id.
            gEXPopMatrixGroup(POLY_XLU_DISP++);
        }
        tanron2 = tanron2->next;
    }

    CLOSE_DISPS(play->state.gfxCtx);
}
