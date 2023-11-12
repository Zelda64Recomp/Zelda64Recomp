#include "patches.h"

// Disable frustum culling for actors, but leave distance culling intact
s32 func_800BA2FC(PlayState* play, Actor* actor, Vec3f* projectedPos, f32 projectedW) {
    if ((-actor->uncullZoneScale < projectedPos->z) &&
        (projectedPos->z < (actor->uncullZoneForward + actor->uncullZoneScale))) {
        // f32 phi_f12;
        // f32 phi_f2 = CLAMP_MIN(projectedW, 1.0f);
        // f32 phi_f14;
        // f32 phi_f16;

        // if (play->view.fovy != 60.0f) {
        //     phi_f12 = actor->uncullZoneScale * play->projectionMtxFDiagonal.x * 0.76980036f; // sqrt(16/27)

        //     phi_f14 = play->projectionMtxFDiagonal.y * 0.57735026f; // 1 / sqrt(3)
        //     phi_f16 = actor->uncullZoneScale * phi_f14;
        //     phi_f14 *= actor->uncullZoneDownward;
        // } else {
        //     phi_f16 = phi_f12 = actor->uncullZoneScale;
        //     phi_f14 = actor->uncullZoneDownward;
        // }

        // if (((fabsf(projectedPos->x) - phi_f12) < phi_f2) && ((-phi_f2 < (projectedPos->y + phi_f14))) &&
        //     ((projectedPos->y - phi_f16) < phi_f2)) {
            return true;
        // }
    }

    return false;
}

// Override LOD to 0
void Player_DrawGameplay(PlayState* play, Player* this, s32 lod, Gfx* cullDList,
                         OverrideLimbDrawFlex overrideLimbDraw) {
    OPEN_DISPS(play->state.gfxCtx);

    gSPSegment(POLY_OPA_DISP++, 0x0C, cullDList);
    gSPSegment(POLY_XLU_DISP++, 0x0C, cullDList);
    
    lod = 0; // Force the closest LOD

    Player_DrawImpl(play, this->skelAnime.skeleton, this->skelAnime.jointTable, this->skelAnime.dListCount, lod,
                    this->transformation, 0, this->actor.shape.face, overrideLimbDraw, Player_PostLimbDrawGameplay,
                    &this->actor);

    CLOSE_DISPS(play->state.gfxCtx);
}
