#include "patches.h"
#include "transform_ids.h"
#include "overlays/actors/ovl_Arms_Hook/z_arms_hook.h"

extern Vec3f D_808C1C10;
extern Vec3f D_808C1C1C;
extern Vec3f D_808C1C28;
extern Vec3f D_808C1C34;
extern Vec3f D_808C1C40;
extern Vec3f D_808C1C4C;

extern Gfx object_link_child_DL_01D960[];
extern Gfx gHookshotChainDL[];

#define THIS ((ArmsHook*)thisx)

void ArmsHook_Shoot(ArmsHook* this, PlayState* play);

RECOMP_PATCH void ArmsHook_Draw(Actor* thisx, PlayState* play) {
    ArmsHook* this = THIS;
    f32 f0;
    Player* player = GET_PLAYER(play);

    if ((player->actor.draw != NULL) && (player->rightHandType == PLAYER_MODELTYPE_RH_HOOKSHOT)) {
        Vec3f sp68;
        Vec3f sp5C;
        Vec3f sp50;
        f32 sp4C;
        f32 sp48;

        OPEN_DISPS(play->state.gfxCtx);

        if ((ArmsHook_Shoot != this->actionFunc) || (this->timer <= 0)) {
            Matrix_MultVec3f(&D_808C1C10, &this->unk1E0);
            Matrix_MultVec3f(&D_808C1C28, &sp5C);
            Matrix_MultVec3f(&D_808C1C34, &sp50);
            this->weaponInfo.active = false;
        } else {
            Matrix_MultVec3f(&D_808C1C1C, &this->unk1E0);
            Matrix_MultVec3f(&D_808C1C40, &sp5C);
            Matrix_MultVec3f(&D_808C1C4C, &sp50);
        }
        func_80126440(play, &this->collider, &this->weaponInfo, &sp5C, &sp50);
        Gfx_SetupDL25_Opa(play->state.gfxCtx);
        func_80122868(play, player);

        // @recomp Tag the matrices for the hookshot tip and chain.
        u32 cur_transform_id = actor_transform_id(thisx);
        gEXMatrixGroupSimple(POLY_OPA_DISP++, cur_transform_id, G_EX_PUSH, G_MTX_MODELVIEW,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);
    
        gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_OPA_DISP++, object_link_child_DL_01D960);
        Matrix_Translate(this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z, MTXMODE_NEW);
        Math_Vec3f_Diff(&player->rightHandWorld.pos, &this->actor.world.pos, &sp68);
        sp48 = SQXZ(sp68);
        sp4C = sqrtf(SQXZ(sp68));
        Matrix_RotateYS(Math_Atan2S(sp68.x, sp68.z), MTXMODE_APPLY);
        Matrix_RotateXS(Math_Atan2S(-sp68.y, sp4C), MTXMODE_APPLY);
        f0 = sqrtf(SQ(sp68.y) + sp48);
        Matrix_Scale(0.015f, 0.015f, f0 * 0.01f, MTXMODE_APPLY);
        gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_OPA_DISP++, gHookshotChainDL);
        func_801229A0(play, player);

        // @recomp Pop the transform id.
        gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);

        CLOSE_DISPS(play->state.gfxCtx);
    }
}

#undef THIS

