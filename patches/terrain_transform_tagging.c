#include "patches.h"
#include "transform_ids.h"
#include "overlays/actors/ovl_Dm_Opstage/z_dm_opstage.h"
#include "overlays/actors/ovl_Dm_Char01/z_dm_char01.h"

static Vec3f sZeroVec = { 0.0f, 0.0f, 0.0f };

extern RoomDrawHandler sRoomDrawHandlers[];

RECOMP_PATCH void Room_Draw(PlayState* play, Room* room, u32 flags) {
    if (room->segment != NULL) {
        gSegments[3] = OS_K0_TO_PHYSICAL(room->segment);
        
        OPEN_DISPS(play->state.gfxCtx);

        // @recomp Tag the room's matrices if applicable.
        // Tag terrain as being ignored for interpolation, which prevents interpolation glitches where some pieces of terrain swap places when one comes into view.
        if (flags & ROOM_DRAW_OPA) {
            gEXMatrixGroupInterpolateOnlyTiles(POLY_OPA_DISP++, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
        }
        if (flags & ROOM_DRAW_XLU) {
            gEXMatrixGroupInterpolateOnlyTiles(POLY_XLU_DISP++, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
        }

        CLOSE_DISPS(play->state.gfxCtx);

        sRoomDrawHandlers[room->roomShape->base.type](play, room, flags);
        
        OPEN_DISPS(play->state.gfxCtx);

        // @recomp Pop the room's matrix tags if applicable.
        if (flags & ROOM_DRAW_OPA) {
            gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);
        }
        if (flags & ROOM_DRAW_XLU) {
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        }

        CLOSE_DISPS(play->state.gfxCtx);
    }
    return;
}

extern Gfx gKeikokuDemoFloorDL[];
extern Gfx gKeikokuDemoFloorEmptyDL[];
extern Gfx gKeikokuDemoTallTreeWithRootBaseDL[];
extern Gfx gKeikokuDemoTallTreeWithRootBaseEmptyDL[];
extern Gfx gKeikokuDemoTallTreeCutDL[];
extern Gfx gKeikokuDemoTallTreeCutEmptyDL[];
extern Gfx gKeikokuDemoTallTreeStraightDL[];
extern Gfx gKeikokuDemoTallTreeStraightEmptyDL[];

// @recomp Tag the ground in the intro cutscene to not interpolate rotation.
RECOMP_PATCH void DmOpstage_Draw(Actor* thisx, PlayState* play) {
    DmOpstage* this = (DmOpstage*)thisx;

    if (DMOPSTAGE_GET_TYPE(&this->dyna.actor) > DMOPSTAGE_TYPE_GROUND) {
        // Assumption: worldPos is being manipulated by cutscene
        Matrix_Translate(this->dyna.actor.world.pos.x + this->drawOffset.x,
                         this->dyna.actor.world.pos.y + this->drawOffset.y,
                         this->dyna.actor.world.pos.z + this->drawOffset.z, MTXMODE_NEW);
        Matrix_RotateYS(this->dyna.actor.world.rot.y, MTXMODE_APPLY);
        Matrix_Scale(0.1f, 0.1f, 0.1f, MTXMODE_APPLY);
    }

    switch (DMOPSTAGE_GET_TYPE(&this->dyna.actor)) {
        case DMOPSTAGE_TYPE_GROUND:
            OPEN_DISPS(play->state.gfxCtx);
            
            // @recomp Tag the ground to skip rotation.
            gEXMatrixGroupDecomposedSkipRot(POLY_OPA_DISP++, actor_transform_id(thisx) + 0, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);

            Gfx_DrawDListOpa(play, gKeikokuDemoFloorDL);
            Gfx_DrawDListXlu(play, gKeikokuDemoFloorEmptyDL);

            // @recomp Pop the tag.
            gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);

            CLOSE_DISPS(play->state.gfxCtx);
            break;

        case DMOPSTAGE_TYPE_ROOT_TREE:
            Gfx_DrawDListOpa(play, gKeikokuDemoTallTreeWithRootBaseDL);
            Gfx_DrawDListXlu(play, gKeikokuDemoTallTreeWithRootBaseEmptyDL);
            break;

        case DMOPSTAGE_TYPE_CUT_TREE:
            Gfx_DrawDListOpa(play, gKeikokuDemoTallTreeCutDL);
            Gfx_DrawDListXlu(play, gKeikokuDemoTallTreeCutEmptyDL);
            break;

        case DMOPSTAGE_TYPE_STRAIGHT_TREE:
            Gfx_DrawDListOpa(play, gKeikokuDemoTallTreeStraightDL);
            Gfx_DrawDListXlu(play, gKeikokuDemoTallTreeStraightEmptyDL);
            break;

        default:
            break;
    }
}

