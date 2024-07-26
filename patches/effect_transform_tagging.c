#include "patches.h"
#include "transform_ids.h"
#include "overlays/actors/ovl_Object_Kankyo/z_object_kankyo.h"
#include "overlays/actors/ovl_En_Clear_Tag/z_en_clear_tag.h"
#include "z64effect.h"

extern f32 D_808DE5B0;
extern Gfx gEffDust5Tex[];
extern Gfx gEffDustDL[];

// Use an unused byte in the particle's struct to track whether its interpolation should be skipped this frame.
#define particle_skipped(particle) \
    ((&(particle).unk_1C)[1])

// @recomp Patched to record when a particle is moved to skip interpolation.
RECOMP_PATCH void func_808DC454(ObjectKankyo* this, PlayState* play) {
    s16 i;
    s32 pad1;
    f32 phi_f20;
    f32 spD0;
    f32 spCC;
    f32 spC8;
    f32 spC4;
    f32 spC0;
    f32 spBC;
    f32 temp_f0_4;
    f32 temp_f22;
    f32 temp_f24;
    f32 temp_f28;
    f32 x = play->view.at.x - play->view.eye.x;
    f32 y = play->view.at.y - play->view.eye.y;
    f32 z = play->view.at.z - play->view.eye.z;
    f32 magnitude = sqrtf(SQ(x) + SQ(y) + SQ(z));
    f32 temp_120 = 120.0f;
    f32 temp_f30;
    Vec3f sp88;
    s32 pad;

    spD0 = x / magnitude;
    spCC = y / magnitude;
    spC8 = z / magnitude;

    for (i = 0; i < play->envCtx.precipitation[PRECIP_SNOW_CUR]; i++) {
        switch (this->unk_14C[i].unk_1C) {
            case 0:
                this->unk_14C[i].unk_00 = play->view.eye.x + (spD0 * 120.0f);
                this->unk_14C[i].unk_04 = play->view.eye.y + (spCC * 120.0f);
                this->unk_14C[i].unk_08 = play->view.eye.z + (spC8 * 120.0f);
                this->unk_14C[i].unk_0C = (Rand_ZeroOne() - 0.5f) * (2.0f * temp_120);

                temp_f22 = (Camera_GetCamDirPitch(GET_ACTIVE_CAM(play)) * 0.004f) + 60.0f;
                if (temp_f22 < 20.0f) {
                    temp_f22 = 20.0f;
                }

                if (this->unk_114E == 0) {
                    this->unk_14C[i].unk_10 = temp_f22;
                } else {
                    this->unk_14C[i].unk_10 += temp_f22;
                    if (play->envCtx.precipitation[PRECIP_SNOW_CUR] == ((u32)i + 1)) {
                        this->unk_114E = 0;
                    }
                }

                this->unk_14C[i].unk_14 = (Rand_ZeroOne() - 0.5f) * (2.0f * temp_120);
                if (play->envCtx.precipitation[PRECIP_SOS_MAX] == 0) {
                    this->unk_14C[i].unk_18 = (Rand_ZeroOne() * 3.0f) + 1.0f;
                } else {
                    this->unk_14C[i].unk_18 = (Rand_ZeroOne() * 3.0f) + 8.0f;
                }
                particle_skipped(this->unk_14C[i]) = true;
                this->unk_14C[i].unk_1C++;
                break;

            case 1:
                temp_f24 = play->view.eye.x + (spD0 * 120.0f);
                temp_f28 = play->view.eye.y + (spCC * 120.0f);
                temp_f30 = play->view.eye.z + (spC8 * 120.0f);

                magnitude = sqrtf((f32)SQ(play->envCtx.windDirection.x) + SQ(play->envCtx.windDirection.y) +
                                  SQ(play->envCtx.windDirection.z));
                if (magnitude == 0.0f) {
                    magnitude = 0.001f;
                }
                spC4 = -play->envCtx.windDirection.x / magnitude;
                spC0 = -play->envCtx.windDirection.y / magnitude;
                spBC = -play->envCtx.windDirection.z / magnitude;

                if (i == 0) {
                    this->unk_144 += 0.049999997f * Rand_ZeroOne();
                    this->unk_148 += 0.049999997f * Rand_ZeroOne();
                }

                phi_f20 = play->envCtx.windSpeed / 120.0f;
                phi_f20 = CLAMP(phi_f20, 0.0f, 1.0f);

                this->unk_14C[i].unk_0C += sinf((this->unk_144 + (i * 100.0f)) * 0.01f) + (spC4 * 10.0f * phi_f20);
                this->unk_14C[i].unk_14 += cosf((this->unk_148 + (i * 100.0f)) * 0.01f) + (spBC * 10.0f * phi_f20);
                this->unk_14C[i].unk_10 -= this->unk_14C[i].unk_18 - (spC0 * 3.0f * (play->envCtx.windSpeed / 100.0f));

                temp_f22 = (-Camera_GetCamDirPitch(GET_ACTIVE_CAM(play)) * 0.012f) + 40.0f;
                if (temp_f22 < -40.0f) {
                    temp_f22 = -40.0f;
                }

                if (((this->unk_14C[i].unk_00 + this->unk_14C[i].unk_0C) - temp_f24) > temp_120) {
                    this->unk_14C[i].unk_00 = temp_f24 - temp_120;
                    // @recomp Skip interpolation.
                    particle_skipped(this->unk_14C[i]) = true;
                }

                if (((this->unk_14C[i].unk_00 + this->unk_14C[i].unk_0C) - temp_f24) < -temp_120) {
                    this->unk_14C[i].unk_00 = temp_f24 + temp_120;
                    // @recomp Skip interpolation.
                    particle_skipped(this->unk_14C[i]) = true;
                }

                sp88.x = this->unk_14C[i].unk_00 + this->unk_14C[i].unk_0C;
                sp88.y = this->unk_14C[i].unk_04 + this->unk_14C[i].unk_10;
                sp88.z = this->unk_14C[i].unk_08 + this->unk_14C[i].unk_14;

                phi_f20 = Math_Vec3f_DistXZ(&sp88, &play->view.eye) / 200.0f;
                phi_f20 = CLAMP(phi_f20, 0.0f, 1.0f);
                temp_f0_4 = 100.0f + phi_f20 + 60.0f;

                if (temp_f0_4 < (this->unk_14C[i].unk_04 + (this->unk_14C[i].unk_10) - temp_f28)) {
                    this->unk_14C[i].unk_04 = temp_f28 - temp_f0_4;
                    // @recomp Skip interpolation.
                    particle_skipped(this->unk_14C[i]) = true;
                }

                if (((this->unk_14C[i].unk_04 + this->unk_14C[i].unk_10) - temp_f28) < -temp_f0_4) {
                    this->unk_14C[i].unk_04 = temp_f28 + temp_f0_4;
                    // @recomp Skip interpolation.
                    particle_skipped(this->unk_14C[i]) = true;
                }

                if (((this->unk_14C[i].unk_08 + this->unk_14C[i].unk_14) - temp_f30) > temp_120) {
                    this->unk_14C[i].unk_08 = temp_f30 - temp_120;
                    // @recomp Skip interpolation.
                    particle_skipped(this->unk_14C[i]) = true;
                }

                if (((this->unk_14C[i].unk_08 + this->unk_14C[i].unk_14) - temp_f30) < -temp_120) {
                    this->unk_14C[i].unk_08 = temp_f30 + temp_120;
                    // @recomp Skip interpolation.
                    particle_skipped(this->unk_14C[i]) = true;
                }

                if ((this->unk_14C[i].unk_04 + this->unk_14C[i].unk_10) < ((play->view.eye.y - temp_f22) - 40.0f)) {
                    this->unk_14C[i].unk_1C = 0;
                }
                break;
        }
    }
}

