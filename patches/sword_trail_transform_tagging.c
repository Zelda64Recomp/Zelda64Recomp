#include "patches.h"

#define VTX_EX_T(x, y, z, s, t, cr, cg, cb, a, px, py, pz) \
    { { x, y, z }, 0, { s, t }, { cr, cg, cb, a }, {px, py, pz} }

void EffectBlure_GetComputedValues(EffectBlure* this, s32 index, f32 ratio, Vec3s* vec1, Vec3s* vec2,
                                   Color_RGBA8* color1, Color_RGBA8* color2);
void EffectBlure_DrawSimple(EffectBlure* this2, GraphicsContext* gfxCtx);
void EffectBlure_DrawSmooth(EffectBlure* this2, GraphicsContext* gfxCtx);

// @recomp Patched to interpolate the vertices towards the front of the trail section if this is the last trail being drawn currently.
RECOMP_PATCH void EffectBlure_DrawElemNoInterpolation(EffectBlure* this, EffectBlureElement* elem, s32 index,
                                         GraphicsContext* gfxCtx) {
    // @recomp Change baseVtx to a VertexEX.
    static VertexEXColor baseVtx = VTX_EX_T(/* pos */ 0, 0, 0, /* st */ 0, 0, /* color */ 255, 255, 255, 255, /* prev pos */ 0, 0, 0);
    // @recomp Change the vertex type to VertexEX.
    VertexEX* vtx;
    Vec3s sp8C;
    Vec3s sp84;
    f32 ratio;
    Color_RGBA8 sp7C;
    Color_RGBA8 sp78;
    Vec3f sp6C;
    Vec3f sp60;
    Vec3f sp54;

    OPEN_DISPS(gfxCtx);

    Math_Vec3s_ToVec3f(&sp6C, &this->elements[0].p2);

    // @recomp Debug print.
    // recomp_printf("No interpolation index %d:\n"
    //               "  Blure: calcMode %08X  flags: %04X  addAngle %04X  addAngle %04X  elemDuration %d\n"
    //               "  Element: state %08X  timer: %d  flags %04X\n",
    //     index,
    //     this->calcMode, this->flags, (u16)this->addAngleChange, (u16)this->addAngle, this->elemDuration,
    //     elem->state, elem->timer, elem->flags);

    // @recomp Allocate using the size of VertexEX instead.
    vtx = GRAPH_ALLOC(gfxCtx, 4 * sizeof(VertexEX));
    if (vtx == NULL) {
    } else {
        vtx[0].v = baseVtx;
        vtx[1].v = baseVtx;
        vtx[2].v = baseVtx;
        vtx[3].v = baseVtx;

        ratio = (f32)elem->timer / (f32)this->elemDuration;
        EffectBlure_GetComputedValues(this, index, ratio, &sp8C, &sp84, &sp7C, &sp78);

        sp60.x = sp84.x;
        sp60.y = sp84.y;
        sp60.z = sp84.z;
        Math_Vec3f_Diff(&sp60, &sp6C, &sp54);
        Math_Vec3f_Scale(&sp54, 10.0f);
        vtx[0].v.ob[0] = sp54.x;
        vtx[0].v.ob[1] = sp54.y;
        vtx[0].v.ob[2] = sp54.z;
        vtx[0].v.cn[0] = sp78.r;
        vtx[0].v.cn[1] = sp78.g;
        vtx[0].v.cn[2] = sp78.b;
        vtx[0].v.cn[3] = sp78.a;

        sp60.x = sp8C.x;
        sp60.y = sp8C.y;
        sp60.z = sp8C.z;
        Math_Vec3f_Diff(&sp60, &sp6C, &sp54);
        Math_Vec3f_Scale(&sp54, 10.0f);
        vtx[1].v.ob[0] = sp54.x;
        vtx[1].v.ob[1] = sp54.y;
        vtx[1].v.ob[2] = sp54.z;
        vtx[1].v.cn[0] = sp7C.r;
        vtx[1].v.cn[1] = sp7C.g;
        vtx[1].v.cn[2] = sp7C.b;
        vtx[1].v.cn[3] = sp7C.a;

        ratio = (f32)(elem + 1)->timer / (f32)this->elemDuration;
        EffectBlure_GetComputedValues(this, index + 1, ratio, &sp8C, &sp84, &sp7C, &sp78);

        sp60.x = sp8C.x;
        sp60.y = sp8C.y;
        sp60.z = sp8C.z;
        Math_Vec3f_Diff(&sp60, &sp6C, &sp54);
        Math_Vec3f_Scale(&sp54, 10.0f);
        vtx[2].v.ob[0] = sp54.x;
        vtx[2].v.ob[1] = sp54.y;
        vtx[2].v.ob[2] = sp54.z;
        vtx[2].v.cn[0] = sp7C.r;
        vtx[2].v.cn[1] = sp7C.g;
        vtx[2].v.cn[2] = sp7C.b;
        vtx[2].v.cn[3] = sp7C.a;

        sp60.x = sp84.x;
        sp60.y = sp84.y;
        sp60.z = sp84.z;
        Math_Vec3f_Diff(&sp60, &sp6C, &sp54);
        Math_Vec3f_Scale(&sp54, 10.0f);
        vtx[3].v.ob[0] = sp54.x;
        vtx[3].v.ob[1] = sp54.y;
        vtx[3].v.ob[2] = sp54.z;
        vtx[3].v.cn[0] = sp78.r;
        vtx[3].v.cn[1] = sp78.g;
        vtx[3].v.cn[2] = sp78.b;
        vtx[3].v.cn[3] = sp78.a;

        // @recomp Set the previous position of the first two vertices to their current position.
        vtx[0].v.obp[0] = vtx[0].v.ob[0];
        vtx[0].v.obp[1] = vtx[0].v.ob[1];
        vtx[0].v.obp[2] = vtx[0].v.ob[2];
        vtx[1].v.obp[0] = vtx[1].v.ob[0];
        vtx[1].v.obp[1] = vtx[1].v.ob[1];
        vtx[1].v.obp[2] = vtx[1].v.ob[2];

        // @recomp If this trail just spawned (timer == 2), set the previous vertex positions for the last two vertices to the positions of the last two (interpolation).
        // Otherwise, set them to the current position of the respective vertex (no interpolation).
        if (elem->timer == 2) {
            // Vertex 2 interpolates from a start position equal to the position of vertex 1.
            vtx[2].v.obp[0] = vtx[1].v.ob[0];
            vtx[2].v.obp[1] = vtx[1].v.ob[1];
            vtx[2].v.obp[2] = vtx[1].v.ob[2];
            // Vertex 3 interpolates from a start position equal to the position of vertex 0.
            vtx[3].v.obp[0] = vtx[0].v.ob[0];
            vtx[3].v.obp[1] = vtx[0].v.ob[1];
            vtx[3].v.obp[2] = vtx[0].v.ob[2];
        }
        else {
            vtx[2].v.obp[0] = vtx[2].v.ob[0];
            vtx[2].v.obp[1] = vtx[2].v.ob[1];
            vtx[2].v.obp[2] = vtx[2].v.ob[2];
            vtx[3].v.obp[0] = vtx[3].v.ob[0];
            vtx[3].v.obp[1] = vtx[3].v.ob[1];
            vtx[3].v.obp[2] = vtx[3].v.ob[2];
        }

        // @recomp Use gEXVertex in place of gSPVertex.
        gEXVertex(POLY_XLU_DISP++, vtx, 4, 0);
        gSP2Triangles(POLY_XLU_DISP++, 0, 1, 2, 0, 0, 2, 3, 0);
    }

    CLOSE_DISPS(gfxCtx);
}