extern AnimatedMaterial gWoodfallSceneryPoisonWaterTexAnim[];
extern AnimatedMaterial gWoodfallSceneryPurifiedWaterTexAnim[];
extern AnimatedMaterial gWoodfallSceneryDynamicPoisonWaterTexAnim[];
extern AnimatedMaterial gWoodfallSceneryPoisonWallsTexAnim[];
extern AnimatedMaterial gWoodfallSceneryPurifiedWallsTexAnim[];
extern AnimatedMaterial gWoodfallSceneryTempleTexAnim[];
extern AnimatedMaterial gWoodfallSceneryWaterFlowingOverTempleTexAnim[];
extern Gfx gWoodfallSceneryPoisonWaterDL[];
extern Gfx gWoodfallSceneryFloorDL[]; 
extern Gfx gWoodfallSceneryPurifiedWaterDL[];
extern Gfx gWoodfallSceneryPurifiedWallsDL[];
extern Gfx gWoodfallSceneryDynamicPoisonWaterDL[];
extern Gfx gWoodfallSceneryPoisonWallsDL[];
extern Gfx gWoodfallSceneryTempleDL[];
extern Gfx gWoodfallSceneryWaterFlowingOverTempleDL[];
extern Gfx gWoodfallSceneryTempleEntrancesDL[];
extern Gfx gWoodfallSceneryTempleRampAndPlatformDL[];
extern Vtx gWoodfallSceneryDynamicPoisonWaterVtx[];

extern s16 D_80AAAE20;
extern s16 D_80AAAE22;
extern s16 D_80AAAE24;

extern f32 D_80AAAAB8;
extern f32 D_80AAAABC;
extern s16 D_80AAAAC0;
extern s16 D_80AAAAC4;
extern s16 D_80AAAAC8;
extern s16 D_80AAAACC;

