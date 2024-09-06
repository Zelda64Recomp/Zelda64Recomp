#include "patches.h"
#include "transform_ids.h"
#include "overlays/actors/ovl_En_Tanron2/z_en_tanron2.h"
#include "overlays/actors/ovl_Boss_03/z_boss_03.h"
#include "overlays/actors/ovl_Boss_04/z_boss_04.h"
#include "overlays/actors/ovl_Boss_Hakugin/z_boss_hakugin.h"
#include "overlays/actors/ovl_En_Water_Effect/z_en_water_effect.h"
#include "overlays/actors/ovl_En_Osn/z_en_osn.h"
#include "overlays/actors/ovl_En_Fall2/z_en_fall2.h"
#include "overlays/actors/ovl_Obj_Entotu/z_obj_entotu.h"
#include "overlays/actors/ovl_En_Goroiwa/z_en_goroiwa.h"
#include "overlays/actors/ovl_En_Twig/z_en_twig.h"
#include "overlays/actors/ovl_En_Honotrap/z_en_honotrap.h"
#include "overlays/actors/ovl_En_Tanron1/z_en_tanron1.h"

// Decomp renames, TODO update decomp and remove these
#define EnHonotrap_FlameGroup func_8092F878
#define EnHonotrap_DrawFlameGroup func_80930190

extern EnTanron2* D_80BB8458[82];
extern Boss04* D_80BB8450;
extern f32 D_80BB8454;

extern Gfx gWartBubbleModelDL[];
extern Gfx gWartBubbleMaterialDL[];
extern Gfx gWartShadowMaterialDL[];
extern Gfx gEffWaterRippleDL[];

// @recomp Tag gyorg's effects.
#define MAX_SPECIAL_EFFECTS 256
u8 special_effect_reset_states[MAX_SPECIAL_EFFECTS];

