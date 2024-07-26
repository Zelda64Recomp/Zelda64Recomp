#include "patches.h"
#include "transform_ids.h"
#include "overlays/actors/ovl_En_Test6/z_en_test6.h"
#include "overlays/actors/ovl_En_Test7/z_en_test7.h"

// Decomp renames, TODO update decomp and remove these
#define gSoaringWarpCsWindCapsuleTexAnim gameplay_keep_Matanimheader_0815D0
#define gSoaringWarpCsWindCapsuleDL gameplay_keep_DL_080FC8
#define EnTest7_DrawFeathers func_80AF14FC

void EnTest7_DrawFeathers(PlayState* play2, OwlWarpFeather* feathers);
s32 func_80AF31D0(PlayState* play, SkeletonInfo* skeletonInfo, s32 limbIndex, Gfx** dList, u8* flags, Actor* thisx,
                  Vec3f* scale, Vec3s* rot, Vec3f* pos);

extern AnimatedMaterial gSoaringWarpCsWindCapsuleTexAnim;
extern Gfx gSoaringWarpCsWindCapsuleDL[];

RECOMP_PATCH void EnTest7_Draw(Actor* thisx, PlayState* play) {
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
        // @recomp Push the matrix group for the song of soaring's wings.
        gEXMatrixGroupDecomposedNormal(POLY_OPA_DISP++, SOARING_WINGS_TRANSFORM_ID, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
        
        func_8018450C(play, &this->skeletonInfo, mtx, func_80AF31D0, NULL, &this->actor);
        
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
        // @recomp Tag the entire lens flare effect, using order linear. Skip interpolation when the camera is skipped.
        if (camera_was_skipped()) {
            gEXMatrixGroupDecomposedSkipAll(POLY_XLU_DISP++, SOARING_LENS_FLARE_TRANSFORM_ID, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
        }
        else {
            gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, SOARING_LENS_FLARE_TRANSFORM_ID, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
        }
        Environment_DrawLensFlare(play, &play->envCtx, &play->view, play->state.gfxCtx, this->actor.world.pos, 70.0f,
                                  5.0f, 0, false);
        // @recomp Pop the lens flare matrix group.
        gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

extern Gfx gSongOfTimeClockDL[];

struct SoTCsAmmoDrops;

typedef void (*SoTCsAmmoDropDrawFunc)(EnTest6*, PlayState*, struct SoTCsAmmoDrops*);

typedef enum SoTCsAmmoDropType {
    /* 0 */ SOTCS_AMMO_DROP_NONE,
    /* 1 */ SOTCS_AMMO_DROP_ARROWS,
    /* 2 */ SOTCS_AMMO_DROP_BOMB,
    /* 3 */ SOTCS_AMMO_DROP_DEKU_NUT,
    /* 4 */ SOTCS_AMMO_DROP_DEKU_STICK,
    /* 5 */ SOTCS_AMMO_DROP_RUPEE_GREEN,
    /* 6 */ SOTCS_AMMO_DROP_RUPEE_BLUE
} SoTCsAmmoDropType;

typedef struct SoTCsAmmoDrops {
    /* 0x00 */ SoTCsAmmoDropType type;
    /* 0x04 */ f32 scale;
    /* 0x08 */ Vec3f pos;
    /* 0x14 */ SoTCsAmmoDropDrawFunc draw;
} SoTCsAmmoDrops; // size = 0x18

extern SoTCsAmmoDrops sSoTCsAmmoDrops[12];

/**
 * Draws clocks in a double spiral above and below player
 */
RECOMP_PATCH void EnTest6_DrawThreeDayResetSoTCutscene(EnTest6* this, PlayState* play) {
    s16 clock1Yaw;
    s16 clock2Yaw;
    s16 angle;
    s32 i;
    Vec3f clockPos;

    OPEN_DISPS(play->state.gfxCtx);

    this->gfx = POLY_OPA_DISP;
    clockPos.y = 0.0f;

    clock1Yaw = this->clockAngle;
    clock2Yaw = clock1Yaw + 0x4E20 + (s32)(0x2EE0 * Math_SinS(play->state.frames));
    // The `& 0x3C` ensures the angle only updates once every 4 frames
    angle = (play->state.frames & 0x3C) * 1024;
    angle *= this->clockSpeed / 200.0f;
    this->clockAngle += (s16)this->clockSpeed;
    this->clockRingRotZ = (s16)((this->clockSpeed / 200.0f) * 256.0f);

    // Draw 2 clocks per loop
    for (i = 0; i < (SOTCS_RESET_NUM_CLOCKS / 2); i++) {
        // Clock 1
        clock1Yaw += 0x1000;
        clockPos.x = Math_CosS(clock1Yaw) * this->clockDist;
        clockPos.z = Math_SinS(clock1Yaw) * this->clockDist;
        Matrix_Translate(clockPos.x, clockPos.y, clockPos.z, MTXMODE_NEW);
        Matrix_RotateXS(0x4000, MTXMODE_APPLY);
        Matrix_Scale(0.8f, 0.8f, 0.8f, MTXMODE_APPLY);
        Matrix_RotateZS(angle, MTXMODE_APPLY);

        gSPMatrix(this->gfx++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gDPSetPrimColor(this->gfx++, 0, 0xFF, 28, 28, 28, 255);
        gDPSetEnvColor(this->gfx++, 255, 255, 255, 255);
        gDPSetRenderMode(this->gfx++, G_RM_FOG_SHADE_A, G_RM_AA_ZB_OPA_SURF2);

        // @recomp Tag the clock's matrix.
        gEXMatrixGroupDecomposedSkipRot(this->gfx++, actor_transform_id(&this->actor) + (2 * i) + 0, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);

        gSPDisplayList(this->gfx++, gSongOfTimeClockDL);

        // @recomp Pop the clock's matrix group.
        gEXPopMatrixGroup(this->gfx++, G_MTX_MODELVIEW);

        // Clock 2
        clock2Yaw += 0x1000;
        clockPos.x = Math_CosS(clock2Yaw) * this->clockDist;
        clockPos.z = Math_SinS(clock2Yaw) * this->clockDist;
        Matrix_Translate(clockPos.x, clockPos.y, clockPos.z, MTXMODE_NEW);
        Matrix_RotateXS(0x4000, MTXMODE_APPLY);
        Matrix_Scale(0.8f, 0.8f, 0.8f, MTXMODE_APPLY);
        Matrix_RotateZS(-angle, MTXMODE_APPLY);

        gSPMatrix(this->gfx++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gDPSetPrimColor(this->gfx++, 0, 0xFF, 28, 28, 28, 255);
        gDPSetEnvColor(this->gfx++, 255, 255, 255, 255);
        gDPSetRenderMode(this->gfx++, G_RM_FOG_SHADE_A, G_RM_AA_ZB_OPA_SURF2);

        // @recomp Tag the clock's matrix.
        gEXMatrixGroupDecomposedSkipRot(this->gfx++, actor_transform_id(&this->actor) + (2 * i) + 1, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);

        gSPDisplayList(this->gfx++, gSongOfTimeClockDL);

        // @recomp Pop the clock's matrix group.
        gEXPopMatrixGroup(this->gfx++, G_MTX_MODELVIEW);

        clockPos.y -= this->speed;
        angle += this->clockRingRotZ;
    }

    POLY_OPA_DISP = this->gfx;

    for (i = 0; i < ARRAY_COUNT(sSoTCsAmmoDrops); i++) {
        if (sSoTCsAmmoDrops[i].scale > 0.0f) {
            sSoTCsAmmoDrops[i].draw(this, play, &sSoTCsAmmoDrops[i]);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx);
}
