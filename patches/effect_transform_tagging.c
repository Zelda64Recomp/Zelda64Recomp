#include "patches.h"
#include "transform_ids.h"
#include "overlays/actors/ovl_En_Test7/z_en_test7.h"

#define THIS ((EnTest7*)thisx)

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
    EnTest7* this = THIS;
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
        OverrideKeyframeDrawScaled func_80AF31D0_relocated = (OverrideKeyframeDrawScaled)((uintptr_t)func_80AF31D0 -
            (intptr_t)((uintptr_t)thisx->overlayEntry->vramStart - (uintptr_t)thisx->overlayEntry->loadedRamAddr));
        
        // @recomp Push the matrix group for the song of soaring's wings.
        gEXMatrixGroup(POLY_OPA_DISP++, SOARING_WINGS_TRANSFORM_ID, G_EX_PUSH, G_MTX_MODELVIEW,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_ORDER_LINEAR);
        
        func_8018450C(play, &this->skeletonInfo, mtx, func_80AF31D0_relocated, NULL, &this->actor);
        
        // @recomp Pop the wings matrix group.
        gEXPopMatrixGroup(POLY_OPA_DISP++);
    }

    // Draw windCapsule encasing that surrounds player after wings
    if (this->flags & OWL_WARP_FLAGS_DRAW_WIND_CAPSULE) {
        // @recomp Push the matrix group for the song of soaring's capsule.
        gEXMatrixGroup(POLY_XLU_DISP++, SOARING_CAPSULE_TRANSFORM_ID, G_EX_PUSH, G_MTX_MODELVIEW,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_ORDER_LINEAR);
        
        Matrix_Push();
        Matrix_Translate(0.0f, 4000.0f, 0.0f, MTXMODE_APPLY);
        Matrix_RotateZYX(0, this->windCapsule.yaw, 0, MTXMODE_APPLY);
        Matrix_Scale(this->windCapsule.xzScale * 100.0f, this->windCapsule.yScale * 100.0f,
                     this->windCapsule.xzScale * 100.0f, MTXMODE_APPLY);
        sp40 = this->windCapsule.unk_00;
        AnimatedMat_DrawStep(play, Lib_SegmentedToVirtual(&gSoaringWarpCsWindCapsuleTexAnim), sp40);        
        Gfx_DrawDListXlu(play, gSoaringWarpCsWindCapsuleDL);

        // @recomp Pop the capsule matrix group.
        gEXPopMatrixGroup(POLY_XLU_DISP++);
        Matrix_Pop();
    }

    EnTest7_DrawFeathers(play, this->feathers);

    if (this->flags & OWL_WARP_FLAGS_DRAW_LENS_FLARE) {
        Environment_DrawLensFlare(play, &play->envCtx, &play->view, play->state.gfxCtx, this->actor.world.pos, 70.0f,
                                  5.0f, 0, false);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

#undef TEST