extern Gfx gHookshotReticleDL[];
RECOMP_PATCH void Player_DrawHookshotReticle(PlayState* play, Player* player, f32 hookshotDistance) {
    static Vec3f D_801C094C = { -500.0f, -100.0f, 0.0f };
    CollisionPoly* poly;
    s32 bgId;
    Vec3f sp7C;
    Vec3f sp70;
    Vec3f pos;

    D_801C094C.z = 0.0f;
    Matrix_MultVec3f(&D_801C094C, &sp7C);
    D_801C094C.z = hookshotDistance;
    Matrix_MultVec3f(&D_801C094C, &sp70);

    if (BgCheck_AnyLineTest3(&play->colCtx, &sp7C, &sp70, &pos, &poly, true, true, true, true, &bgId)) {
        if (!func_800B90AC(play, &player->actor, poly, bgId, &pos) ||
            BgCheck_ProjectileLineTest(&play->colCtx, &sp7C, &sp70, &pos, &poly, true, true, true, true, &bgId)) {
            Vec3f sp58;
            f32 sp54;
            f32 scale;

            OPEN_DISPS(play->state.gfxCtx);

            OVERLAY_DISP = Gfx_SetupDL(OVERLAY_DISP, SETUPDL_7);

            SkinMatrix_Vec3fMtxFMultXYZW(&play->viewProjectionMtxF, &pos, &sp58, &sp54);

            scale = (sp54 < 200.0f) ? 0.08f : (sp54 / 200.0f) * 0.08f;

            Matrix_Translate(pos.x, pos.y, pos.z, MTXMODE_NEW);
            Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);

            // @recomp Tag the reticle's transform.
            gEXMatrixGroupSimple(OVERLAY_DISP++, HOOKSHOT_RETICLE_TRANSFORM_ID, G_EX_PUSH, G_MTX_MODELVIEW,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);

            gSPMatrix(OVERLAY_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

            gSPSegment(OVERLAY_DISP++, 0x06, play->objectCtx.slots[player->actor.objectSlot].segment);
            gSPDisplayList(OVERLAY_DISP++, gHookshotReticleDL);

            // @recomp Pop the reticle's transform tag.
            gEXPopMatrixGroup(OVERLAY_DISP, G_MTX_MODELVIEW);

            CLOSE_DISPS(play->state.gfxCtx);
        }
    }
}


extern Gfx object_link_child_DL_017818[];

Gfx bowstring_start_hook_dl[] = {
    // One command worth of space to copy the command that was overwritten.
    gsDPNoOp(),
    // Two commands worth of space for the gEXMatrixGroup.
    gsDPNoOp(),
    gsDPNoOp(),
    
    gsSPMatrix(&gIdentityMtx, G_MTX_MODELVIEW | G_MTX_NOPUSH | G_MTX_MUL),
    // Jump back to the original DL.
    gsSPBranchList(object_link_child_DL_017818 + 1),
};

Gfx bowstring_end_hook_dl[] = {
    // One command worth of space for the gEXPopMatrixGroup.
    gsDPNoOp(),
    // Return from the displaylist.
    gsSPEndDisplayList(),
};

RECOMP_PATCH void Player_DrawGameplay(PlayState* play, Player* this, s32 lod, Gfx* cullDList,
                         OverrideLimbDrawFlex overrideLimbDraw) {
    OPEN_DISPS(play->state.gfxCtx);

    gSPSegment(POLY_OPA_DISP++, 0x0C, cullDList);
    gSPSegment(POLY_XLU_DISP++, 0x0C, cullDList);
    
    // @recomp Force the closest LOD
    lod = 0;

    // @recomp If the player is a human, patch object_link_child_DL_017818 (the DL for the bowstring) with a transform tag.
    if (this->transformation == PLAYER_FORM_HUMAN) {
        gSegments[0x0C] = OS_K0_TO_PHYSICAL(cullDList);
        Gfx* dl_virtual_address = (Gfx*)Lib_SegmentedToVirtual(object_link_child_DL_017818);

        // Check if the commands have already been overwritten.
        if ((dl_virtual_address[0].words.w0 >> 24) != G_DL) {
            // Copy the first command before overwriting.
            bowstring_start_hook_dl[0] = dl_virtual_address[0];
            // Overwrite the first command with a branch.
            gSPBranchList(dl_virtual_address, OS_K0_TO_PHYSICAL(bowstring_start_hook_dl));
            Gfx* enddl_command = dl_virtual_address;
            while ((enddl_command->words.w0 >> 24) != G_ENDDL) {
                enddl_command++;
            }
            // Overwrite the last command with a branch.
            gSPBranchList(enddl_command, bowstring_end_hook_dl);
            // Write the transform tag command. Use simple interpolation to avoid issues from decomposition failure due to a scale of zero.
            gEXMatrixGroupSimple(&bowstring_start_hook_dl[1], BOWSTRING_TRANSFORM_ID, G_EX_PUSH, G_MTX_MODELVIEW,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);
            // Write the pop group command.
            gEXPopMatrixGroup(&bowstring_end_hook_dl[0], G_MTX_MODELVIEW);
        }
    }

    Player_DrawImpl(play, this->skelAnime.skeleton, this->skelAnime.jointTable, this->skelAnime.dListCount, lod,
                    this->transformation, 0, this->actor.shape.face, overrideLimbDraw, Player_PostLimbDrawGameplay,
                    &this->actor);

    CLOSE_DISPS(play->state.gfxCtx);
}

