#include "patches.h"
#include "transform_ids.h"
#include "overlays/actors/ovl_En_Test7/z_en_test7.h"
#include "overlays/actors/ovl_Object_Kankyo/z_object_kankyo.h"
#include "z64effect.h"

// Decomp renames, TODO update decomp and remove these
#define gSoaringWarpCsWindCapsuleTexAnim gameplay_keep_Matanimheader_0815D0
#define gSoaringWarpCsWindCapsuleDL gameplay_keep_DL_080FC8
#define EnTest7_DrawFeathers func_80AF14FC

void EnTest7_DrawFeathers(PlayState* play2, OwlWarpFeather* feathers);
s32 func_80AF31D0(PlayState* play, SkeletonInfo* skeletonInfo, s32 limbIndex, Gfx** dList, u8* flags, Actor* thisx,
                  Vec3f* scale, Vec3s* rot, Vec3f* pos);

extern AnimatedMaterial gSoaringWarpCsWindCapsuleTexAnim;
extern Gfx gSoaringWarpCsWindCapsuleDL[];

void EnTest7_Draw(Actor* thisx, PlayState* play) {
    s32 pad[2];
    EnTest7* this = (EnTest7*)thisx;
    s32 sp40;

    OPEN_DISPS(play->state.gfxCtx);

    // Draw wings
    if (this->flags & OWL_WARP_FLAGS_DRAW_WINGS) {
        Mtx* mtx = GRAPH_ALLOC(play->state.gfxCtx, this->skeletonInfo.unk_18->unk_1 * sizeof(Mtx));

        if (mtx == NULL) {
            return;
        }
        // func_80AF31D0 is an overlay symbol, so its addresses need to be offset to get the actual loaded vram address.
        // TODO remove this once the recompiler is able to handle overlay symbols automatically for patch functions.
        OverrideKeyframeDrawScaled func_80AF31D0_relocated = (OverrideKeyframeDrawScaled)actor_relocate(thisx, func_80AF31D0);
        
        // @recomp Push the matrix group for the song of soaring's wings.
        gEXMatrixGroupDecomposedNormal(POLY_OPA_DISP++, SOARING_WINGS_TRANSFORM_ID, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
        
        func_8018450C(play, &this->skeletonInfo, mtx, func_80AF31D0_relocated, NULL, &this->actor);
        
        // @recomp Pop the wings matrix group.
        gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);
    }

    // Draw windCapsule encasing that surrounds player after wings
    if (this->flags & OWL_WARP_FLAGS_DRAW_WIND_CAPSULE) {
        // @recomp Push the matrix group for the song of soaring's capsule.
        gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, SOARING_CAPSULE_TRANSFORM_ID, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
        
        Matrix_Push();
        Matrix_Translate(0.0f, 4000.0f, 0.0f, MTXMODE_APPLY);
        Matrix_RotateZYX(0, this->windCapsule.yaw, 0, MTXMODE_APPLY);
        Matrix_Scale(this->windCapsule.xzScale * 100.0f, this->windCapsule.yScale * 100.0f,
                     this->windCapsule.xzScale * 100.0f, MTXMODE_APPLY);
        sp40 = this->windCapsule.unk_00;
        AnimatedMat_DrawStep(play, Lib_SegmentedToVirtual(&gSoaringWarpCsWindCapsuleTexAnim), sp40);        
        Gfx_DrawDListXlu(play, gSoaringWarpCsWindCapsuleDL);

        // @recomp Pop the capsule matrix group.
        gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        Matrix_Pop();
    }

    EnTest7_DrawFeathers(play, this->feathers);

    if (this->flags & OWL_WARP_FLAGS_DRAW_LENS_FLARE) {
        Environment_DrawLensFlare(play, &play->envCtx, &play->view, play->state.gfxCtx, this->actor.world.pos, 70.0f,
                                  5.0f, 0, false);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

extern f32 D_808DE5B0;
extern Gfx gEffDust5Tex[];
extern Gfx gEffDustDL[];

// Use an unused byte in the particle's struct to track whether its interpolation should be skipped this frame.
#define particle_skipped(particle) \
    ((&(particle).unk_1C)[1])

// @recomp Patched to record when a particle is moved to skip interpolation.
void func_808DC454(ObjectKankyo* this, PlayState* play) {
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

void func_808DD3C8(Actor* thisx, PlayState* play2) {
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
        // @recomp Manual relocation, TODO remove when the recompiler handles this automatically.
        f32* D_808DE5B0_ptr = actor_relocate(thisx, &D_808DE5B0);

        temp_f0 = func_80173B48(&play->state) / 1.4e7f;
        temp_f0 = CLAMP(temp_f0, 0.0f, 1.0f);
        Math_SmoothStepToF(D_808DE5B0_ptr, temp_f0, 0.2f, 0.1f, 0.001f);

        sp68 = play->envCtx.precipitation[PRECIP_SNOW_CUR];
        sp68 *= *D_808DE5B0_ptr;

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
                gEXMatrixGroupDecomposedSkip(POLY_XLU_DISP++, actor_transform_id(thisx) + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
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
void Effect_DrawAll(GraphicsContext* gfxCtx) {
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