RECOMP_PATCH void func_808DD3C8(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    ObjectKankyo* this = (ObjectKankyo*)thisx;
    Vec3f worldPos;
    Vec3f screenPos;
    s16 i;
    u8 pad2;
    u8 spB4;
    f32 temp_f0;
    u8 sp68;
    s32 pad;
    f32 temp_f2;
    f32 tempf;

    if ((play->cameraPtrs[CAM_ID_MAIN]->stateFlags & CAM_STATE_UNDERWATER) || ((u8)play->envCtx.stormState == 0)) {
        return;
    }

    OPEN_DISPS(play->state.gfxCtx);

    spB4 = false;

    if (this->actor.params == 3) {
        temp_f0 = func_80173B48(&play->state) / 1.4e7f;
        temp_f0 = CLAMP(temp_f0, 0.0f, 1.0f);
        Math_SmoothStepToF(&D_808DE5B0, temp_f0, 0.2f, 0.1f, 0.001f);

        sp68 = play->envCtx.precipitation[PRECIP_SNOW_CUR];
        sp68 *= D_808DE5B0;

        if ((play->envCtx.precipitation[PRECIP_SNOW_CUR] >= 32) && (sp68 < 32)) {
            sp68 = 32;
        }
    } else {
        sp68 = play->envCtx.precipitation[PRECIP_SNOW_CUR];
    }

    for (i = 0; i < sp68; i++) {
        worldPos.x = this->unk_14C[i].unk_00 + this->unk_14C[i].unk_0C;
        worldPos.y = this->unk_14C[i].unk_04 + this->unk_14C[i].unk_10;
        worldPos.z = this->unk_14C[i].unk_08 + this->unk_14C[i].unk_14;

        Play_GetScreenPos(play, &worldPos, &screenPos);

        // @recomp Render particles beyond the screen bounds.
        if (true) {
        // if ((screenPos.x >= 0.0f) && (screenPos.x < SCREEN_WIDTH) && (screenPos.y >= 0.0f) &&
        //     (screenPos.y < SCREEN_HEIGHT)) {
            if (!spB4) {
                spB4 = true;

                gDPPipeSync(POLY_XLU_DISP++);
                gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
                gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING);

                POLY_XLU_DISP = Gfx_SetupDL(POLY_XLU_DISP, SETUPDL_0);

                gDPSetRenderMode(POLY_XLU_DISP++, G_RM_FOG_SHADE_A, G_RM_ZB_CLD_SURF2);
                gSPSetGeometryMode(POLY_XLU_DISP++, G_FOG);
                gSPSegment(POLY_XLU_DISP++, 0x08, Lib_SegmentedToVirtual(gEffDust5Tex));
            }

            Matrix_Translate(worldPos.x, worldPos.y, worldPos.z, MTXMODE_NEW);
            tempf = (i & 7) * 0.008f;
            Matrix_Scale(0.05f + tempf, 0.05f + tempf, 0.05f + tempf, MTXMODE_APPLY);
            temp_f2 = Math_Vec3f_DistXYZ(&worldPos, &play->view.eye) / 300.0f;
            temp_f2 = ((1.0f < temp_f2) ? 0.0f : (((1.0f - temp_f2) > 1.0f) ? 1.0f : 1.0f - temp_f2));

            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, (u8)(160.0f * temp_f2));

            Matrix_Mult(&play->billboardMtxF, MTXMODE_APPLY);

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

            // @recomp Check if the particle's interpolation should be skipped this frame.
            if (particle_skipped(this->unk_14C[i])) {
                // @recomp Tag the particle's transform to skip interpolation.
                gEXMatrixGroupDecomposedSkipPosRot(POLY_XLU_DISP++, actor_transform_id(thisx) + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
            }
            else {
                // @recomp Tag the particle's matrix to interpolate normally.
                gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, actor_transform_id(thisx) + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
            }
            
            gSPDisplayList(POLY_XLU_DISP++, gEffDustDL);

            // @recomp Pop the particle transform tag.
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        }

        // @recomp Reset the particle's skip flag.
        particle_skipped(this->unk_14C[i]) = false;
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

#define SPARK_COUNT 3
#define BLURE_COUNT 25
#define SHIELD_PARTICLE_COUNT 3
#define TIRE_MARK_COUNT 15

#define TOTAL_EFFECT_COUNT SPARK_COUNT + BLURE_COUNT + SHIELD_PARTICLE_COUNT + TIRE_MARK_COUNT

typedef struct EffectStatus {
    /* 0x0 */ u8 active;
    /* 0x1 */ u8 unk1;
    /* 0x2 */ u8 unk2;
} EffectStatus; // size = 0x3

typedef struct EffectContext {
    /* 0x0000 */ struct PlayState* play;
    struct {
        EffectStatus status;
        EffectSpark effect;
    } /* 0x0004 */ sparks[SPARK_COUNT];
    struct {
        EffectStatus status;
        EffectBlure effect;
    } /* 0x0E5C */ blures[BLURE_COUNT];
    struct {
        EffectStatus status;
        EffectShieldParticle effect;
    } /* 0x388C */ shieldParticles[SHIELD_PARTICLE_COUNT];
    struct {
        EffectStatus status;
        EffectTireMark effect;
    } /* 0x3DF0 */ tireMarks[TIRE_MARK_COUNT];
} EffectContext; // size = 0x98E0

typedef struct EffectInfo {
    /* 0x00 */ u32 size;
    /* 0x04 */ void (*init)(void* effect, void* initParams);
    /* 0x08 */ void (*destroy)(void* effect);
    /* 0x0C */ s32 (*update)(void* effect);
    /* 0x10 */ void (*draw)(void* effect, struct GraphicsContext* gfxCtx);
} EffectInfo; // size = 0x14

extern EffectContext sEffectContext;
extern EffectInfo sEffectInfoTable[];

static inline void tag_interpolate_effect(GraphicsContext* gfxCtx, u32 id) {
    OPEN_DISPS(gfxCtx);

    gEXMatrixGroupDecomposedNormal(POLY_OPA_DISP++, id, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
    
    gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, id + EFFECT_TRANSFORM_ID_COUNT, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);

    CLOSE_DISPS();
}

static inline void tag_skip_effect(GraphicsContext* gfxCtx, u32 id) {
    OPEN_DISPS(gfxCtx);

    gEXMatrixGroupSimple(POLY_OPA_DISP++, id, G_EX_PUSH, G_MTX_MODELVIEW,
        G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP,
        G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);
    
    gEXMatrixGroupSimple(POLY_XLU_DISP++, id + EFFECT_TRANSFORM_ID_COUNT, G_EX_PUSH, G_MTX_MODELVIEW,
        G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP,
        G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);

    CLOSE_DISPS();
}

static inline void pop_effect_tag(GraphicsContext* gfxCtx) {
    OPEN_DISPS(gfxCtx);

    gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);
    gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);

    CLOSE_DISPS();
}