// @recomp Tag Wart's bubbles
RECOMP_PATCH void EnTanron2_Draw(Actor* thisx, PlayState* play2) {
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

    for (i = 0; i < ARRAY_COUNT(D_80BB8458); i++) {
        D_80BB8458[i] = NULL;
    }

    found = 0;
    while (tanron2 != NULL) {
        if (tanron2->params < 100) {
            D_80BB8458[found] = (EnTanron2*)tanron2;
            found++;
        }
        tanron2 = tanron2->next;
    }

    for (j = 0; j < found - 1; j++) {
        for (i = 0; i < found - 1; i++) {
            if (D_80BB8458[i + 1] != NULL) {
                if (D_80BB8458[i]->actor.projectedPos.z < D_80BB8458[i + 1]->actor.projectedPos.z) {
                    SWAP(EnTanron2*, D_80BB8458[i], D_80BB8458[i + 1]);
                }
            }
        }
    }

    for (i = 0; i < ARRAY_COUNT(D_80BB8458); i++) {
        if (D_80BB8458[i] != NULL) {
            Matrix_Translate(D_80BB8458[i]->actor.world.pos.x, D_80BB8458[i]->actor.world.pos.y,
                             D_80BB8458[i]->actor.world.pos.z, MTXMODE_NEW);
            Matrix_ReplaceRotation(&play->billboardMtxF);
            Matrix_Scale(D_80BB8458[i]->actor.scale.x, D_80BB8458[i]->actor.scale.y, 0.0f, MTXMODE_APPLY);
            Matrix_RotateZS(D_80BB8458[i]->unk_14A, MTXMODE_APPLY);
            Matrix_Scale(0.13f, 0.14299999f, 0.13f, MTXMODE_APPLY);
            Matrix_RotateZS(-D_80BB8458[i]->unk_14A, MTXMODE_APPLY);

            // @recomp Tag the transform.
            gEXMatrixGroupSimple(POLY_XLU_DISP++, actor_transform_id(&D_80BB8458[i]->actor) + 0,
                G_EX_PUSH, G_MTX_MODELVIEW,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, gWartBubbleModelDL);

            // @recomp Pop the transform id.
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        }
    }

    Gfx_SetupDL44_Xlu(play->state.gfxCtx);

    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 0, 0, 0, 150);
    gSPDisplayList(POLY_XLU_DISP++, gWartShadowMaterialDL);

    tanron2 = play->actorCtx.actorLists[ACTORCAT_BOSS].first;
    while (tanron2 != NULL) {
        if ((tanron2->params < 100) && (((EnTanron2*)tanron2)->unk_15B != 0)) {
            Matrix_Translate(tanron2->world.pos.x, D_80BB8450->actor.floorHeight, tanron2->world.pos.z, MTXMODE_NEW);
            Matrix_Scale(0.6f, 0.0f, 0.6f, MTXMODE_APPLY);

            // @recomp Tag the transform.
            gEXMatrixGroupSimple(POLY_XLU_DISP++, actor_transform_id(tanron2) + 1,
                G_EX_PUSH, G_MTX_MODELVIEW,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, gWartShadowModelDL);

            // @recomp Pop the transform id.
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
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
            Matrix_Translate(tanron2->world.pos.x, D_80BB8450->actor.floorHeight + 2.0f, tanron2->world.pos.z,
                             MTXMODE_NEW);
            Matrix_Scale(D_80BB8454, 0.0f, D_80BB8454, MTXMODE_APPLY);

            // @recomp Tag the transform.
            gEXMatrixGroupSimple(POLY_XLU_DISP++, actor_transform_id(tanron2) + 2,
                G_EX_PUSH, G_MTX_MODELVIEW,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, gEffWaterRippleDL);

            // @recomp Pop the transform id.
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        }
        tanron2 = tanron2->next;
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

// @recomp Track this effect's reset state.
RECOMP_PATCH void Boss03_SpawnEffectWetSpot(PlayState* play, Vec3f* pos) {
    s16 i;
    GyorgEffect* eff = play->specialEffects;

    for (i = 0; i < GYORG_EFFECT_COUNT; i++) {
        if ((eff->type == GYORG_EFFECT_NONE) || (eff->type == GYORG_EFFECT_BUBBLE)) {
            eff->type = GYORG_EFFECT_WET_SPOT;
            eff->pos = *pos;
            eff->unk_34.x = 0.1f;
            eff->unk_34.y = 0.4f;
            eff->velocity = gZeroVec3f;
            eff->accel = gZeroVec3f;
            eff->alpha = 150;
            eff->alphaDelta = Rand_ZeroFloat(4.0f) + 5.0f;
            // @recomp Tag this effect as having been reset.
            special_effect_reset_states[i] = true;
            return;
        }

        eff++;
    }
}

// @recomp Track this effect's reset state.
RECOMP_PATCH void Boss03_SpawnEffectDroplet(PlayState* play, Vec3f* pos) {
    s16 i;
    GyorgEffect* eff = play->specialEffects;

    for (i = 0; i < GYORG_EFFECT_COUNT; i++) {
        if ((eff->type == GYORG_EFFECT_NONE) || (eff->type == GYORG_EFFECT_BUBBLE)) {
            eff->type = GYORG_EFFECT_DROPLET;
            eff->pos = *pos;
            eff->velocity = gZeroVec3f;
            eff->accel = gZeroVec3f;
            eff->accel.y = -2.0f;
            eff->unk_34.x = 0.1f;
            eff->unk_34.y = 0.0f;
            eff->unk_34.z = Rand_ZeroFloat(2 * M_PI);
            eff->unk_02 = Rand_ZeroFloat(100.0f);
            eff->velocity.x = Rand_CenteredFloat(25.0f);
            eff->velocity.z = Rand_CenteredFloat(25.0f);
            // @recomp Tag this effect as having been reset.
            special_effect_reset_states[i] = true;
            return;
        }

        eff++;
    }
}

// @recomp Track this effect's reset state.
RECOMP_PATCH void Boss03_SpawnEffectSplash(PlayState* play, Vec3f* pos, Vec3f* velocity) {
    Vec3f accel = { 0.0f, -1.0f, 0.0f };
    f32 temp_f2;
    GyorgEffect* eff = play->specialEffects;
    s16 i;

    for (i = 0; i < GYORG_EFFECT_COUNT; i++) {
        if ((eff->type == GYORG_EFFECT_NONE) || (eff->type == GYORG_EFFECT_BUBBLE)) {
            eff->type = GYORG_EFFECT_SPLASH;
            eff->pos = *pos;
            eff->velocity = *velocity;
            eff->accel = accel;
            temp_f2 = Rand_ZeroFloat(0.02f) + 0.02f;
            eff->unk_34.y = temp_f2;
            eff->unk_34.x = temp_f2;
            eff->unk_34.z = Rand_ZeroFloat(2 * M_PI);
            eff->unk_02 = Rand_ZeroFloat(100.0f);
            // @recomp Tag this effect as having been reset.
            special_effect_reset_states[i] = true;
            return;
        }

        eff++;
    }
}

// @recomp Track this effect's reset state.
RECOMP_PATCH void Boss03_SpawnEffectBubble(PlayState* play, Vec3f* pos) {
    s16 i;
    GyorgEffect* eff = play->specialEffects;

    for (i = 0; i < GYORG_EFFECT_COUNT; i++) {
        if (eff->type == GYORG_EFFECT_NONE) {
            eff->type = GYORG_EFFECT_BUBBLE;
            eff->pos = *pos;
            eff->velocity = gZeroVec3f;
            eff->accel = gZeroVec3f;
            eff->accel.y = 0.2f;
            eff->unk_34.x = Rand_ZeroFloat(0.3f) + 0.2f;
            eff->unk_02 = 0;
            // @recomp Tag this effect as having been reset.
            special_effect_reset_states[i] = true;
            return;
        }

        eff++;
    }
}

extern Gfx object_water_effect_DL_004260[];
extern Gfx object_water_effect_DL_0042B0[];
extern Gfx object_water_effect_DL_0042F8[];
extern u8 gEffDust1Tex[];

void Boss03_SetObject(PlayState* play, s16 objectId);

// @recomp Tag Gyorg's effects.
RECOMP_PATCH void Boss03_DrawEffects(PlayState* play) {
    u8 flag = false;
    s16 i;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    s32 pad;
    GyorgEffect* eff = play->specialEffects;
    GyorgEffect* effFirst = eff;

    OPEN_DISPS(gfxCtx);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    Gfx_SetupDL25_Opa(gfxCtx);

    for (i = 0; i < GYORG_EFFECT_COUNT; i++, eff++) {
        if (eff->type == GYORG_EFFECT_BUBBLE) {
            if (!flag) {
                gSPDisplayList(POLY_OPA_DISP++, gGyorgBubbleMaterialDL);
                gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
                gDPSetEnvColor(POLY_OPA_DISP++, 150, 150, 150, 0);

                flag = true;
            }

            Matrix_Translate(eff->pos.x, eff->pos.y, eff->pos.z, MTXMODE_NEW);
            Matrix_Scale(eff->unk_34.x, eff->unk_34.x, 1.0f, MTXMODE_APPLY);
            Matrix_ReplaceRotation(&play->billboardMtxF);

            // @recomp Tag this effect and clear its reset state.
            if (special_effect_reset_states[i]) {
                gEXMatrixGroupDecomposedSkipAll(POLY_OPA_DISP++, SPECIAL_EFFECTS_TRANSFORM_ID_START + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
            }
            else {
                gEXMatrixGroupDecomposedNormal(POLY_OPA_DISP++, SPECIAL_EFFECTS_TRANSFORM_ID_START + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
            }
            special_effect_reset_states[i] = false;
            
            gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_OPA_DISP++, gGyorgBubbleModelDL);

            // @recomp Pop the effect's matrix group.
            gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);
        }
    }

    eff = effFirst;

    Boss03_SetObject(play, OBJECT_WATER_EFFECT);

    flag = false;

    for (i = 0; i < GYORG_EFFECT_COUNT; i++, eff++) {
        if ((eff->type == GYORG_EFFECT_DROPLET) || (eff->type == GYORG_EFFECT_SPLASH)) {

            if (!flag) {
                POLY_XLU_DISP = Gfx_SetupDL(POLY_XLU_DISP, SETUPDL_0);

                gSPSegment(POLY_XLU_DISP++, 0x08, Lib_SegmentedToVirtual(gEffDust1Tex));
                gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_004260);
                gDPSetEnvColor(POLY_XLU_DISP++, 250, 250, 255, 0);

                flag++;
            }

            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, (u8)eff->unk_40, (u8)((((void)0, eff->unk_40) + 55.0f)), 225, 150);

            Matrix_Translate(eff->pos.x, eff->pos.y, eff->pos.z, MTXMODE_NEW);

            if (eff->type == GYORG_EFFECT_DROPLET) {
                Matrix_RotateYF(BINANG_TO_RAD(Camera_GetInputDirYaw(GET_ACTIVE_CAM(play))), MTXMODE_APPLY);
            } else { // GYORG_EFFECT_SPLASH
                Matrix_ReplaceRotation(&play->billboardMtxF);
            }

            Matrix_Scale(eff->unk_34.x, eff->unk_34.y, 1.0f, MTXMODE_APPLY);
            Matrix_RotateZF(eff->unk_34.z, MTXMODE_APPLY);

            // @recomp Tag this effect and clear its reset state.
            if (special_effect_reset_states[i]) {
                gEXMatrixGroupDecomposedSkipAll(POLY_XLU_DISP++, SPECIAL_EFFECTS_TRANSFORM_ID_START + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
            }
            else {
                gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, SPECIAL_EFFECTS_TRANSFORM_ID_START + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
            }
            special_effect_reset_states[i] = false;

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_0042B0);

            // @recomp Pop the effect's matrix group.
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        }
    }

    eff = effFirst;

    flag = false;

    for (i = 0; i < GYORG_EFFECT_COUNT; i++, eff++) {
        if (eff->type == GYORG_EFFECT_WET_SPOT) {
            if (!flag) {
                Gfx_SetupDL44_Xlu(gfxCtx);

                gSPSegment(POLY_XLU_DISP++, 0x08, Lib_SegmentedToVirtual(gEffDust1Tex));
                gDPSetEnvColor(POLY_XLU_DISP++, 250, 250, 255, 0);
                gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_004260);

                flag++;
            }

            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, (s16)eff->unk_40, ((void)0, ((s16)eff->unk_40) + 55), 225,
                            eff->alpha);

            Matrix_Translate(eff->pos.x, eff->pos.y, eff->pos.z, MTXMODE_NEW);

            Matrix_Scale(eff->unk_34.x, 1.0f, eff->unk_34.x, MTXMODE_APPLY);
            Matrix_RotateYF(eff->unk_34.z, MTXMODE_APPLY);

            // @recomp Tag this effect and clear its reset state.
            if (special_effect_reset_states[i]) {
                gEXMatrixGroupDecomposedSkipAll(POLY_XLU_DISP++, SPECIAL_EFFECTS_TRANSFORM_ID_START + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
            }
            else {
                gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, SPECIAL_EFFECTS_TRANSFORM_ID_START + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
            }
            special_effect_reset_states[i] = false;

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_0042F8);

            // @recomp Pop the effect's matrix group.
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        }
    }

    Boss03_SetObject(play, OBJECT_BOSS03);

    CLOSE_DISPS(gfxCtx);
}

