#include "patches.h"
#include "transform_ids.h"
#include "overlays/actors/ovl_En_Tanron2/z_en_tanron2.h"
#include "overlays/actors/ovl_Boss_03/z_boss_03.h"
#include "overlays/actors/ovl_Boss_04/z_boss_04.h"
#include "overlays/actors/ovl_Boss_Hakugin/z_boss_hakugin.h"
#include "overlays/actors/ovl_En_Water_Effect/z_en_water_effect.h"

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

// @recomp Track this effect's reset state.
void Boss03_SpawnEffectWetSpot(PlayState* play, Vec3f* pos) {
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
void Boss03_SpawnEffectDroplet(PlayState* play, Vec3f* pos) {
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
void Boss03_SpawnEffectSplash(PlayState* play, Vec3f* pos, Vec3f* velocity) {
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
void Boss03_SpawnEffectBubble(PlayState* play, Vec3f* pos) {
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
void Boss03_DrawEffects(PlayState* play) {
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
                gEXMatrixGroupDecomposed(POLY_OPA_DISP++, SPECIAL_EFFECTS_TRANSFORM_ID_START + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_ORDER_LINEAR);
            }
            else {
                gEXMatrixGroupDecomposed(POLY_OPA_DISP++, SPECIAL_EFFECTS_TRANSFORM_ID_START + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);
            }
            special_effect_reset_states[i] = false;
            
            gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_OPA_DISP++, gGyorgBubbleModelDL);

            // @recomp Pop the effect's matrix group.
            gEXPopMatrixGroup(POLY_OPA_DISP++);
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
                gEXMatrixGroupDecomposed(POLY_XLU_DISP++, SPECIAL_EFFECTS_TRANSFORM_ID_START + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_ORDER_LINEAR);
            }
            else {
                gEXMatrixGroupDecomposed(POLY_XLU_DISP++, SPECIAL_EFFECTS_TRANSFORM_ID_START + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);
            }
            special_effect_reset_states[i] = false;

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_0042B0);

            // @recomp Pop the effect's matrix group.
            gEXPopMatrixGroup(POLY_XLU_DISP++);
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
                gEXMatrixGroupDecomposed(POLY_XLU_DISP++, SPECIAL_EFFECTS_TRANSFORM_ID_START + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_ORDER_LINEAR);
            }
            else {
                gEXMatrixGroupDecomposed(POLY_XLU_DISP++, SPECIAL_EFFECTS_TRANSFORM_ID_START + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);
            }
            special_effect_reset_states[i] = false;

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_0042F8);

            // @recomp Pop the effect's matrix group.
            gEXPopMatrixGroup(POLY_XLU_DISP++);
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