// @recomp Patched to tag effects.
RECOMP_PATCH void Effect_DrawAll(GraphicsContext* gfxCtx) {
    s32 i;


    for (i = 0; i < SPARK_COUNT; i++) {
        if (!sEffectContext.sparks[i].status.active) {
            continue;
        }
        // @recomp Tag transform.
        tag_interpolate_effect(gfxCtx, EFFECT_SPARK_TRANSFORM_ID_START + i);

        sEffectInfoTable[EFFECT_SPARK].draw(&sEffectContext.sparks[i].effect, gfxCtx);

        // @recomp Pop tag.
        pop_effect_tag(gfxCtx);
    }

    for (i = 0; i < BLURE_COUNT; i++) {
        if (!sEffectContext.blures[i].status.active) {
            continue;
        }
        // @recomp Tag transform to skip interpolation as this effect doesn't work well with it.
        tag_skip_effect(gfxCtx, EFFECT_BLURE_TRANSFORM_ID_START + i);

        sEffectInfoTable[EFFECT_BLURE1].draw(&sEffectContext.blures[i].effect, gfxCtx);

        // @recomp Pop tag.
        pop_effect_tag(gfxCtx);
    }

    for (i = 0; i < SHIELD_PARTICLE_COUNT; i++) {
        if (!sEffectContext.shieldParticles[i].status.active) {
            continue;
        }
        // @recomp Tag transform.
        tag_interpolate_effect(gfxCtx, EFFECT_SHIELD_PARTICLE_TRANSFORM_ID_START + i);

        sEffectInfoTable[EFFECT_SHIELD_PARTICLE].draw(&sEffectContext.shieldParticles[i].effect, gfxCtx);

        // @recomp Pop tag.
        pop_effect_tag(gfxCtx);
    }

    for (i = 0; i < TIRE_MARK_COUNT; i++) {
        if (!sEffectContext.tireMarks[i].status.active) {
            continue;
        }
        // @recomp Tag transform to skip interpolation as this effect doesn't work well with it.
        tag_skip_effect(gfxCtx, EFFECT_TIRE_MARK_TRANSFORM_ID_START + i);

        sEffectInfoTable[EFFECT_TIRE_MARK].draw(&sEffectContext.tireMarks[i].effect, gfxCtx);

        // @recomp Pop tag.
        pop_effect_tag(gfxCtx);
    }
}

