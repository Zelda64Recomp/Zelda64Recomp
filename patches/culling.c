#include "patches.h"

// Disable frustum culling for actors, but leave distance culling intact
RECOMP_PATCH s32 func_800BA2FC(PlayState* play, Actor* actor, Vec3f* projectedPos, f32 projectedW) {
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

// Disable frustum culling for bush spawning
// RECOMP_PATCH s32 EnWood02_SpawnZoneCheck(EnWood02* this, PlayState* play, Vec3f* arg2) {
//     f32 phi_f12;

//     SkinMatrix_Vec3fMtxFMultXYZW(&play->viewProjectionMtxF, arg2, &this->actor.projectedPos, &this->actor.projectedW);

//     if (this->actor.projectedW == 0.0f) {
//         phi_f12 = 1000.0f;
//     } else {
//         phi_f12 = fabsf(1.0f / this->actor.projectedW);
//     }

//     if (((-this->actor.uncullZoneScale < this->actor.projectedPos.z) &&
//          (this->actor.projectedPos.z < (this->actor.uncullZoneForward + this->actor.uncullZoneScale)) /* &&
//          (((fabsf(this->actor.projectedPos.x) - this->actor.uncullZoneScale) * phi_f12) < 1.0f)) &&
//         (((this->actor.projectedPos.y + this->actor.uncullZoneDownward) * phi_f12) > -1.0f) &&
//         (((this->actor.projectedPos.y - this->actor.uncullZoneScale) * phi_f12) < 1.0f */)) {
//         return true;
//     }
//     return false;
// }

// Disable frustum culling for grass
RECOMP_PATCH s32 func_809A9110(PlayState* play, Vec3f* pos) {
    f32 w;
    Vec3f projectedPos;

    SkinMatrix_Vec3fMtxFMultXYZW(&play->viewProjectionMtxF, pos, &projectedPos, &w);

    if ((play->projectionMtxFDiagonal.z * -130.13191f) < projectedPos.z) {
        if (w < 1.0f) {
            w = 1.0f;
        }

        // if (((fabsf(projectedPos.x) - (130.13191f * play->projectionMtxFDiagonal.x)) < w) &&
        //     ((fabsf(projectedPos.y) - (130.13191f * play->projectionMtxFDiagonal.y)) < w)) {
            return true;
        // }
    }
    return false;
}

// Replace point light glow effect with RT64 point Z test so it works in widescreen

RECOMP_PATCH void Lights_GlowCheck(PlayState* play) {
    LightNode* light = play->lightCtx.listHead;

    while (light != NULL) {
        LightPoint* params = &light->info->params.point;

        if (light->info->type == LIGHT_POINT_GLOW) {
            Vec3f worldPos;
            Vec3f projectedPos;
            f32 invW;

            worldPos.x = params->x;
            worldPos.y = params->y;
            worldPos.z = params->z;
            Actor_GetProjectedPos(play, &worldPos, &projectedPos, &invW);

            params->drawGlow = 0;

            // @recomp Enable glow as long as the projected Z position is valid to enable widescreen support.
            // The depth check will be handled via the vertex Z test extended gbi command.
            if ((projectedPos.z > 1)) {
                params->drawGlow = 1;
            }
        }

        light = light->next;
    }
}

extern Gfx gameplay_keep_DL_029CB0[];
extern Gfx gameplay_keep_DL_029CF0[];

Vtx light_test_vert = VTX(0, 0, 0,  0, 0,  0xFF, 0xFF, 0xFF, 0xFF);

RECOMP_PATCH void Lights_DrawGlow(PlayState* play) {
    Gfx* dl;
    LightPoint* params;
    LightNode* light = play->lightCtx.listHead;

    if (light != NULL) {
        OPEN_DISPS(play->state.gfxCtx);

        dl = Gfx_SetupDL65_NoCD(POLY_XLU_DISP);

        gDPSetDither(dl++, G_CD_NOISE);

        gDPSetCombineLERP(dl++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE,
                          0);

        gSPDisplayList(dl++, gameplay_keep_DL_029CB0);

        do {
            if (light->info->type == LIGHT_POINT_GLOW) {
                params = &light->info->params.point;
                if (params->drawGlow) {
                    f32 scale = SQ((f32)params->radius) * 2e-6f;

                    gDPSetPrimColor(dl++, 0, 0, params->color[0], params->color[1], params->color[2], 50);

                    Matrix_Translate(params->x, params->y, params->z, MTXMODE_NEW);
                    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);

                    gSPMatrix(dl++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW); // <--- Made these use the right DL pointer

                    // @recomp Use the vertex Z test to prevent drawing the glow if the depth check fails.
                    // This replaces the normal Z buffer read in a way that supports widescreen and isn't 1 frame out of date.
                    gSPVertex(dl++, &light_test_vert, 1, 0);
                    gEXVertexZTest(dl++, 0);
 
                    gSPDisplayList(dl++, gameplay_keep_DL_029CF0);
                    
                    gEXEndVertexZTest(dl++);
                }
            }

            light = light->next;
        } while (light != NULL);

        POLY_XLU_DISP = dl;

        CLOSE_DISPS(play->state.gfxCtx);
    }
}