// @recomp Patched to enable vertex interpolation for the dynamic water as Woodfall temple rises from below the water.
RECOMP_PATCH void DmChar01_Draw(Actor* thisx, PlayState* play) {
    // @recomp Move function statics to externs so they still get reset on overlay load like normal.
    DmChar01* this = (DmChar01*)thisx;
    f32 temp_f12;
    f32 spBC;
    s32 i;
    u8 spB7 = false;

    switch (DMCHAR01_GET(thisx)) {
        case DMCHAR01_0:
            switch (this->unk_34C) {
                case 0:
                    AnimatedMat_Draw(play, Lib_SegmentedToVirtual(gWoodfallSceneryPoisonWaterTexAnim));
                    Gfx_DrawDListOpa(play, gWoodfallSceneryPoisonWaterDL);
                    break;

                case 1:
                    if (gSaveContext.sceneLayer == 1) {
                        AnimatedMat_Draw(play, Lib_SegmentedToVirtual(gWoodfallSceneryPurifiedWaterTexAnim));
                        Gfx_DrawDListOpa(play, gWoodfallSceneryFloorDL);
                        Gfx_DrawDListXlu(play, gWoodfallSceneryPurifiedWaterDL);
                        Matrix_Translate(0.0f, 10.0f, 0.0f, MTXMODE_APPLY);
                    }
                    AnimatedMat_Draw(play, Lib_SegmentedToVirtual(gWoodfallSceneryDynamicPoisonWaterTexAnim));

                    OPEN_DISPS(play->state.gfxCtx);

                    if ((u8)this->unk_348 == 255) {
                        Gfx_SetupDL25_Opa(play->state.gfxCtx);

                        gDPSetRenderMode(POLY_OPA_DISP++, G_RM_FOG_SHADE_A, G_RM_AA_ZB_OPA_SURF2);
                        gDPPipeSync(POLY_OPA_DISP++);
                        gDPSetEnvColor(POLY_OPA_DISP++, 0, 0, 0, 255);
                        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0x96, 255, 255, 255, 255);
                        gSPSegment(POLY_OPA_DISP++, 0x0B, gWoodfallSceneryDynamicPoisonWaterVtx);
                        gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx),
                                  G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                        // @recomp Tag the matrix to enable vertex interpolation.
                        gEXMatrixGroupDecomposedVerts(POLY_OPA_DISP++, actor_transform_id(thisx), G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
                        gSPDisplayList(POLY_OPA_DISP++, gWoodfallSceneryDynamicPoisonWaterDL);
                        // @recomp Tag the matrix to enable vertex interpolation.
                        gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);
                    } else {
                        Gfx_SetupDL25_Xlu(play->state.gfxCtx);

                        gDPSetRenderMode(POLY_XLU_DISP++, G_RM_FOG_SHADE_A, G_RM_AA_ZB_XLU_SURF2);
                        gDPPipeSync(POLY_XLU_DISP++);
                        gDPSetEnvColor(POLY_XLU_DISP++, 0, 0, 0, (u8)this->unk_348);
                        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x96, 255, 255, 255, (u8)this->unk_348);
                        gSPSegment(POLY_XLU_DISP++, 0x0B, gWoodfallSceneryDynamicPoisonWaterVtx);
                        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx),
                                  G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                        // @recomp Tag the matrix to enable vertex interpolation.
                        gEXMatrixGroupDecomposedVerts(POLY_XLU_DISP++, actor_transform_id(thisx), G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
                        gSPDisplayList(POLY_XLU_DISP++, gWoodfallSceneryDynamicPoisonWaterDL);
                        // @recomp Tag the matrix to enable vertex interpolation.
                        gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
                    }

                    CLOSE_DISPS(play->state.gfxCtx);
                    break;

                case 2:
                    AnimatedMat_Draw(play, Lib_SegmentedToVirtual(gWoodfallSceneryPurifiedWaterTexAnim));
                    Gfx_DrawDListOpa(play, gWoodfallSceneryFloorDL);
                    Gfx_DrawDListXlu(play, gWoodfallSceneryPurifiedWaterDL);
                    break;
            }
            break;

        case DMCHAR01_1:
            switch (this->unk_34C) {
                case 0:
                    AnimatedMat_Draw(play, Lib_SegmentedToVirtual(gWoodfallSceneryPoisonWallsTexAnim));
                    Gfx_DrawDListOpa(play, gWoodfallSceneryPoisonWallsDL);
                    break;

                case 1:
                    AnimatedMat_Draw(play, Lib_SegmentedToVirtual(gWoodfallSceneryPurifiedWallsTexAnim));
                    Gfx_DrawDListOpa(play, gWoodfallSceneryPurifiedWallsDL);
                    break;
            }
            break;

        case DMCHAR01_2:
            AnimatedMat_Draw(play, Lib_SegmentedToVirtual(gWoodfallSceneryTempleTexAnim));
            Gfx_DrawDListOpa(play, gWoodfallSceneryTempleDL);

            if ((this->unk_34C != 0) && ((u8)this->unk_348 != 0)) {
                AnimatedMat_Draw(play, Lib_SegmentedToVirtual(gWoodfallSceneryWaterFlowingOverTempleTexAnim));

                OPEN_DISPS(play->state.gfxCtx);

                Gfx_SetupDL25_Xlu(play->state.gfxCtx);

                gDPPipeSync(POLY_XLU_DISP++);
                gDPSetEnvColor(POLY_XLU_DISP++, 0, 0, 0, (u8)this->unk_348);
                gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, 255, 255, 255, (u8)this->unk_348);
                gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx),
                          G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                gSPDisplayList(POLY_XLU_DISP++, gWoodfallSceneryWaterFlowingOverTempleDL);

                CLOSE_DISPS(play->state.gfxCtx);
            }
            
            if (D_80AAAE24 != 0) {
                if ((D_80AAAE22 > -1800) && (D_80AAAE22 < 3000)) {
                    temp_f12 = D_80AAAE22 - 640.0f;
                    if ((D_80AAAE20 == 380) && (D_80AAAE22 > 640)) {
                        D_80AAAAC0 = 2;
                        D_80AAAAC4 = 0;
                        D_80AAAAC8 = 900;
                        D_80AAAACC = 700;
                        spB7 = true;
                        if (D_80AAAE22 < 1350) {
                            f32 temp_f0 = temp_f12 / 2000.0f;

                            D_80AAAAB8 = 420.0f - (420.0f * temp_f0);
                            D_80AAAABC = (200.0f * temp_f0) + 200.0f;
                        } else {
                            f32 temp_f0 = temp_f12 / 2000.0f;

                            D_80AAAAB8 = 420.0f - (420.0f * temp_f0);
                            D_80AAAABC = 400.0f;
                        }
                    }
                }

                if (spB7) {
                    for (i = 0; i < D_80AAAAC0 * 2; i++) {
                        Vec3f sp44;
                        f32 phi_f2 = D_80AAAABC;
                        s16 temp;

                        spBC = Rand_ZeroOne() * D_80AAAAC8;
                        if ((play->state.frames % 2) != 0) {
                            sp44.x = (Rand_ZeroOne() - 0.5f) * (2.0f * phi_f2);
                            sp44.y = D_80AAAAB8;
                            sp44.z = (Rand_ZeroOne() * D_80AAAAC4) + phi_f2;
                            temp = (s16)spBC + D_80AAAACC;
                            EffectSsGSplash_Spawn(play, &sp44, NULL, NULL, 0, temp);
                        } else {
                            sp44.x = -phi_f2 - (Rand_ZeroOne() * D_80AAAAC4);
                            sp44.y = D_80AAAAB8;
                            sp44.z = (Rand_ZeroOne() - 0.5f) * (2.0f * phi_f2);
                            temp = (s16)spBC + D_80AAAACC;
                            EffectSsGSplash_Spawn(play, &sp44, NULL, NULL, 0, temp);
                        }
                    }
                }
            }

            Gfx_DrawDListXlu(play, gWoodfallSceneryTempleEntrancesDL);
            break;

        case DMCHAR01_3:
            if (thisx->world.pos.y > -120.0f) {
                Gfx_DrawDListOpa(play, gWoodfallSceneryTempleRampAndPlatformDL);
            }
            break;
    }
}