// @recomp Tag Gyorg's water effects.
extern AnimatedMaterial object_water_effect_Matanimheader_000DE0[];
extern AnimatedMaterial object_water_effect_Matanimheader_000E0C[];
extern AnimatedMaterial object_water_effect_Matanimheader_000E40[];
extern AnimatedMaterial object_water_effect_Matanimheader_000E58[];
extern Gfx object_water_effect_DL_000730[];
extern Gfx object_water_effect_DL_000420[];
extern Gfx object_water_effect_DL_000A48[];
extern Gfx object_water_effect_DL_000CD8[];

RECOMP_PATCH void func_80A5A6B8(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    EnWaterEffect* this = (EnWaterEffect*)thisx;
    EnWaterEffectStruct* ptr = &this->unk_144[0];
    u8 phi_s4 = false;
    s16 i;

    // @recomp Extract this actor's ID.
    u32 actor_id = actor_transform_id(thisx);

    OPEN_DISPS(play->state.gfxCtx);

    Matrix_Translate(this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z, MTXMODE_NEW);
    Matrix_RotateYS(this->actor.shape.rot.y, MTXMODE_APPLY);
    Matrix_Scale(this->actor.scale.x, this->actor.scale.y, this->actor.scale.z, MTXMODE_APPLY);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    gDPSetEnvColor(POLY_XLU_DISP++, 165, 235, 255, 128);

    Matrix_Push();
    Matrix_Push();
    Matrix_Push();

    if ((this->actor.params == ENWATEREFFECT_TYPE_GYORG_RIPPLES) ||
        (this->actor.params == ENWATEREFFECT_TYPE_GYORG_PRIMARY_SPRAY)) {
        if (this->unk_E2C > 1.0f) {
            Gfx_SetupDL25_Xlu(play->state.gfxCtx);
            AnimatedMat_Draw(play, Lib_SegmentedToVirtual(object_water_effect_Matanimheader_000DE0));
            Matrix_Scale(this->unk_DC8[1].y, this->unk_DC8[1].z, this->unk_DC8[1].y, MTXMODE_APPLY);

            // @recomp Tag the transform.
            gEXMatrixGroupDecomposedVerts(POLY_XLU_DISP++, actor_id + 0, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, (u8)this->unk_E2C);
            gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_000420);

            // @recomp Pop the transform id.
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        }

        Matrix_Pop();

        if (this->unk_E30 > 1.0f) {
            Gfx_SetupDL25_Xlu(play->state.gfxCtx);
            AnimatedMat_Draw(play, Lib_SegmentedToVirtual(object_water_effect_Matanimheader_000E0C));
            Matrix_Scale(this->unk_DC8[2].y, this->unk_DC8[2].z, this->unk_DC8[2].y, MTXMODE_APPLY);

            // @recomp Tag the transform.
            gEXMatrixGroupDecomposedVerts(POLY_XLU_DISP++, actor_id + 1, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, (u8)this->unk_E30);
            gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_000730);

            // @recomp Pop the transform id.
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        }
        Matrix_Pop();
    } else {
        Matrix_Pop();
        Matrix_Pop();
    }

    if ((this->unk_E34 > 1.0f) && (this->actor.params != ENWATEREFFECT_TYPE_GYORG_SHOCKWAVE)) {
        Gfx_SetupDL25_Xlu(play->state.gfxCtx);
        AnimatedMat_Draw(play, Lib_SegmentedToVirtual(object_water_effect_Matanimheader_000E40));
        Matrix_Scale(this->unk_DC8[3].y, this->unk_DC8[3].z, this->unk_DC8[3].y, MTXMODE_APPLY);

        // @recomp Tag the transform.
        gEXMatrixGroupDecomposedVerts(POLY_XLU_DISP++, actor_id + 2, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);

        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, (u8)this->unk_E34);
        gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_000A48);

        // @recomp Pop the transform id.
        gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
    }

    Matrix_Pop();

    if ((this->actor.params == ENWATEREFFECT_TYPE_GYORG_RIPPLES) ||
        (this->actor.params == ENWATEREFFECT_TYPE_GYORG_SHOCKWAVE)) {
        Gfx_SetupDL25_Xlu(play->state.gfxCtx);
        AnimatedMat_Draw(play, Lib_SegmentedToVirtual(object_water_effect_Matanimheader_000E58));
        Matrix_Scale(this->unk_DC8[4].y, this->unk_DC8[4].z, this->unk_DC8[4].y, MTXMODE_APPLY);

        // @recomp Tag the transform.
        gEXMatrixGroupDecomposedVerts(POLY_XLU_DISP++, actor_id + 3, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);

        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, (u8)this->unk_E38);
        gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_000CD8);

        // @recomp Pop the transform id.
        gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
    }

    if (this->actor.params == ENWATEREFFECT_TYPE_GYORG_RIPPLES) {
        Gfx_SetupDL25_Xlu(play->state.gfxCtx);

        for (i = 0; i < ARRAY_COUNT(this->unk_144) / 2; i++, ptr++) {
            if (ptr->unk_00 == 3) {
                if (!phi_s4) {
                    Gfx_SetupDL44_Xlu(play->state.gfxCtx);

                    gSPSegment(POLY_XLU_DISP++, 0x08, Lib_SegmentedToVirtual(gEffDust1Tex));
                    gDPSetEnvColor(POLY_XLU_DISP++, 250, 250, 255, 0);
                    gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_004260);
                    phi_s4++;
                }

                gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, (u8)ptr->unk_38, (u8)(((void)0, ptr->unk_38) + 55.0f), 225,
                                ptr->unk_3C);

                Matrix_Translate(ptr->unk_04.x, ptr->unk_04.y, ptr->unk_04.z, MTXMODE_NEW);
                Matrix_Scale(ptr->unk_2C.x, 1.0f, ptr->unk_2C.x, MTXMODE_APPLY);
                Matrix_RotateYF(ptr->unk_2C.z, MTXMODE_APPLY);

                // @recomp Tag the transform.
                gEXMatrixGroupDecomposedVerts(POLY_XLU_DISP++, actor_id + 4 + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);

                gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx),
                          G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_0042F8);

                // @recomp Pop the transform id.
                gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
            }
        }
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