// @recomp Patched to interpolate the vertices towards the front of the trail section if this is the last trail being drawn currently.
RECOMP_PATCH void EffectBlure_DrawElemHermiteInterpolation(EffectBlure* this, EffectBlureElement* elem, s32 index,
                                              GraphicsContext* gfxCtx) {
    // @recomp Change baseVtx to a VertexEX.
    static VertexEXColor baseVtx = VTX_EX_T(/* pos */ 0, 0, 0, /* st */ 0, 0, /* color */ 255, 255, 255, 255, /* prev pos */ 0, 0, 0);
    // @recomp Change the vertex type to VertexEX.
    VertexEX* vtx;
    Vec3s sp1EC;
    Vec3s sp1E4;
    f32 ratio;
    Color_RGBA8 sp1DC;
    Color_RGBA8 sp1D8;
    Vec3f sp1CC;
    Vec3f sp1C0;
    Vec3f sp1B4;
    Vec3f sp1A8;
    Color_RGBA8 sp1A4;
    Color_RGBA8 sp1A0;
    Color_RGBA8 sp19C;
    Color_RGBA8 sp198;
    Vec3f sp18C;
    Vec3f sp180;
    Vec3f sp174;
    Vec3f sp168;
    s32 i;
    Vec3f sp158;
    Vec3f sp14C;
    Color_RGBA8 sp148;
    Color_RGBA8 sp144;
    Vec3f sp138;

    // @recomp Debug print.
    // recomp_printf("Hermite interpolation index %d:\n"
    //               "  Blure: calcMode %08X  flags: %04X  addAngle %04X  addAngle %04X  elemDuration %d\n"
    //               "  Element: state %08X  timer: %d  flags %04X\n", 
    //     index,
    //     this->calcMode, this->flags, (u16)this->addAngleChange, (u16)this->addAngle, this->elemDuration,
    //     elem->state, elem->timer, elem->flags);

    OPEN_DISPS(gfxCtx);

    Math_Vec3s_ToVec3f(&sp138, &this->elements[0].p2);

    ratio = (f32)elem->timer / (f32)this->elemDuration;
    EffectBlure_GetComputedValues(this, index, ratio, &sp1EC, &sp1E4, &sp1A4, &sp1A0);
    Math_Vec3s_ToVec3f(&sp1CC, &sp1EC);
    Math_Vec3s_ToVec3f(&sp1C0, &sp1E4);

    ratio = (f32)(elem + 1)->timer / (f32)this->elemDuration;
    EffectBlure_GetComputedValues(this, index + 1, ratio, &sp1EC, &sp1E4, &sp19C, &sp198);
    Math_Vec3s_ToVec3f(&sp18C, &sp1EC);
    Math_Vec3s_ToVec3f(&sp180, &sp1E4);

    if ((elem->flags & (EFFECT_BLURE_ELEMENT_FLAG_1 | EFFECT_BLURE_ELEMENT_FLAG_2)) == EFFECT_BLURE_ELEMENT_FLAG_2) {
        Math_Vec3f_Diff(&sp18C, &sp1CC, &sp1B4);
        Math_Vec3f_Diff(&sp180, &sp1C0, &sp1A8);
    } else {
        Vec3f sp118;
        Vec3f sp10C;

        ratio = (f32)(elem - 1)->timer / (f32)this->elemDuration;
        EffectBlure_GetComputedValues(this, index - 1, ratio, &sp1EC, &sp1E4, &sp1DC, &sp1D8);
        Math_Vec3s_ToVec3f(&sp118, &sp1EC);
        Math_Vec3s_ToVec3f(&sp10C, &sp1E4);
        Math_Vec3f_Diff(&sp18C, &sp118, &sp1B4);
        Math_Vec3f_Diff(&sp180, &sp10C, &sp1A8);
    }

    Math_Vec3f_Scale(&sp1B4, 0.5f);
    Math_Vec3f_Scale(&sp1A8, 0.5f);

    if (((elem + 1)->flags & (EFFECT_BLURE_ELEMENT_FLAG_1 | EFFECT_BLURE_ELEMENT_FLAG_2)) ==
        EFFECT_BLURE_ELEMENT_FLAG_2) {
        Math_Vec3f_Diff(&sp18C, &sp1CC, &sp174);
        Math_Vec3f_Diff(&sp180, &sp1C0, &sp168);
    } else {
        Vec3f sp100;
        Vec3f spF4;

        ratio = (f32)(elem + 2)->timer / (f32)this->elemDuration;
        EffectBlure_GetComputedValues(this, index + 2, ratio, &sp1EC, &sp1E4, &sp1DC, &sp1D8);
        Math_Vec3s_ToVec3f(&sp100, &sp1EC);
        Math_Vec3s_ToVec3f(&spF4, &sp1E4);
        Math_Vec3f_Diff(&sp100, &sp1CC, &sp174);
        Math_Vec3f_Diff(&spF4, &sp1C0, &sp168);
    }

    Math_Vec3f_Scale(&sp174, 0.5f);
    Math_Vec3f_Scale(&sp168, 0.5f);

    // @recomp Allocate using the size of VertexEX instead.
    vtx = GRAPH_ALLOC(gfxCtx, 16 * sizeof(VertexEX));
    if (vtx == NULL) {
    } else {
        Math_Vec3f_Diff(&sp1CC, &sp138, &sp158);
        Math_Vec3f_Scale(&sp158, 10.0f);
        Math_Vec3f_Diff(&sp1C0, &sp138, &sp14C);
        Math_Vec3f_Scale(&sp14C, 10.0f);

        Color_RGBA8_Copy(&sp148, &sp1A4);
        Color_RGBA8_Copy(&sp144, &sp1A0);

        vtx[0].v = baseVtx;
        vtx[1].v = baseVtx;

        vtx[0].v.ob[0] = Math_FNearbyIntF(sp158.x);
        vtx[0].v.ob[1] = Math_FNearbyIntF(sp158.y);
        vtx[0].v.ob[2] = Math_FNearbyIntF(sp158.z);
        vtx[0].v.cn[0] = sp148.r;
        vtx[0].v.cn[1] = sp148.g;
        vtx[0].v.cn[2] = sp148.b;
        vtx[0].v.cn[3] = sp148.a;
        vtx[1].v.ob[0] = Math_FNearbyIntF(sp14C.x);
        vtx[1].v.ob[1] = Math_FNearbyIntF(sp14C.y);
        vtx[1].v.ob[2] = Math_FNearbyIntF(sp14C.z);
        vtx[1].v.cn[0] = sp144.r;
        vtx[1].v.cn[1] = sp144.g;
        vtx[1].v.cn[2] = sp144.b;
        vtx[1].v.cn[3] = sp144.a;

        // @recomp Set the previous position of the first two vertices to their current position.
        vtx[0].v.obp[0] = vtx[0].v.ob[0];
        vtx[0].v.obp[1] = vtx[0].v.ob[1];
        vtx[0].v.obp[2] = vtx[0].v.ob[2];
        vtx[1].v.obp[0] = vtx[1].v.ob[0];
        vtx[1].v.obp[1] = vtx[1].v.ob[1];
        vtx[1].v.obp[2] = vtx[1].v.ob[2];

        for (i = 1; i < 8; i++) {
            s32 j1 = 2 * i;
            s32 j2 = 2 * i + 1;
            Vec3f spE0;
            f32 temp_f28 = i / 7.0f;                               // t
            f32 temp_f0 = SQ(temp_f28);                            // t^2
            f32 temp_f2 = temp_f0 * temp_f28;                      // t^3
            f32 temp_f20 = temp_f2 - temp_f0;                      // t^3 - t^2
            f32 temp_f22 = temp_f2 - 2.0f * temp_f0 + temp_f28;    // t^3 - 2t^2 + t
            f32 temp_f24 = 2.0f * temp_f2 - temp_f0 * 3.0f + 1.0f; // 2t^3 - 3t^2 + 1
            f32 temp_f26 = temp_f0 * 3.0f - 2.0f * temp_f2;        // 3t^2 - 2t^3
            s32 pad1;
            s32 pad2;

            // p = (2t^3 - 3t^2 + 1)p0 + (3t^2 - 2t^3)p1 + (t^3 - 2t^2 + t)m0 + (t^3 - t^2)m1
            spE0.x = (temp_f24 * sp1CC.x) + (temp_f26 * sp18C.x) + (temp_f22 * sp1B4.x) + (temp_f20 * sp174.x);
            spE0.y = (temp_f24 * sp1CC.y) + (temp_f26 * sp18C.y) + (temp_f22 * sp1B4.y) + (temp_f20 * sp174.y);
            spE0.z = (temp_f24 * sp1CC.z) + (temp_f26 * sp18C.z) + (temp_f22 * sp1B4.z) + (temp_f20 * sp174.z);
            Math_Vec3f_Diff(&spE0, &sp138, &sp158);
            Math_Vec3f_Scale(&sp158, 10.0f);

            spE0.x = (temp_f24 * sp1C0.x) + (temp_f26 * sp180.x) + (temp_f22 * sp1A8.x) + (temp_f20 * sp168.x);
            spE0.y = (temp_f24 * sp1C0.y) + (temp_f26 * sp180.y) + (temp_f22 * sp1A8.y) + (temp_f20 * sp168.y);
            spE0.z = (temp_f24 * sp1C0.z) + (temp_f26 * sp180.z) + (temp_f22 * sp1A8.z) + (temp_f20 * sp168.z);
            Math_Vec3f_Diff(&spE0, &sp138, &sp14C);
            Math_Vec3f_Scale(&sp14C, 10.0f);

            vtx[j1].v = baseVtx;
            vtx[j2].v = baseVtx;

            vtx[j1].v.ob[0] = Math_FNearbyIntF(sp158.x);
            vtx[j1].v.ob[1] = Math_FNearbyIntF(sp158.y);
            vtx[j1].v.ob[2] = Math_FNearbyIntF(sp158.z);
            vtx[j1].v.cn[0] = func_800B0A24(sp1A4.r, sp19C.r, temp_f28);
            vtx[j1].v.cn[1] = func_800B0A24(sp1A4.g, sp19C.g, temp_f28);
            vtx[j1].v.cn[2] = func_800B0A24(sp1A4.b, sp19C.b, temp_f28);
            vtx[j1].v.cn[3] = func_800B0A24(sp1A4.a, sp19C.a, temp_f28);

            vtx[j2].v.ob[0] = Math_FNearbyIntF(sp14C.x);
            vtx[j2].v.ob[1] = Math_FNearbyIntF(sp14C.y);
            vtx[j2].v.ob[2] = Math_FNearbyIntF(sp14C.z);
            vtx[j2].v.cn[0] = func_800B0A24(sp1A0.r, sp198.r, temp_f28);
            vtx[j2].v.cn[1] = func_800B0A24(sp1A0.g, sp198.g, temp_f28);
            vtx[j2].v.cn[2] = func_800B0A24(sp1A0.b, sp198.b, temp_f28);
            vtx[j2].v.cn[3] = func_800B0A24(sp1A0.a, sp198.a, temp_f28);

            // @recomp If this trail just spawned (timer == 2), set the previous vertex positions for the remaining vertices to the positions of the first two (interpolation).
            // Otherwise, set them to the current position of the respective vertex (no interpolation).
            if (elem->timer == 2) {
                vtx[j1].v.obp[0] = vtx[0].v.ob[0];
                vtx[j1].v.obp[1] = vtx[0].v.ob[1];
                vtx[j1].v.obp[2] = vtx[0].v.ob[2];
                vtx[j2].v.obp[0] = vtx[1].v.ob[0];
                vtx[j2].v.obp[1] = vtx[1].v.ob[1];
                vtx[j2].v.obp[2] = vtx[1].v.ob[2];
            }
            else {
                vtx[j1].v.obp[0] = vtx[j1].v.ob[0];
                vtx[j1].v.obp[1] = vtx[j1].v.ob[1];
                vtx[j1].v.obp[2] = vtx[j1].v.ob[2];
                vtx[j2].v.obp[0] = vtx[j2].v.ob[0];
                vtx[j2].v.obp[1] = vtx[j2].v.ob[1];
                vtx[j2].v.obp[2] = vtx[j2].v.ob[2];
            }
        }

        // @recomp Use gEXVertex in place of gSPVertex.
        gEXVertex(POLY_XLU_DISP++, vtx, 16, 0);
        gSP2Triangles(POLY_XLU_DISP++, 0, 1, 3, 0, 0, 3, 2, 0);
        gSP2Triangles(POLY_XLU_DISP++, 2, 3, 5, 0, 2, 5, 4, 0);
        gSP2Triangles(POLY_XLU_DISP++, 4, 5, 7, 0, 4, 7, 6, 0);
        gSP2Triangles(POLY_XLU_DISP++, 6, 7, 9, 0, 6, 9, 8, 0);
        gSP2Triangles(POLY_XLU_DISP++, 8, 9, 11, 0, 8, 11, 10, 0);
        gSP2Triangles(POLY_XLU_DISP++, 10, 11, 13, 0, 10, 13, 12, 0);
        gSP2Triangles(POLY_XLU_DISP++, 12, 13, 15, 0, 12, 15, 14, 0);
    }

    CLOSE_DISPS(gfxCtx);
}