typedef enum {
    /* 0x00 */ CLEAR_TAG_EFFECT_AVAILABLE,
    /* 0x01 */ CLEAR_TAG_EFFECT_DEBRIS,
    /* 0x02 */ CLEAR_TAG_EFFECT_FIRE, // never set to, remnant of OoT
    /* 0x03 */ CLEAR_TAG_EFFECT_SMOKE,
    /* 0x04 */ CLEAR_TAG_EFFECT_FLASH,
    /* 0x05 */ CLEAR_TAG_EFFECT_LIGHT_RAYS,
    /* 0x06 */ CLEAR_TAG_EFFECT_SHOCKWAVE,
    /* 0x07 */ CLEAR_TAG_EFFECT_SPLASH,
    /* 0x08 */ CLEAR_TAG_EFFECT_ISOLATED_SMOKE
} ClearTagEffectType;

extern Gfx gClearTagDebrisEffectMaterialDL[];
extern Gfx gClearTagDebrisEffectDL[];
extern Gfx gEffShockwaveDL[];
extern Gfx gClearTagFlashEffectGroundDL[];
extern Gfx gClearTagFireEffectMaterialDL[];
extern Gfx gClearTagFireEffectDL[];
extern Gfx gClearTagFireEffectMaterialDL[];
extern Gfx gClearTagFireEffectDL[];
extern Gfx gClearTagFlashEffectDL[];
extern Gfx gClearTagLightRayEffectMaterialDL[];
extern Gfx gClearTagLightRayEffectDL[];
extern Gfx gEffWaterSplashDL[];