// @recomp Tag normal water effects.
RECOMP_PATCH void EnWaterEffect_Draw(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    EnWaterEffect* this = (EnWaterEffect*)thisx;
    s32 pad;
    EnWaterEffectStruct* backupPtr = &this->unk_144[0];
    EnWaterEffectStruct* ptr = backupPtr;
    u8 phi_s4 = false;
    s16 i;

    OPEN_DISPS(gfxCtx);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    // @recomp Extract this actor's ID.
    u32 actor_id = actor_transform_id(thisx);

    for (i = 0; i < ARRAY_COUNT(this->unk_144) / 2; i++, ptr++) {
        if ((ptr->unk_00 == 1) || (ptr->unk_00 == 2)) {
            if (!phi_s4) {
                POLY_XLU_DISP = Gfx_SetupDL(POLY_XLU_DISP, SETUPDL_0);

                gSPSegment(POLY_XLU_DISP++, 0x08, Lib_SegmentedToVirtual(gEffDust1Tex));
                gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_004260);
                gDPSetEnvColor(POLY_XLU_DISP++, 250, 250, 255, 0);
                phi_s4++;
            }

            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, (u8)ptr->unk_38, (u8)(((void)0, ptr->unk_38) + 55.0f), 225, 150);

            Matrix_Translate(ptr->unk_04.x, ptr->unk_04.y, ptr->unk_04.z, MTXMODE_NEW);

            if (ptr->unk_00 == 1) {
                Matrix_RotateYS(Camera_GetInputDirYaw(GET_ACTIVE_CAM(play)), MTXMODE_APPLY);
            } else {
                Matrix_ReplaceRotation(&play->billboardMtxF);
            }

            Matrix_Scale(ptr->unk_2C.x, ptr->unk_2C.y, 1.0f, MTXMODE_APPLY);
            Matrix_RotateZF(ptr->unk_2C.z, MTXMODE_APPLY);

            // @recomp Tag the transform.
            gEXMatrixGroupDecomposedVerts(POLY_XLU_DISP++, actor_id + i + 0 * (ARRAY_COUNT(this->unk_144) / 2), G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_0042B0);

            // @recomp Pop the transform id.
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        }
    }

    phi_s4 = false;
    ptr = backupPtr;

    for (i = 0; i < ARRAY_COUNT(this->unk_144) / 2; i++, ptr++) {
        if (ptr->unk_00 == 3) {
            if (!phi_s4) {
                Gfx_SetupDL44_Xlu(gfxCtx);

                gSPSegment(POLY_XLU_DISP++, 0x08, Lib_SegmentedToVirtual(gEffDust1Tex));
                gDPSetEnvColor(POLY_XLU_DISP++, 250, 250, 255, 0);
                gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_004260);
                phi_s4++;
            }

            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, (u8)ptr->unk_38, (u8)(((void)0, ptr->unk_38) + 55.0f), 225,
                            ptr->unk_3C);

            Matrix_Translate(ptr->unk_04.x, ptr->unk_04.y, ptr->unk_04.z, MTXMODE_NEW);
            Matrix_Scale(ptr->unk_2C.x, 1.0f, ptr->unk_2C.x, MTXMODE_APPLY);
            Matrix_RotateYF(ptr->unk_2C.z, MTXMODE_APPLY);

            // @recomp Tag the transform.
            gEXMatrixGroupDecomposedVerts(POLY_XLU_DISP++, actor_id + i + 1 * (ARRAY_COUNT(this->unk_144) / 2), G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_0042F8);

            // @recomp Pop the transform id.
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        }
    }

    CLOSE_DISPS(gfxCtx);
}

extern Gfx gameplay_keep_DL_06AB30[];
extern Gfx gGohtRockMaterialDL[];
extern Gfx gGohtRockModelDL[];
extern Gfx gGohtStalactiteMaterialDL[];
extern Gfx gGohtStalactiteModelDL[];