// @recomp Patched to interpolate the vertices towards the front of the trail section if this is the last trail being drawn currently.
RECOMP_PATCH void EffectBlure_Draw(void* thisx, GraphicsContext* gfxCtx) {
    EffectBlure* this = (EffectBlure*)thisx;
    // @recomp Change the vertex type to VertexEX.
    VertexEX* vtx;
    EffectBlureElement* elem;
    s32 i;
    s32 j;
    s32 phi_t2;

    OPEN_DISPS(gfxCtx);

    gSPMatrix(POLY_XLU_DISP++, &gIdentityMtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    if (this->numElements != 0) {
        if (this->flags == 0) {
            Gfx_SetupDL38_Xlu(gfxCtx);
            gDPPipeSync(POLY_XLU_DISP++);

            // @recomp Allocate using the size of VertexEX instead.
            vtx = GRAPH_ALLOC(gfxCtx, 32 * sizeof(VertexEX));
            if (vtx == NULL) {
            } else {
                j = 0;
                for (i = 0; i < this->numElements; i++) {
                    elem = &this->elements[i];

                    // @recomp Debug print.
                    // recomp_printf("Flag 0 %d:\n"
                    //               "  Blure: calcMode %08X  flags: %04X  addAngle %04X  addAngle %04X  elemDuration %d\n"
                    //               "  Element: state %08X  timer: %d  flags %04X\n", 
                    //     i,
                    //     this->calcMode, this->flags, (u16)this->addAngleChange, (u16)this->addAngle, this->elemDuration,
                    //     elem->state, elem->timer, elem->flags);

                    if (elem->state == 1) {
                        f32 ratio = (f32)elem->timer / (f32)this->elemDuration;

                        switch (this->calcMode) {
                            case 1:
                                vtx[j].v.ob[0] = func_800B09D0(elem->p1.x, elem->p2.x, ratio);
                                vtx[j].v.ob[1] = func_800B09D0(elem->p1.y, elem->p2.y, ratio);
                                vtx[j].v.ob[2] = func_800B09D0(elem->p1.z, elem->p2.z, ratio);
                                vtx[j + 1].v.ob[0] = elem->p2.x;
                                vtx[j + 1].v.ob[1] = elem->p2.y;
                                vtx[j + 1].v.ob[2] = elem->p2.z;
                                break;

                            case 2:
                                vtx[j].v.ob[0] = elem->p1.x;
                                vtx[j].v.ob[1] = elem->p1.y;
                                vtx[j].v.ob[2] = elem->p1.z;
                                vtx[j + 1].v.ob[0] = func_800B09D0(elem->p2.x, elem->p1.x, ratio);
                                vtx[j + 1].v.ob[1] = func_800B09D0(elem->p2.y, elem->p1.y, ratio);
                                vtx[j + 1].v.ob[2] = func_800B09D0(elem->p2.z, elem->p1.z, ratio);
                                break;

                            case 3:
                                ratio *= 0.5f;
                                vtx[j].v.ob[0] = func_800B09D0(elem->p1.x, elem->p2.x, ratio);
                                vtx[j].v.ob[1] = func_800B09D0(elem->p1.y, elem->p2.y, ratio);
                                vtx[j].v.ob[2] = func_800B09D0(elem->p1.z, elem->p2.z, ratio);
                                vtx[j + 1].v.ob[0] = func_800B09D0(elem->p2.x, elem->p1.x, ratio);
                                vtx[j + 1].v.ob[1] = func_800B09D0(elem->p2.y, elem->p1.y, ratio);
                                vtx[j + 1].v.ob[2] = func_800B09D0(elem->p2.z, elem->p1.z, ratio);
                                ratio *= 2.0f;
                                break;

                            case 0:
                            default:
                                vtx[j].v.ob[0] = elem->p1.x;
                                vtx[j].v.ob[1] = elem->p1.y;
                                vtx[j].v.ob[2] = elem->p1.z;
                                vtx[j + 1].v.ob[0] = elem->p2.x;
                                vtx[j + 1].v.ob[1] = elem->p2.y;
                                vtx[j + 1].v.ob[2] = elem->p2.z;
                                break;
                        }

                        // @recomp If this trail just spawned (timer == 1), set the previous vertex positions for this vertex that of the second previous vertex (interpolation).
                        // Otherwise, set them to the current position of the respective vertex (no interpolation).
                        if (elem->timer == 1 && j >= 2) {
                            vtx[j].v.obp[0] = vtx[j - 2].v.ob[0];
                            vtx[j].v.obp[1] = vtx[j - 2].v.ob[1];
                            vtx[j].v.obp[2] = vtx[j - 2].v.ob[2];
                        }
                        else {
                            vtx[j].v.obp[0] = vtx[j].v.ob[0];
                            vtx[j].v.obp[1] = vtx[j].v.ob[1];
                            vtx[j].v.obp[2] = vtx[j].v.ob[2];
                        }

                        vtx[j].v.flag = 0;
                        vtx[j].v.tc[0] = 0;
                        vtx[j].v.tc[1] = 0;
                        vtx[j].v.cn[0] = func_800B0A24(this->p1StartColor[0], this->p1EndColor[0], ratio);
                        vtx[j].v.cn[1] = func_800B0A24(this->p1StartColor[1], this->p1EndColor[1], ratio);
                        vtx[j].v.cn[2] = func_800B0A24(this->p1StartColor[2], this->p1EndColor[2], ratio);
                        vtx[j].v.cn[3] = func_800B0A24(this->p1StartColor[3], this->p1EndColor[3], ratio);
                        j++;

                        // @recomp If this trail just spawned (timer == 1), set the previous vertex positions for this vertex that of the second previous vertex (interpolation).
                        // Otherwise, set them to the current position of the respective vertex (no interpolation).
                        if (elem->timer == 1 && j >= 2) {
                            vtx[j].v.obp[0] = vtx[j - 2].v.ob[0];
                            vtx[j].v.obp[1] = vtx[j - 2].v.ob[1];
                            vtx[j].v.obp[2] = vtx[j - 2].v.ob[2];
                        }
                        else {
                            vtx[j].v.obp[0] = vtx[j].v.ob[0];
                            vtx[j].v.obp[1] = vtx[j].v.ob[1];
                            vtx[j].v.obp[2] = vtx[j].v.ob[2];
                        }

                        vtx[j].v.flag = 0;
                        vtx[j].v.tc[0] = 0;
                        vtx[j].v.tc[1] = 0;
                        vtx[j].v.cn[0] = func_800B0A24(this->p2StartColor[0], this->p2EndColor[0], ratio);
                        vtx[j].v.cn[1] = func_800B0A24(this->p2StartColor[1], this->p2EndColor[1], ratio);
                        vtx[j].v.cn[2] = func_800B0A24(this->p2StartColor[2], this->p2EndColor[2], ratio);
                        vtx[j].v.cn[3] = func_800B0A24(this->p2StartColor[3], this->p2EndColor[3], ratio);
                        j++;
                    }
                }

                phi_t2 = 0;
                j = 0;

                // @recomp Use gEXVertex in place of gSPVertex.
                gEXVertex(POLY_XLU_DISP++, vtx, 32, 0);

                for (i = 0; i < this->numElements; i++) {
                    elem = &this->elements[i];

                    if (elem->state == 0) {
                        phi_t2 = 0;
                    } else {
                        if (phi_t2 == 0) {
                            phi_t2 = 1;
                        } else {
                            gSP1Quadrangle(POLY_XLU_DISP++, j - 2, j - 1, j + 1, j, 0);

                            if (this->unkFlag == 1) {
                                phi_t2 = 0;
                            }
                        }
                        j += 2;
                    }
                }
            }
        } else if (this->drawMode <= EFF_BLURE_DRAW_MODE_SIMPLE_ALT_COLORS) {
            EffectBlure_DrawSimple(this, gfxCtx);
        } else {
            EffectBlure_DrawSmooth(this, gfxCtx);
        }
    }

    CLOSE_DISPS(gfxCtx);
}