extern u64 gEffWaterSplash1Tex[];
extern u64 gEffWaterSplash2Tex[];
extern u64 gEffWaterSplash3Tex[];
extern u64 gEffWaterSplash4Tex[];
extern u64 gEffWaterSplash5Tex[];
extern u64 gEffWaterSplash6Tex[];
extern u64 gEffWaterSplash7Tex[];
extern u64 gEffWaterSplash8Tex[];

static TexturePtr sWaterSplashTextures[] = {
    gEffWaterSplash1Tex,
    gEffWaterSplash2Tex,
    gEffWaterSplash3Tex,
    gEffWaterSplash4Tex,
    gEffWaterSplash5Tex,
    gEffWaterSplash6Tex,
    gEffWaterSplash7Tex,
    gEffWaterSplash8Tex,
    NULL,
    NULL,
    NULL,
};

/**
 * Draws all effects.
 * Each effect type is drawn before the next. The function will apply a material that
 * applies to all effects of that type while drawing the first effect of that type.
 */
// @recomp Patched to tag matrices.
RECOMP_PATCH void EnClearTag_DrawEffects(Actor* thisx, PlayState* play) {
    u8 isMaterialApplied = false;
    s16 i;
    s16 j;
    Vec3f vec;
    WaterBox* waterBox;
    f32 ySurface;
    MtxF mtxF;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    EnClearTag* this = (EnClearTag*)thisx;
    EnClearTagEffect* effect = this->effect;
    EnClearTagEffect* firstEffect = this->effect;

    OPEN_DISPS(gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    // Draw all Debris effects.
    for (i = 0; i < ARRAY_COUNT(this->effect); i++, effect++) {
        if (effect->type == CLEAR_TAG_EFFECT_DEBRIS) {
            // Apply the debris effect material if it has not already been applied.
            if (!isMaterialApplied) {
                isMaterialApplied++;
                gSPDisplayList(POLY_OPA_DISP++, gClearTagDebrisEffectMaterialDL);
            }

            // Draw the debris effect.
            Matrix_Translate(effect->pos.x, effect->pos.y, effect->pos.z, MTXMODE_NEW);
            Matrix_Scale(effect->scale, effect->scale, effect->scale, MTXMODE_APPLY);
            Matrix_RotateYF(effect->rotationY, MTXMODE_APPLY);
            Matrix_RotateXFApply(effect->rotationX);
            gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            // @recomp Tag the matrix.
            gEXMatrixGroupDecomposedNormal(POLY_OPA_DISP++, actor_transform_id(thisx) + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
            gSPDisplayList(POLY_OPA_DISP++, gClearTagDebrisEffectDL);
            // @recomp Pop the matrix tag.
            gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);
        }
    }

    // Draw all Shockwave effects.
    effect = firstEffect;
    if (this->actor.floorPoly != NULL) {
        for (i = 0; i < ARRAY_COUNT(this->effect); i++, effect++) {
            if (effect->type == CLEAR_TAG_EFFECT_SHOCKWAVE) {
                // Draw the shockwave effect.
                gDPPipeSync(POLY_XLU_DISP++);
                gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, (s8)effect->primColor.a);
                gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, (s8)effect->primColor.a);
                func_800C0094(this->actor.floorPoly, effect->pos.x, effect->pos.y, effect->pos.z, &mtxF);
                Matrix_Put(&mtxF);
                Matrix_Scale(effect->scale, 1.0f, effect->scale, MTXMODE_APPLY);
                gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                // @recomp Tag the matrix.
                gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, actor_transform_id(thisx) + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
                gSPDisplayList(POLY_XLU_DISP++, gEffShockwaveDL);
                // @recomp Pop the matrix tag.
                gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
            }
        }
    }

    // Draw all ground flash effects.
    effect = firstEffect;
    isMaterialApplied = false;
    if (this->actor.floorPoly != NULL) {
        for (i = 0; i < ARRAY_COUNT(this->effect); i++, effect++) {
            if (effect->type == CLEAR_TAG_EFFECT_FLASH) {
                // Apply the flash ground effect material if it has not already been applied.
                if (!isMaterialApplied) {
                    gDPPipeSync(POLY_XLU_DISP++);
                    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 200, 0);
                    isMaterialApplied++;
                }

                // Draw the ground flash effect.
                gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 200, (s8)(effect->primColor.a * 0.7f));
                func_800C0094(this->actor.floorPoly, effect->pos.x, this->actor.floorHeight, effect->pos.z, &mtxF);
                Matrix_Put(&mtxF);
                Matrix_Scale(effect->scale * 3.0f, 1.0f, effect->scale * 3.0f, MTXMODE_APPLY);
                gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                // @recomp Tag the matrix.
                gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, actor_transform_id(thisx) + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
                gSPDisplayList(POLY_XLU_DISP++, gClearTagFlashEffectGroundDL);
                // @recomp Pop the matrix tag.
                gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
            }
        }
    }

    // Draw all smoke effects.
    effect = firstEffect;
    isMaterialApplied = false;
    for (i = 0; i < ARRAY_COUNT(this->effect); i++, effect++) {
        if ((effect->type == CLEAR_TAG_EFFECT_SMOKE) || (effect->type == CLEAR_TAG_EFFECT_ISOLATED_SMOKE)) {
            // Apply the smoke effect material if it has not already been applied.
            if (!isMaterialApplied) {
                gSPDisplayList(POLY_XLU_DISP++, gClearTagFireEffectMaterialDL);
                isMaterialApplied++;
            }

            // Draw the smoke effect.
            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetEnvColor(POLY_XLU_DISP++, (s8)effect->envColor.r, (s8)effect->envColor.g, (s8)effect->envColor.b,
                           128);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, (s8)effect->primColor.r, (s8)effect->primColor.g,
                            (s8)effect->primColor.b, (s8)effect->primColor.a);
            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScroll(play->state.gfxCtx, 0, 0, -effect->actionTimer * 5, 32, 64, 1, 0, 0, 32, 32));
            Matrix_Translate(effect->pos.x, effect->pos.y, effect->pos.z, MTXMODE_NEW);
            Matrix_ReplaceRotation(&play->billboardMtxF);
            Matrix_Scale(effect->smokeScaleX * effect->scale, effect->smokeScaleY * effect->scale, 1.0f, MTXMODE_APPLY);
            Matrix_Translate(0.0f, 20.0f, 0.0f, MTXMODE_APPLY);
            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            // @recomp Tag the matrix.
            gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, actor_transform_id(thisx) + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
            gSPDisplayList(POLY_XLU_DISP++, gClearTagFireEffectDL);
            // @recomp Pop the matrix tag.
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        }
    }

    // Draw all fire effects.
    effect = firstEffect;
    isMaterialApplied = false;
    for (i = 0; i < ARRAY_COUNT(this->effect); i++, effect++) {
        if (effect->type == CLEAR_TAG_EFFECT_FIRE) {
            // Apply the fire effect material if it has not already been applied.
            if (!isMaterialApplied) {
                gSPDisplayList(POLY_XLU_DISP++, gClearTagFireEffectMaterialDL);
                gDPSetEnvColor(POLY_XLU_DISP++, 255, 215, 255, 128);
                isMaterialApplied++;
            }

            // Draw the fire effect.
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 200, 20, 0, (s8)effect->primColor.a);
            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScroll(play->state.gfxCtx, 0, 0, -effect->actionTimer * 15, 32, 64, 1, 0, 0, 32, 32));
            Matrix_Translate(effect->pos.x, effect->pos.y, effect->pos.z, MTXMODE_NEW);
            Matrix_ReplaceRotation(&play->billboardMtxF);
            Matrix_Scale(effect->scale, effect->scale, 1.0f, MTXMODE_APPLY);
            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            // @recomp Tag the matrix.
            gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, actor_transform_id(thisx) + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
            gSPDisplayList(POLY_XLU_DISP++, gClearTagFireEffectDL);
            // @recomp Pop the matrix tag.
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        }
    }

    // Draw all flash billboard effects.
    effect = firstEffect;
    isMaterialApplied = false;
    for (i = 0; i < ARRAY_COUNT(this->effect); i++, effect++) {
        if (effect->type == CLEAR_TAG_EFFECT_FLASH) {
            // Apply the flash billboard effect material if it has not already been applied.
            if (!isMaterialApplied) {
                gDPPipeSync(POLY_XLU_DISP++);
                gDPSetEnvColor(POLY_XLU_DISP++, this->flashEnvColor.r, this->flashEnvColor.g, this->flashEnvColor.b, 0);
                isMaterialApplied++;
            }

            // Draw the flash billboard effect.
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 200, (s8)effect->primColor.a);
            Matrix_Translate(effect->pos.x, effect->pos.y, effect->pos.z, MTXMODE_NEW);
            Matrix_ReplaceRotation(&play->billboardMtxF);
            Matrix_Scale(2.0f * effect->scale, 2.0f * effect->scale, 1.0f, MTXMODE_APPLY);
            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            // @recomp Tag the matrix.
            gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, actor_transform_id(thisx) + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
            gSPDisplayList(POLY_XLU_DISP++, gClearTagFlashEffectDL);
            // @recomp Pop the matrix tag.
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        }
    }

    // Draw all light ray effects.
    effect = firstEffect;
    isMaterialApplied = false;
    for (i = 0; i < ARRAY_COUNT(this->effect); i++, effect++) {
        if (effect->type == CLEAR_TAG_EFFECT_LIGHT_RAYS) {
            // Apply the light ray effect material if it has not already been applied.
            if (!isMaterialApplied) {
                gDPPipeSync(POLY_XLU_DISP++);
                gDPSetEnvColor(POLY_XLU_DISP++, (u8)effect->envColor.r, (u8)effect->envColor.g, (u8)effect->envColor.b,
                               0);
                gSPDisplayList(POLY_XLU_DISP++, gClearTagLightRayEffectMaterialDL);
                isMaterialApplied++;
            }

            // Draw the light ray effect.
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, (u8)effect->primColor.r, (u8)effect->primColor.g,
                            (u8)effect->primColor.b, (u8)effect->primColor.a);
            Matrix_Translate(effect->pos.x, effect->pos.y, effect->pos.z, MTXMODE_NEW);
            Matrix_RotateYF(effect->rotationY, MTXMODE_APPLY);
            Matrix_RotateXFApply(effect->rotationX);
            Matrix_RotateZF(effect->rotationZ, MTXMODE_APPLY);
            Matrix_Scale(effect->scale * 0.5f, effect->scale * 0.5f, effect->maxScale * effect->scale, MTXMODE_APPLY);
            Matrix_RotateXFApply(M_PI / 2);
            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            // @recomp Tag the matrix.
            gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, actor_transform_id(thisx) + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
            gSPDisplayList(POLY_XLU_DISP++, gClearTagLightRayEffectDL);
            // @recomp Pop the matrix tag.
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        }
    }

    // Draw all splash effects.
    effect = firstEffect;
    for (i = 0; i < ARRAY_COUNT(this->effect); i++, effect++) {
        if (effect->type == CLEAR_TAG_EFFECT_SPLASH) {
            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 200);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 200);
            gSPSegment(POLY_XLU_DISP++, 0x08, Lib_SegmentedToVirtual(sWaterSplashTextures[effect->actionTimer]));
            Gfx_SetupDL61_Xlu(gfxCtx);
            gSPClearGeometryMode(POLY_XLU_DISP++, G_CULL_BACK);
            isMaterialApplied++;

            // Apply material 16 times along a circle to give the appearance of a splash
            for (j = 0; j < 16; j++) {
                Matrix_RotateYF(2.0f * (j * M_PI) * (1.0f / 16.0f), MTXMODE_NEW);
                Matrix_MultVecZ(effect->maxScale, &vec);
                /**
                 * Get the water surface at point (`x`, `ySurface`, `z`). `ySurface` doubles as position y input
                 * returns true if point is within the xz boundaries of an active water box, else false
                 * `ySurface` returns the water box's surface, while `outWaterBox` returns a pointer to the WaterBox
                 */
                ySurface = effect->pos.y;
                if (WaterBox_GetSurface1(play, &play->colCtx, effect->pos.x + vec.x, effect->pos.z + vec.z, &ySurface,
                                         &waterBox)) {
                    if ((effect->pos.y - ySurface) < 200.0f) {
                        // Draw the splash effect.
                        Matrix_Translate(effect->pos.x + vec.x, ySurface, effect->pos.z + vec.z, MTXMODE_NEW);
                        Matrix_RotateYF(2.0f * (j * M_PI) * (1.0f / 16.0f), MTXMODE_APPLY);
                        Matrix_RotateXFApply(effect->rotationX);
                        Matrix_Scale(effect->scale, effect->scale, effect->scale, MTXMODE_APPLY);
                        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                        // @recomp TODO figure out a way to tag these without increasing the allocated IDs per actor. 
                        gSPDisplayList(POLY_XLU_DISP++, gEffWaterSplashDL);
                    }
                }
            }
        }
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