// @recomp Tag Goht's rocks.
RECOMP_PATCH void func_80B0C398(BossHakugin* this, PlayState* play) {
    BossHakuginEffect* effect;
    s32 i;

    OPEN_DISPS(play->state.gfxCtx);

    gSPDisplayList(POLY_OPA_DISP++, gGohtRockMaterialDL);
    for (i = 0; i < ARRAY_COUNT(this->unk_9F8); i++) {
        effect = &this->unk_9F8[i];
        if ((effect->unk_18 >= 0) && (effect->unk_1A == 0)) {
            Matrix_SetTranslateRotateYXZ(effect->unk_0.x, effect->unk_0.y, effect->unk_0.z, &effect->unk_1C);
            Matrix_Scale(effect->unk_24, effect->unk_24, effect->unk_24, MTXMODE_APPLY);

            // @recomp Tag the transform.
            gEXMatrixGroupDecomposedVerts(POLY_OPA_DISP++, GOHT_ROCKS_TRANSFORM_ID_START + i + 0 * ARRAY_COUNT(this->unk_9F8), G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);

            gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_OPA_DISP++, gGohtRockModelDL);

            // @recomp Pop the transform id.
            gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);
        }
    }

    gSPDisplayList(POLY_OPA_DISP++, gGohtStalactiteMaterialDL);
    for (i = 0; i < ARRAY_COUNT(this->unk_9F8); i++) {
        effect = &this->unk_9F8[i];
        if ((effect->unk_18 >= 0) && (effect->unk_1A == 1)) {
            Matrix_SetTranslateRotateYXZ(effect->unk_0.x, effect->unk_0.y, effect->unk_0.z, &effect->unk_1C);
            Matrix_Scale(effect->unk_24, effect->unk_24, effect->unk_24, MTXMODE_APPLY);

            // @recomp Tag the transform.
            gEXMatrixGroupDecomposedVerts(POLY_OPA_DISP++, GOHT_ROCKS_TRANSFORM_ID_START + i + 1 * ARRAY_COUNT(this->unk_9F8), G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);

            gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_OPA_DISP++, gGohtStalactiteModelDL);

            // @recomp Pop the transform id.
            gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

// @recomp Skip interpolation for the Happy Mask Salesman when his animation changes.
extern AnimationInfo sHappyMaskSalesmanAnimationInfo[];

typedef enum {
    /*  0 */ OSN_ANIM_IDLE,
    /*  1 */ OSN_ANIM_ARMS_OUT,
    /*  2 */ OSN_ANIM_BOWING,
    /*  3 */ OSN_ANIM_REMINISCE,
    /*  4 */ OSN_ANIM_HANDS_CLASPED,
    /*  5 */ OSN_ANIM_BELIEVE,
    /*  6 */ OSN_ANIM_THINK,
    /*  7 */ OSN_ANIM_SHAKE_HEAD,
    /*  8 */ OSN_ANIM_ORGAN_TALK,
    /*  9 */ OSN_ANIM_ORGAN_PLAY,
    /* 10 */ OSN_ANIM_SHAKE,
    /* 11 */ OSN_ANIM_CHOKE,
    /* 12 */ OSN_ANIM_DESPAIR,
    /* 13 */ OSN_ANIM_FAST_BOWS,
    /* 14 */ OSN_ANIM_HAND_OUT,
    /* 15 */ OSN_ANIM_LYING_DOWN_FACE_UP,
    /* 16 */ OSN_ANIM_LYING_DOWN_FACE_DOWN,
    /* 17 */ OSN_ANIM_MASK_LOOK_AT,
    /* 18 */ OSN_ANIM_TURN_AROUND_START,
    /* 19 */ OSN_ANIM_TURN_AROUND_LOOP,
    /* 20 */ OSN_ANIM_WALK_AWAY,
    /* 21 */ OSN_ANIM_MASK_LOOK_FROM_START,
    /* 22 */ OSN_ANIM_MASK_LOOK_FROM_LOOP,
    /* 23 */ OSN_ANIM_HAND_OUT_2,    // Exact same as OSN_ANIM_HAND_OUT
    /* 24 */ OSN_ANIM_WALK_AWAY_END, // Only the last frame of OSN_ANIM_WALK_AWAY
    /* 25 */ OSN_ANIM_MAX
} OsnAnimation;

void EnOsn_HandleCsAction(EnOsn* this, PlayState* play);
void EnOsn_Idle(EnOsn* this, PlayState* play);

// @recomp Patched to skip interpolation when the Happy Mask Salesman changes animations.
RECOMP_PATCH void EnOsn_ChooseAction(EnOsn* this, PlayState* play) {
    u32 isSwitchFlagSet = Flags_GetSwitch(play, 0);

    this->csId = this->actor.csId;

    Actor_ChangeAnimationByInfo(&this->skelAnime, sHappyMaskSalesmanAnimationInfo, OSN_ANIM_IDLE);
    if (!isSwitchFlagSet) {
        this->actionFunc = EnOsn_HandleCsAction;
    } else {
        this->actionFunc = EnOsn_Idle;
    }
    
    // @recomp Skip interpolation this frame.
    actor_set_interpolation_skipped(&this->actor);
}

void EnOsn_TurnAround(EnOsn* this);
void EnOsn_LookFromMask(EnOsn* this);
void EnOsn_FadeOut(EnOsn* this);

// @recomp Patched to skip interpolation when the Happy Mask Salesman changes animations.
RECOMP_PATCH void EnOsn_HandleCsAction(EnOsn* this, PlayState* play) {
    u8 pad;
    s32 cueChannel;

    if (Cutscene_IsCueInChannel(play, CS_CMD_ACTOR_CUE_130)) {
        cueChannel = Cutscene_GetCueChannel(play, CS_CMD_ACTOR_CUE_130);
        this->shouldRotateHead = false;
        if (this->cueId != play->csCtx.actorCues[cueChannel]->id) {
            this->cueId = play->csCtx.actorCues[cueChannel]->id;
            switch (play->csCtx.actorCues[cueChannel]->id) {
                case 1:
                    this->animIndex = OSN_ANIM_BOWING;
                    break;

                case 2:
                    this->animIndex = OSN_ANIM_ARMS_OUT;
                    break;

                case 3:
                    this->animIndex = OSN_ANIM_SHAKE_HEAD;
                    break;

                case 4:
                    this->animIndex = OSN_ANIM_REMINISCE;
                    break;

                case 5:
                    this->animIndex = OSN_ANIM_THINK;
                    break;

                case 6:
                    this->animIndex = OSN_ANIM_BELIEVE;
                    break;

                case 7:
                    this->animIndex = OSN_ANIM_HANDS_CLASPED;
                    break;

                case 8:
                    this->animIndex = OSN_ANIM_IDLE;
                    break;

                case 10:
                    this->animIndex = OSN_ANIM_ORGAN_TALK;
                    break;

                case 11:
                    this->animIndex = OSN_ANIM_ORGAN_PLAY;
                    break;

                case 13:
                    this->animIndex = OSN_ANIM_SHAKE;
                    break;

                case 15:
                    this->animIndex = OSN_ANIM_CHOKE;
                    break;

                case 16:
                    this->animIndex = OSN_ANIM_DESPAIR;
                    break;

                case 17:
                    this->animIndex = OSN_ANIM_FAST_BOWS;
                    break;

                case 18:
                    this->animIndex = OSN_ANIM_HAND_OUT;
                    break;

                case 19:
                    this->animIndex = OSN_ANIM_MASK_LOOK_AT;
                    break;

                case 20:
                    this->animIndex = OSN_ANIM_TURN_AROUND_START;
                    break;

                case 21:
                    this->animIndex = OSN_ANIM_WALK_AWAY;
                    break;

                case 22:
                    this->animIndex = OSN_ANIM_MASK_LOOK_FROM_START;
                    break;

                case 23:
                    this->animIndex = OSN_ANIM_HAND_OUT_2;
                    break;

                case 24:
                    this->animIndex = OSN_ANIM_WALK_AWAY_END;
                    break;

                default:
                    this->animIndex = OSN_ANIM_IDLE;
                    break;
            }            
            Actor_ChangeAnimationByInfo(&this->skelAnime, sHappyMaskSalesmanAnimationInfo, this->animIndex);

            // @recomp Skip interpolation this frame.
            actor_set_interpolation_skipped(&this->actor);

        }

        if ((this->animIndex == OSN_ANIM_BELIEVE) && (play->sceneId == SCENE_SPOT00) &&
            (gSaveContext.sceneLayer == 0xB) && (play->csCtx.curFrame == 400)) {
            Actor_PlaySfx(&this->actor, NA_SE_VO_OMVO00);
        }

        if (this->animIndex == OSN_ANIM_TURN_AROUND_START) {
            EnOsn_TurnAround(this);
        }

        if (this->animIndex == OSN_ANIM_MASK_LOOK_FROM_START) {
            EnOsn_LookFromMask(this);
        }

        if (this->animIndex == OSN_ANIM_WALK_AWAY_END) {
            EnOsn_FadeOut(this);
        }

        if ((this->animIndex == OSN_ANIM_WALK_AWAY) &&
            (Animation_OnFrame(&this->skelAnime, 17.0f) || Animation_OnFrame(&this->skelAnime, 27.0f) ||
             Animation_OnFrame(&this->skelAnime, 37.0f) || Animation_OnFrame(&this->skelAnime, 47.0f) ||
             Animation_OnFrame(&this->skelAnime, 57.0f) || Animation_OnFrame(&this->skelAnime, 67.0f))) {
            Actor_PlaySfx(&this->actor, NA_SE_EV_OMENYA_WALK);
        }
        Cutscene_ActorTranslateAndYaw(&this->actor, play, cueChannel);
    } else {
        this->shouldRotateHead = true;
        this->cueId = 99;
        EnOsn_ChooseAction(this, play);
    }
}

// @recomp Patched to tag this actor's draws using linear order matching.
RECOMP_PATCH void EnFall2_Draw(Actor* thisx, PlayState* play) {
    s32 pad;
    EnFall2* this = (EnFall2*)thisx;
    Mtx* mtx;

    if (!(this->alphaLevel <= 0.0f)) {
        Gfx_SetupDL25_Xlu(play->state.gfxCtx);
        AnimatedMat_DrawXlu(play, Lib_SegmentedToVirtual(object_fall2_Matanimheader_008840));

        mtx = GRAPH_ALLOC(play->state.gfxCtx, this->skeletonInfo.unk_18->unk_1 * sizeof(Mtx));

        if (mtx != NULL) {
            Gfx_SetupDL25_Xlu(play->state.gfxCtx);
            Matrix_RotateYS((s16)(Camera_GetCamDirYaw(GET_ACTIVE_CAM(play)) + 0x8000), MTXMODE_APPLY);

            // @recomp Tag this actor's matrices.
            OPEN_DISPS(play->state.gfxCtx);
            gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, actor_transform_id(thisx), G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);

            func_8018450C(play, &this->skeletonInfo, mtx, NULL, NULL, &this->actor);
            
            // @recomp Pop the matrix group.
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
            CLOSE_DISPS(play->state.gfxCtx);
        }
    }
}