void func_80A5A6B8(Actor* thisx, PlayState* play2) {
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
            gEXMatrixGroupDecomposed(POLY_XLU_DISP++, actor_id + 0,
                G_EX_PUSH, G_MTX_MODELVIEW,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, (u8)this->unk_E2C);
            gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_000420);

            // @recomp Pop the transform id.
            gEXPopMatrixGroup(POLY_XLU_DISP++);
        }

        Matrix_Pop();

        if (this->unk_E30 > 1.0f) {
            Gfx_SetupDL25_Xlu(play->state.gfxCtx);
            AnimatedMat_Draw(play, Lib_SegmentedToVirtual(object_water_effect_Matanimheader_000E0C));
            Matrix_Scale(this->unk_DC8[2].y, this->unk_DC8[2].z, this->unk_DC8[2].y, MTXMODE_APPLY);

            // @recomp Tag the transform.
            gEXMatrixGroupDecomposed(POLY_XLU_DISP++, actor_id + 1,
                G_EX_PUSH, G_MTX_MODELVIEW,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, (u8)this->unk_E30);
            gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_000730);

            // @recomp Pop the transform id.
            gEXPopMatrixGroup(POLY_XLU_DISP++);
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
        gEXMatrixGroupDecomposed(POLY_XLU_DISP++, actor_id + 2,
            G_EX_PUSH, G_MTX_MODELVIEW,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);

        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, (u8)this->unk_E34);
        gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_000A48);

        // @recomp Pop the transform id.
        gEXPopMatrixGroup(POLY_XLU_DISP++);
    }

    Matrix_Pop();

    if ((this->actor.params == ENWATEREFFECT_TYPE_GYORG_RIPPLES) ||
        (this->actor.params == ENWATEREFFECT_TYPE_GYORG_SHOCKWAVE)) {
        Gfx_SetupDL25_Xlu(play->state.gfxCtx);
        AnimatedMat_Draw(play, Lib_SegmentedToVirtual(object_water_effect_Matanimheader_000E58));
        Matrix_Scale(this->unk_DC8[4].y, this->unk_DC8[4].z, this->unk_DC8[4].y, MTXMODE_APPLY);

        // @recomp Tag the transform.
        gEXMatrixGroupDecomposed(POLY_XLU_DISP++, actor_id + 3,
            G_EX_PUSH, G_MTX_MODELVIEW,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);

        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, (u8)this->unk_E38);
        gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_000CD8);

        // @recomp Pop the transform id.
        gEXPopMatrixGroup(POLY_XLU_DISP++);
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
                gEXMatrixGroupDecomposed(POLY_XLU_DISP++, actor_id + 4 + i,
                    G_EX_PUSH, G_MTX_MODELVIEW,
                    G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                    G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);

                gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx),
                          G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_0042F8);

                // @recomp Pop the transform id.
                gEXPopMatrixGroup(POLY_XLU_DISP++);
            }
        }
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

// @recomp Tag normal water effects.
void EnWaterEffect_Draw(Actor* thisx, PlayState* play2) {
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
            gEXMatrixGroupDecomposed(POLY_XLU_DISP++, actor_id + i + 0 * (ARRAY_COUNT(this->unk_144) / 2),
                G_EX_PUSH, G_MTX_MODELVIEW,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_0042B0);

            // @recomp Pop the transform id.
            gEXPopMatrixGroup(POLY_XLU_DISP++);
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
            gEXMatrixGroupDecomposed(POLY_XLU_DISP++, actor_id + i + 1 * (ARRAY_COUNT(this->unk_144) / 2),
                G_EX_PUSH, G_MTX_MODELVIEW,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_0042F8);

            // @recomp Pop the transform id.
            gEXPopMatrixGroup(POLY_XLU_DISP++);
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
void func_80B0C398(BossHakugin* this, PlayState* play) {
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
            gEXMatrixGroupDecomposed(POLY_OPA_DISP++, GOHT_ROCKS_TRANSFORM_ID_START + i + 0 * ARRAY_COUNT(this->unk_9F8),
                G_EX_PUSH, G_MTX_MODELVIEW,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);

            gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_OPA_DISP++, gGohtRockModelDL);

            // @recomp Pop the transform id.
            gEXPopMatrixGroup(POLY_OPA_DISP++);
        }
    }

    gSPDisplayList(POLY_OPA_DISP++, gGohtStalactiteMaterialDL);
    for (i = 0; i < ARRAY_COUNT(this->unk_9F8); i++) {
        effect = &this->unk_9F8[i];
        if ((effect->unk_18 >= 0) && (effect->unk_1A == 1)) {
            Matrix_SetTranslateRotateYXZ(effect->unk_0.x, effect->unk_0.y, effect->unk_0.z, &effect->unk_1C);
            Matrix_Scale(effect->unk_24, effect->unk_24, effect->unk_24, MTXMODE_APPLY);

            // @recomp Tag the transform.
            gEXMatrixGroupDecomposed(POLY_OPA_DISP++, GOHT_ROCKS_TRANSFORM_ID_START + i + 1 * ARRAY_COUNT(this->unk_9F8),
                G_EX_PUSH, G_MTX_MODELVIEW,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);

            gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_OPA_DISP++, gGohtStalactiteModelDL);

            // @recomp Pop the transform id.
            gEXPopMatrixGroup(POLY_OPA_DISP++);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx);
}