// @recomp Patched to tag the two custom lens flares (used by the Igos du Ikana curtains).
RECOMP_PATCH void Environment_DrawCustomLensFlare(PlayState* play) {
    Vec3f pos;

    // @recomp Set up the graphics context.
    OPEN_DISPS(play->state.gfxCtx);

    if (gCustomLensFlare1On) {
        pos.x = gCustomLensFlare1Pos.x;
        pos.y = gCustomLensFlare1Pos.y;
        pos.z = gCustomLensFlare1Pos.z;
        // @recomp Tag the entire lens flare effect, using order linear. Skip interpolation when the camera is skipped.
        if (camera_was_skipped()) {
            gEXMatrixGroupDecomposedSkipAll(POLY_XLU_DISP++, CUSTOM_LENS_FLARE_TRANSFORM_ID_START + 0, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
        }
        else {
            gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, CUSTOM_LENS_FLARE_TRANSFORM_ID_START + 0, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
        }
        Environment_DrawLensFlare(play, &play->envCtx, &play->view, play->state.gfxCtx, pos, D_801F4E44, D_801F4E48,
                                  D_801F4E4C, false);
        // @recomp Pop the matrix group.
        gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
    }

    if (gCustomLensFlare2On) {
        pos.x = gCustomLensFlare2Pos.x;
        pos.y = gCustomLensFlare2Pos.y;
        pos.z = gCustomLensFlare2Pos.z;
        // @recomp Tag the entire lens flare effect, using order linear. Skip interpolation when the camera is skipped.
        if (camera_was_skipped()) {
            gEXMatrixGroupDecomposedSkipAll(POLY_XLU_DISP++, CUSTOM_LENS_FLARE_TRANSFORM_ID_START + 1, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
        }
        else {
            gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, CUSTOM_LENS_FLARE_TRANSFORM_ID_START + 1, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
        }
        Environment_DrawLensFlare(play, &play->envCtx, &play->view, play->state.gfxCtx, pos, D_801F4E5C, D_801F4E60,
                                  D_801F4E64, false);
        // @recomp Pop the matrix group.
        gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
    }

    // @recomp Close the graphics context.
    CLOSE_DISPS();
}

// @recomp Patched to tag the sun lens flare.
RECOMP_PATCH void Environment_DrawSunLensFlare(PlayState* play, EnvironmentContext* envCtx, View* view, GraphicsContext* gfxCtx,
                                  Vec3f vec) {
    if ((play->envCtx.precipitation[PRECIP_RAIN_CUR] == 0) &&
        !(GET_ACTIVE_CAM(play)->stateFlags & CAM_STATE_UNDERWATER) && (play->skyboxId == SKYBOX_NORMAL_SKY)) {
        f32 v0 = Math_CosS(CURRENT_TIME - CLOCK_TIME(12, 0));

        // @recomp Set up the graphics context.
        OPEN_DISPS(play->state.gfxCtx);

        // @recomp Tag the entire lens flare effect, using order linear. Skip interpolation when the camera is skipped.
        if (camera_was_skipped()) {
            gEXMatrixGroupDecomposedSkipAll(POLY_XLU_DISP++, SUN_LENS_FLARE_TRANSFORM_ID, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
        }
        else {
            gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, SUN_LENS_FLARE_TRANSFORM_ID, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
        }
        Environment_DrawLensFlare(play, &play->envCtx, &play->view, play->state.gfxCtx, vec, 370.0f, v0 * 120.0f, 0x190,
                                  true);
        // @recomp Pop the matrix group.
        gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        
        // @recomp Close the graphics context.
        CLOSE_DISPS();
    }
}