// @recomp Skip interpolation on item pickups the frame they're collected.
RECOMP_PATCH void func_800A6A40(EnItem00* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (this->getItemId != GI_NONE) {
        if (!Actor_HasParent(&this->actor, play)) {
            Actor_OfferGetItem(&this->actor, play, this->getItemId, 50.0f, 80.0f);
            this->unk152++;
        } else {
            this->getItemId = GI_NONE;
        }
    }

    if (this->unk152 == 0) {
        Actor_Kill(&this->actor);
        return;
    }

    this->actor.world.pos = player->actor.world.pos;

    if (this->actor.params <= ITEM00_RUPEE_RED) {
        this->actor.shape.rot.y += 0x3C0;
    } else if (this->actor.params == ITEM00_RECOVERY_HEART) {
        this->actor.shape.rot.y = 0;
    }

    this->actor.world.pos.y += (40.0f + (Math_SinS(this->unk152 * 15000) * (this->unk152 * 0.3f)));

    if (LINK_IS_ADULT) {
        this->actor.world.pos.y += 20.0f;
    }

    // @recomp Use custom flag 1 to check if this actor has been in this state before.
    if (!actor_get_custom_flag_1(&this->actor)) {
        // It hasn't, so skip interpolation this frame and set custom flag 1.
        actor_set_interpolation_skipped(&this->actor);
        actor_set_custom_flag_1(&this->actor);
    }
}

extern Vtx ovl_Obj_Entotu_Vtx_000D10[7];
extern Gfx object_f53_obj_DL_001C00[];

// @recomp Skip rotation interpolation for the Clock Town chimney's smoke when the camera skips interolation.
RECOMP_PATCH void func_80A34B28(ObjEntotu* this, PlayState* play) {
    u8 sp57;
    u8 sp56;
    s32 i;

    this->unk_1B8.y += 1.8f;
    this->unk_1B8.z += 0.6f;
    sp57 = 0x7F - (u8)this->unk_1B8.y;
    sp56 = 0x7F - (u8)this->unk_1B8.z;

    this->unk_1B8.x = CLAMP(this->unk_1B8.x, 0.0f, 1.0f);

    for (i = 0; i < ARRAY_COUNT(ovl_Obj_Entotu_Vtx_000D10); i++) {
        this->unk_148[i].v.cn[3] = ovl_Obj_Entotu_Vtx_000D10[i].v.cn[3] * this->unk_1B8.x;
    }

    if (this->unk_1B8.x > 0.0f) {
        Matrix_Translate(this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z, MTXMODE_NEW);
        this->actor.shape.rot.y = BINANG_ROT180(Camera_GetCamDirYaw(GET_ACTIVE_CAM(play)));
        Matrix_RotateYS(this->actor.shape.rot.y, MTXMODE_APPLY);
        Matrix_Scale(0.1f, 0.1f, 0.0f, MTXMODE_APPLY);

        OPEN_DISPS(play->state.gfxCtx);

        Gfx_SetupDL25_Opa(play->state.gfxCtx);

        // @recomp Tag the matrix, skipping rotation if the camera skipped interpolation this frame.
        if (camera_was_skipped()) {
            gEXMatrixGroupDecomposedSkipRot(POLY_XLU_DISP++, actor_transform_id(&this->actor), G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
        }
        else {
            gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, actor_transform_id(&this->actor), G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
        }

        gSPSegment(POLY_XLU_DISP++, 0x08,
                   Gfx_TwoTexScroll(play->state.gfxCtx, 0, 0, sp57, 0x20, 0x20, 1, 0, sp56, 0x20, 0x20));
        gSPSegment(POLY_XLU_DISP++, 0x09, Lib_SegmentedToVirtual(this->unk_148));
        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_XLU_DISP++, object_f53_obj_DL_001C00);
    
        // @recomp Pop the matrix group.
        gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);

        CLOSE_DISPS(play->state.gfxCtx);
    }
}

extern Gfx object_f53_obj_DL_000158[];

// @recomp Skip rotation interpolation for the Clock Town chimney when the camera skips interolation.
RECOMP_PATCH void func_80A34A44(ObjEntotu* this, PlayState* play) {
    Matrix_Translate(this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z, MTXMODE_NEW);
    this->actor.shape.rot.y = BINANG_ROT180(Camera_GetCamDirYaw(GET_ACTIVE_CAM(play)));
    Matrix_RotateYS(this->actor.shape.rot.y, MTXMODE_APPLY);
    Matrix_Scale(0.1f, 0.1f, 0.0f, MTXMODE_APPLY);

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    // @recomp Tag the matrix, skipping rotation if the camera skipped interpolation this frame.
    if (camera_was_skipped()) {
        gEXMatrixGroupDecomposedSkipRot(POLY_OPA_DISP++, actor_transform_id(&this->actor) + 1, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
    }
    else {
        gEXMatrixGroupDecomposedNormal(POLY_OPA_DISP++, actor_transform_id(&this->actor) + 1, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
    }

    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(POLY_OPA_DISP++, object_f53_obj_DL_000158);
    
    // @recomp Pop the matrix group.
    gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);

    CLOSE_DISPS(play->state.gfxCtx);
}

void Environment_DrawRainImpl(PlayState* play, View* view, GraphicsContext* gfxCtx);

// @recomp Skip interpolation for the splashes that raindrops make.
RECOMP_PATCH void Environment_DrawRain(PlayState* play, View* view, GraphicsContext* gfxCtx) {
    if (!(GET_ACTIVE_CAM(play)->stateFlags & CAM_STATE_UNDERWATER) &&
        (play->envCtx.precipitation[PRECIP_SNOW_CUR] == 0)) {
        
        // @recomp Open the displaylists and skip interpolation for rain.
        OPEN_DISPS(gfxCtx);
        gEXMatrixGroupNoInterpolation(POLY_XLU_DISP++, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);

        if (play->envCtx.precipitation[PRECIP_SOS_MAX] != 0) {
            if (play->envCtx.precipitation[PRECIP_SNOW_CUR] == 0) {
                Environment_DrawRainImpl(play, view, gfxCtx);
            }
        } else if (!(GET_ACTIVE_CAM(play)->stateFlags & CAM_STATE_UNDERWATER)) {
            if ((Environment_GetStormState(play) != STORM_STATE_OFF) &&
                (play->envCtx.precipitation[PRECIP_SNOW_CUR] == 0)) {
                Environment_DrawRainImpl(play, view, gfxCtx);
            }
        }

        // @recomp Pop the matrix group and close the displaylists.
        gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        CLOSE_DISPS(gfxCtx);
    }
}

// @recomp Skip interpolation on the boulders in the path to Snowhead and the path to Ikana Canyon when they teleport back to their home position.
RECOMP_PATCH void func_8093EE64(EnGoroiwa* this, s32 arg1) {
    Vec3s* temp_v0 = &this->pathPoints[arg1];

    this->actor.world.pos.x = temp_v0->x;
    this->actor.world.pos.y = temp_v0->y;
    this->actor.world.pos.z = temp_v0->z;

    // @recomp Skip interpolation if their next goal position is their home position.
    if (arg1 == 0) {
        actor_set_interpolation_skipped(&this->actor);
    }
}

extern Gfx object_twig_DL_001C38[];
extern Gfx object_twig_DL_0014C8[];

// @recomp Skip interpolation on the rotation for the beaver race rings in order to retain the intended animation look.
RECOMP_PATCH void EnTwig_Draw(Actor* thisx, PlayState* play) {
    EnTwig* this = (EnTwig*)thisx;

    switch (this->unk_160) {
        case 1:
            // @recomp Set the matrix group to skip interpolation on rotation.
            OPEN_DISPS(play->state.gfxCtx);
            gEXMatrixGroupDecomposedSkipRot(POLY_OPA_DISP++, actor_transform_id(thisx), G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
            Gfx_DrawDListOpa(play, object_twig_DL_001C38);

            // @recomp Pop the tag.
            gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);
            
            CLOSE_DISPS(play->state.gfxCtx);
            break;
        case 2:
            Gfx_DrawDListOpa(play, object_twig_DL_0014C8);
            break;
    }
}

extern Gfx gEffFire1DL[];

#define HONOTRAP_EXTRA_BYTE_1(flameGroup) (&(flameGroup)->flameList[0].isDrawn)[1]
#define HONOTRAP_EXTRA_BYTE_2(flameGroup) (&(flameGroup)->flameList[1].isDrawn)[1]

RECOMP_PATCH void EnHonotrap_FlameGroup(EnHonotrap* this, PlayState* play) {
    s32 i;
    EnHonotrapFlameGroup* flameGroup = &this->flameGroup;
    f32 var_fs0;
    f32 temp_fs0;
    f32 temp_fs1;
    f32 sp88;
    f32 sp84;
    f32 sp80;
    s32 flameScrollDisplacement;
    s32 sp78 = false;
    f32 var_fs0_2;
    Vec3f sp68;
    EnHonotrapFlameElement* flameElem;

    sp80 = fabsf(Math_CosS(Camera_GetCamDirPitch(GET_ACTIVE_CAM(play))));
    flameScrollDisplacement = (s32)(sp80 * -10.5f) - 10;
    Math_StepToF(&flameGroup->unk0, 1.0f, 0.05f);
    if (this->timer <= 40) {
        sp78 = Math_StepToF(&flameGroup->unk4, 1.0f, 0.05f);
    } else if (this->actor.parent == NULL) {
        this->timer = 40;
    }
    for (i = 0; i < ARRAY_COUNT(flameGroup->flameList); i++) {
        flameGroup->flameList[i].isDrawn = false;
    }

    sp88 = Math_SinS(this->actor.shape.rot.y) * 120.0f;
    sp84 = Math_CosS(this->actor.shape.rot.y) * 120.0f;

    flameGroup->unk8 += 0.050f * (1.0f - flameGroup->unk4);

    if (flameGroup->unk8 > 1.0f / 6) {
        flameGroup->unk8 -= 1.0f / 6;
        // @recomp Indicate that a cycle of the flame group has occurred.
        HONOTRAP_EXTRA_BYTE_1(flameGroup) = true;
        HONOTRAP_EXTRA_BYTE_2(flameGroup)++;
        if (HONOTRAP_EXTRA_BYTE_2(flameGroup) == ARRAY_COUNT(flameGroup->flameList)) {
            HONOTRAP_EXTRA_BYTE_2(flameGroup) = 0;
        }
    }
    else {
        HONOTRAP_EXTRA_BYTE_1(flameGroup) = false;
    }
    var_fs0 = flameGroup->unk4 + flameGroup->unk8;

    for (i = 0; i < ARRAY_COUNT(flameGroup->flameList) && (var_fs0 <= flameGroup->unk0); i++) {
        flameElem = &flameGroup->flameList[i];
        flameElem->isDrawn = true;

        flameElem->pos.x = (sp88 * var_fs0) + this->actor.world.pos.x;
        flameElem->pos.y = this->actor.world.pos.y;
        flameElem->pos.z = (sp84 * var_fs0) + this->actor.world.pos.z;

        flameElem->unkC = Math_SinS(TRUNCF_BINANG(var_fs0 * 25486.223f)) * 1.6f;
        if (flameElem->unkC > 1.0f) {
            flameElem->unkC = 1.0f;
        } else if (flameElem->unkC < 0.0f) {
            flameElem->unkC = 0.0f;
        }

        var_fs0 += 1.0f / 6;
        flameElem->unkC *= (0.006f * (((1.0f - flameGroup->unk4) * 0.8f) + 0.2f));
        flameElem->flameScroll += flameScrollDisplacement;
        flameElem->flameScroll %= 0x200U;
    }

    if (sp78 || (this->timer <= 0)) {
        Actor_Kill(&this->actor);
        return;
    }
    temp_fs0 = flameGroup->unk0 * 120.0f;
    temp_fs1 = flameGroup->unk4 * 120.0f;
    Actor_OffsetOfPointInActorCoords(&this->actor, &sp68, &GET_PLAYER(play)->actor.world.pos);

    if (sp68.z < temp_fs1) {
        sp68.z = temp_fs1;
    } else if (temp_fs0 < sp68.z) {
        sp68.z = temp_fs0;
    }

    var_fs0_2 = Math_SinS(TRUNCF_BINANG(sp68.z * 212.3852f)) * 1.6f;
    if (var_fs0_2 > 1.0f) {
        var_fs0_2 = 1.0f;
    }
    sp80 *= ((1.0f - flameGroup->unk4) * 0.8f) + 0.2f;
    if (sp80 > 0.2f) {
        this->collider.cyl.dim.pos.x =
            TRUNCF_BINANG((Math_SinS(this->actor.shape.rot.y) * sp68.z) + this->actor.world.pos.x);
        this->collider.cyl.dim.pos.y = TRUNCF_BINANG(this->actor.world.pos.y - (24.0f * var_fs0_2));
        this->collider.cyl.dim.pos.z =
            TRUNCF_BINANG((Math_CosS(this->actor.shape.rot.y) * sp68.z) + this->actor.world.pos.z);
        this->collider.cyl.dim.radius = TRUNCF_BINANG(15.0f * var_fs0_2);
        this->collider.cyl.dim.height = TRUNCF_BINANG(37.5f * var_fs0_2);
        CollisionCheck_SetAT(play, &play->colChkCtx, &this->collider.tris.base);
        CollisionCheck_SetAC(play, &play->colChkCtx, &this->collider.tris.base);
        CollisionCheck_SetOC(play, &play->colChkCtx, &this->collider.tris.base);
    }
}


// @recomp Patched to tag the flames that come out of fire eyes.
RECOMP_PATCH void EnHonotrap_DrawFlameGroup(Actor* thisx, PlayState* play) {
    s32 pad;
    EnHonotrap* this = (EnHonotrap*)thisx;
    EnHonotrapFlameElement* flameElem;
    EnHonotrapFlameGroup* flameGroup = &this->flameGroup;
    s32 i;
    s32 pad2;
    Vec3s camDir;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, 255, 200, 0, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 0, 0, 0);
    camDir = Camera_GetCamDir(GET_ACTIVE_CAM(play));
    camDir.y += 0x8000;

    // @recomp Get the transform ID for this actor.
    u32 base_transform_id = actor_transform_id(thisx);

    for (i = 0; i < ARRAY_COUNT(flameGroup->flameList); i++) {
        flameElem = &flameGroup->flameList[i];
        if (flameElem->isDrawn) {

            // @recomp Tag the current flame.
            s32 transform_id_offset = i - HONOTRAP_EXTRA_BYTE_2(flameGroup);
            if (transform_id_offset < 0) {
                transform_id_offset += ARRAY_COUNT(flameGroup->flameList);
            }
            u32 transform_id = base_transform_id + transform_id_offset;

            // @recomp Use the texscroll for the flame index based on the calculated transform offset to make it consistent in movement.
            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScroll(play->state.gfxCtx, 0, 0, 0, 32, 64, 1, 0, flameGroup->flameList[transform_id_offset].flameScroll, 32, 128));
            Matrix_SetTranslateRotateYXZ(flameElem->pos.x, flameElem->pos.y - (4000.0f * flameElem->unkC),
                                         flameElem->pos.z, &camDir);
            Matrix_Scale(((fabsf(Math_SinS((s16)(camDir.y - thisx->shape.rot.y) >> 1)) * 0.2f) + 1.7f) *
                             flameElem->unkC,
                         flameElem->unkC, 1.0f, MTXMODE_APPLY);
            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

            // @recomp Skip if a cycle occurred and this is the first flame.
            if (i == 0 && HONOTRAP_EXTRA_BYTE_1(flameGroup)) {
                gEXMatrixGroupDecomposedSkipAll(POLY_XLU_DISP++, transform_id, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
            }
            else {
                gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, transform_id, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
            }
            gSPDisplayList(POLY_XLU_DISP++, gEffFire1DL);
            // @recomp Pop the matrix group.
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

extern Gfx ovl_En_Tanron1_DL_001428[];
extern Gfx ovl_En_Tanron1_DL_001888[];
extern Gfx ovl_En_Tanron1_DL_001900[];

// @recomp Patched to interpolate the moths that circle torches.
RECOMP_PATCH void func_80BB5AAC(EnTanron1* this, PlayState* play) {
    EnTanron1Struct* ptrBase = &this->unk_160[0];
    s16 i;
    u8 flag = 0;
    EnTanron1Struct* ptr = ptrBase;

    // @recomp Get actor transform id
    u32 cur_transform_id = actor_transform_id(&this->actor);

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    for (i = 0; i < this->actor.params; i++, ptr++) {
        if (ptr->unk_24 == 1) {
            if (!flag) {
                gSPDisplayList(POLY_OPA_DISP++, ovl_En_Tanron1_DL_001888);
                flag++;
            }
            Matrix_Translate(ptr->unk_00.x, ptr->unk_00.y, ptr->unk_00.z, MTXMODE_NEW);
            Matrix_RotateYS(ptr->unk_1A, MTXMODE_APPLY);
            Matrix_RotateXS(ptr->unk_18 * -1, MTXMODE_APPLY);
            Matrix_Scale(1.2f, ptr->unk_2C, 1.2f, MTXMODE_APPLY);

            // @recomp Write matrix group for POLY_OPA
            gEXMatrixGroupDecomposedNormal(POLY_OPA_DISP++, cur_transform_id + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);

            gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_OPA_DISP++, ovl_En_Tanron1_DL_001900);

            // @recomp Pop matrix group
            gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);
        }
    }

    flag = 0;
    ptr = ptrBase;
    for (i = 0; i < this->actor.params; i++, ptr++) {
        if (ptr->unk_24 == 2) {
            if (!flag) {
                gSPDisplayList(POLY_OPA_DISP++, ovl_En_Tanron1_DL_001888);
                gDPLoadTextureBlock(POLY_OPA_DISP++, ovl_En_Tanron1_DL_001428, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 32, 0,
                                    G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, 4, 5, G_TX_NOLOD,
                                    G_TX_NOLOD);
                flag++;
            }

            Matrix_Translate(ptr->unk_00.x, ptr->unk_00.y, ptr->unk_00.z, MTXMODE_NEW);
            Matrix_RotateYS(ptr->unk_1A, MTXMODE_APPLY);
            Matrix_RotateXS(ptr->unk_18 * -1, MTXMODE_APPLY);
            Matrix_Scale(1.0f, ptr->unk_2C, 1.0f, MTXMODE_APPLY);

            // @recomp Write matrix group for POLY_OPA
            gEXMatrixGroupDecomposedNormal(POLY_OPA_DISP++, cur_transform_id + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);

            gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_OPA_DISP++, ovl_En_Tanron1_DL_001900);

            // @recomp Pop matrix group
            gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx);
}
