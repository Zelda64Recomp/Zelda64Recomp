#include "patches.h"
#include "transform_ids.h"
#include "play_patches.h"

// Needs to copy billboard state
//   Matrix_Push

// Needs to restore billboard state
//   Matrix_Pop

// Can set or reset billboard state
//   Matrix_Put
//   Matrix_ReplaceRotation
//   Matrix_MtxFCopy
//   Matrix_Mult

// Can only reset billboard state
//   Matrix_Translate in NEW
//   Matrix_Scale in NEW
//   Matrix_RotateXS in NEW
//   Matrix_RotateXF in NEW
//   Matrix_RotateXFNew
//   Matrix_RotateYS in NEW
//   Matrix_RotateYF in NEW
//   Matrix_RotateZS in NEW
//   Matrix_RotateZF in NEW
//   Matrix_RotateZYX in NEW
//   Matrix_TranslateRotateZYX in NEW
//   Matrix_SetTranslateRotateYXZ
//   Matrix_RotateAxisF in NEW
//   Matrix_RotateAxisS in NEW

#define MATRIX_STACK_SIZE 20

MtxF* play_billboard_matrix;
u8 matrix_stack_billboard_states[MATRIX_STACK_SIZE] = {0};
u8* current_billboard_state;

#define MAX_TRACKED_BILLBOARDS 2048
Mtx* tracked_billboard_matrices[MAX_TRACKED_BILLBOARDS] = {0};
u32 tracked_billboard_matrix_count = 0;

void edit_billboard_groups(PlayState* play) {
    OPEN_DISPS(play->state.gfxCtx);

    if (camera_was_skipped()) {
        // Skip rotation for the main billboard matrix.
        gEXEditGroupByAddress(POLY_XLU_DISP++, play->billboardMtx, G_EX_INTERPOLATE_DECOMPOSE, G_MTX_PUSH, G_MTX_MODELVIEW,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE,
            G_EX_ORDER_LINEAR);
        // Skip rotation for any additional tracked billboard matrices.
        for (u32 i = 0; i < tracked_billboard_matrix_count; i++) {
            gEXEditGroupByAddress(POLY_XLU_DISP++, tracked_billboard_matrices[i], G_EX_INTERPOLATE_DECOMPOSE, G_MTX_PUSH, G_MTX_MODELVIEW,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE,
                G_EX_ORDER_LINEAR);
        }
    }

    tracked_billboard_matrix_count = 0;

    CLOSE_DISPS(play->state.gfxCtx);
}

RECOMP_PATCH Mtx* Matrix_NewMtx(GraphicsContext* gfxCtx) {
    Mtx* ret = Matrix_ToMtx(GRAPH_ALLOC(gfxCtx, sizeof(Mtx)));

    if (*current_billboard_state) {
        if (tracked_billboard_matrix_count >= MAX_TRACKED_BILLBOARDS) {
            recomp_printf("Ran out of billboard matrix tracking space!\n");
        }
        else {
            tracked_billboard_matrices[tracked_billboard_matrix_count] = ret;
            tracked_billboard_matrix_count++;
        }
    }

    return ret;
}

RECOMP_PATCH void Matrix_Init(GameState* gameState) {
    sMatrixStack = THA_AllocTailAlign16(&gameState->tha, MATRIX_STACK_SIZE * sizeof(MtxF));
    sCurrentMatrix = sMatrixStack;

    // @recomp Reset the matrix stack billboard states.
    Lib_MemSet(matrix_stack_billboard_states, 0, sizeof(matrix_stack_billboard_states));
    current_billboard_state = matrix_stack_billboard_states;

    // @recomp Reset the state's billboard matrix pointer.
    play_billboard_matrix = NULL;
    tracked_billboard_matrix_count = 0;
}

void matrix_play_update(PlayState* play) {
    play_billboard_matrix = &play->billboardMtxF;
}

RECOMP_PATCH void Matrix_Push(void) {
    MtxF* prev = sCurrentMatrix;

    sCurrentMatrix++;
    Matrix_MtxFCopy(sCurrentMatrix, prev);

    // @recomp Push a new matrix stack billboard state and copy the previous.
    u8* prev_billboard = current_billboard_state;
    current_billboard_state++;
    *current_billboard_state = *prev_billboard;
}

RECOMP_PATCH void Matrix_Pop(void) {
    sCurrentMatrix--;
    // @recomp Pop the matrix stack billboard state.
    current_billboard_state--;
}

RECOMP_PATCH void Matrix_Put(MtxF* src) {
    Matrix_MtxFCopy(sCurrentMatrix, src);

    // @recomp Update the current billboard state.
    *current_billboard_state = (src == play_billboard_matrix);
}

RECOMP_PATCH void Matrix_ReplaceRotation(MtxF* mf) {
    MtxF* cmf = sCurrentMatrix;
    f32 acc;
    f32 component;
    f32 curColNorm;

    // compute the Euclidean norm of the first column of the current matrix
    acc = cmf->xx;
    acc *= acc;
    component = cmf->yx;
    acc += SQ(component);
    component = cmf->zx;
    acc += SQ(component);
    curColNorm = sqrtf(acc);

    cmf->xx = mf->xx * curColNorm;
    cmf->yx = mf->yx * curColNorm;
    cmf->zx = mf->zx * curColNorm;

    // second column
    acc = cmf->xy;
    acc *= acc;
    component = cmf->yy;
    acc += SQ(component);
    component = cmf->zy;
    acc += SQ(component);
    curColNorm = sqrtf(acc);

    cmf->xy = mf->xy * curColNorm;
    cmf->yy = mf->yy * curColNorm;
    cmf->zy = mf->zy * curColNorm;

    // third column
    acc = cmf->xz;
    acc *= acc;
    component = cmf->yz;
    acc += SQ(component);
    component = cmf->zz;
    acc += SQ(component);
    curColNorm = sqrtf(acc);

    cmf->xz = mf->xz * curColNorm;
    cmf->yz = mf->yz * curColNorm;
    cmf->zz = mf->zz * curColNorm;

    // @recomp Update the current billboard state.
    *current_billboard_state = (mf == play_billboard_matrix);
}

RECOMP_PATCH void Matrix_Mult(MtxF* mf, MatrixMode mode) {
    MtxF* cmf = Matrix_GetCurrent();

    if (mode == MTXMODE_APPLY) {
        SkinMatrix_MtxFMtxFMult(cmf, mf, cmf);

        // @recomp Update the current billboard state. Use or because multiplying another matrix won't remove the current billboarded status of the stack.
        *current_billboard_state = *current_billboard_state || (mf == play_billboard_matrix);
    } else {
        Matrix_MtxFCopy(sCurrentMatrix, mf);
        // @recomp Update the current billboard state.
        *current_billboard_state = (mf == play_billboard_matrix);
    }
}

RECOMP_PATCH void Matrix_Translate(f32 x, f32 y, f32 z, MatrixMode mode) {
    MtxF* cmf = sCurrentMatrix;
    f32 tempX;
    f32 tempY;

    if (mode == MTXMODE_APPLY) {
        tempX = cmf->xx;
        tempY = cmf->xy;
        cmf->xw += tempX * x + tempY * y + cmf->xz * z;
        tempX = cmf->yx;
        tempY = cmf->yy;
        cmf->yw += tempX * x + tempY * y + cmf->yz * z;
        tempX = cmf->zx;
        tempY = cmf->zy;
        cmf->zw += tempX * x + tempY * y + cmf->zz * z;
        tempX = cmf->wx;
        tempY = cmf->wy;
        cmf->ww += tempX * x + tempY * y + cmf->wz * z;
    } else {
        SkinMatrix_SetTranslate(cmf, x, y, z);

        // @recomp Clear the current billboard state.
        *current_billboard_state = false;
    }
}

RECOMP_PATCH void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode mode) {
    MtxF* cmf = sCurrentMatrix;

    if (mode == MTXMODE_APPLY) {
        cmf->xx *= x;
        cmf->yx *= x;
        cmf->zx *= x;
        cmf->xy *= y;
        cmf->yy *= y;
        cmf->zy *= y;
        cmf->xz *= z;
        cmf->yz *= z;
        cmf->zz *= z;
        cmf->wx *= x;
        cmf->wy *= y;
        cmf->wz *= z;
    } else {
        SkinMatrix_SetScale(cmf, x, y, z);

        // @recomp Clear the current billboard state.
        *current_billboard_state = false;
    }
}

RECOMP_PATCH void Matrix_RotateXS(s16 x, MatrixMode mode) {
    MtxF* cmf;
    f32 sin;
    f32 cos;
    f32 tempY;
    f32 tempZ;

    if (mode == MTXMODE_APPLY) {
        if (x != 0) {
            cmf = sCurrentMatrix;

            sin = Math_SinS(x);
            cos = Math_CosS(x);

            tempY = cmf->xy;
            tempZ = cmf->xz;
            cmf->xy = tempY * cos + tempZ * sin;
            cmf->xz = tempZ * cos - tempY * sin;

            tempY = cmf->yy;
            tempZ = cmf->yz;
            cmf->yy = tempY * cos + tempZ * sin;
            cmf->yz = tempZ * cos - tempY * sin;

            tempY = cmf->zy;
            tempZ = cmf->zz;
            cmf->zy = tempY * cos + tempZ * sin;
            cmf->zz = tempZ * cos - tempY * sin;

            tempY = cmf->wy;
            tempZ = cmf->wz;
            cmf->wy = tempY * cos + tempZ * sin;
            cmf->wz = tempZ * cos - tempY * sin;
        }
    } else {
        cmf = sCurrentMatrix;

        if (x != 0) {
            sin = Math_SinS(x);
            cos = Math_CosS(x);
        } else {
            sin = 0.0f;
            cos = 1.0f;
        }

        cmf->yx = 0.0f;
        cmf->zx = 0.0f;
        cmf->wx = 0.0f;
        cmf->xy = 0.0f;
        cmf->wy = 0.0f;
        cmf->xz = 0.0f;
        cmf->wz = 0.0f;
        cmf->xw = 0.0f;
        cmf->yw = 0.0f;
        cmf->zw = 0.0f;
        cmf->xx = 1.0f;
        cmf->ww = 1.0f;
        cmf->yy = cos;
        cmf->zz = cos;
        cmf->zy = sin;
        cmf->yz = -sin;

        // @recomp Clear the current billboard state.
        *current_billboard_state = false;
    }
}

RECOMP_PATCH void Matrix_RotateXF(f32 x, MatrixMode mode) {
    MtxF* cmf;
    f32 sin;
    f32 cos;
    f32 tempY;
    f32 tempZ;
    f32 zero = 0.0;
    f32 one = 1.0;

    if (mode == MTXMODE_APPLY) {
        if (x != 0) {
            cmf = sCurrentMatrix;

            sin = sinf(x);
            cos = cosf(x);

            tempY = cmf->xy;
            tempZ = cmf->xz;
            cmf->xy = tempY * cos + tempZ * sin;
            cmf->xz = tempZ * cos - tempY * sin;

            tempY = cmf->yy;
            tempZ = cmf->yz;
            cmf->yy = tempY * cos + tempZ * sin;
            cmf->yz = tempZ * cos - tempY * sin;

            tempY = cmf->zy;
            tempZ = cmf->zz;
            cmf->zy = tempY * cos + tempZ * sin;
            cmf->zz = tempZ * cos - tempY * sin;

            tempY = cmf->wy;
            tempZ = cmf->wz;
            cmf->wy = tempY * cos + tempZ * sin;
            cmf->wz = tempZ * cos - tempY * sin;
        }
    } else {
        cmf = sCurrentMatrix;

        if (x != 0) {
            sin = sinf(x);
            cos = cosf(x);
        } else {
            sin = zero;
            cos = one;
        }

        cmf->xx = one;
        cmf->yx = zero;
        cmf->zx = zero;
        cmf->wx = zero;
        cmf->xy = zero;
        cmf->yy = cos;
        cmf->zy = sin;
        cmf->wy = zero;
        cmf->xz = zero;
        cmf->yz = -sin;
        cmf->zz = cos;
        cmf->wz = zero;
        cmf->xw = zero;
        cmf->yw = zero;
        cmf->zw = zero;
        cmf->ww = one;

        // @recomp Clear the current billboard state.
        *current_billboard_state = false;
    }
}

RECOMP_PATCH void Matrix_RotateXFNew(f32 x) {
    MtxF* cmf = sCurrentMatrix;
    s32 pad[2];
    f32 sin;
    f32 cos;

    cmf->xx = 1.0f;
    cmf->yx = 0.0f;
    cmf->zx = 0.0f;
    cmf->wx = 0.0f;
    cmf->xy = 0.0f;
    cmf->wy = 0.0f;
    cmf->xz = 0.0f;
    cmf->wz = 0.0f;
    cmf->xw = 0.0f;
    cmf->yw = 0.0f;
    cmf->zw = 0.0f;
    cmf->ww = 1.0f;

    if (x != 0.0f) {
        sin = sinf(x);
        cos = cosf(x);

        cmf->yy = cos;
        cmf->zz = cos;
        cmf->yz = -sin;
        cmf->zy = sin;
    } else {
        cmf->yy = 1.0f;
        cmf->zy = 0.0f;
        cmf->yz = 0.0f;
        cmf->zz = 1.0f;
    }

    // @recomp Clear the current billboard state.
    *current_billboard_state = false;
}
RECOMP_PATCH void Matrix_RotateYS(s16 y, MatrixMode mode) {
    MtxF* cmf;
    f32 sin;
    f32 cos;
    f32 tempX;
    f32 tempZ;

    if (mode == MTXMODE_APPLY) {
        if (y != 0) {
            cmf = sCurrentMatrix;

            sin = Math_SinS(y);
            cos = Math_CosS(y);

            tempX = cmf->xx;
            tempZ = cmf->xz;
            cmf->xx = tempX * cos - tempZ * sin;
            cmf->xz = tempX * sin + tempZ * cos;

            tempX = cmf->yx;
            tempZ = cmf->yz;
            cmf->yx = tempX * cos - tempZ * sin;
            cmf->yz = tempX * sin + tempZ * cos;

            tempX = cmf->zx;
            tempZ = cmf->zz;
            cmf->zx = tempX * cos - tempZ * sin;
            cmf->zz = tempX * sin + tempZ * cos;

            tempX = cmf->wx;
            tempZ = cmf->wz;
            cmf->wx = tempX * cos - tempZ * sin;
            cmf->wz = tempX * sin + tempZ * cos;
        }
    } else {
        cmf = sCurrentMatrix;

        if (y != 0) {
            sin = Math_SinS(y);
            cos = Math_CosS(y);
        } else {
            sin = 0.0f;
            cos = 1.0f;
        }

        cmf->yx = 0.0f;
        cmf->wx = 0.0f;
        cmf->xy = 0.0f;
        cmf->zy = 0.0f;
        cmf->wy = 0.0f;
        cmf->yz = 0.0f;
        cmf->wz = 0.0f;
        cmf->xw = 0.0f;
        cmf->yw = 0.0f;
        cmf->zw = 0.0f;
        cmf->yy = 1.0f;
        cmf->ww = 1.0f;
        cmf->xx = cos;
        cmf->zz = cos;
        cmf->zx = -sin;
        cmf->xz = sin;

        // @recomp Clear the current billboard state.
        *current_billboard_state = false;
    }
}

RECOMP_PATCH void Matrix_RotateYF(f32 y, MatrixMode mode) {
    MtxF* cmf;
    f32 sin;
    f32 cos;
    f32 tempX;
    f32 tempZ;
    f32 zero = 0.0;
    f32 one = 1.0;

    if (mode == MTXMODE_APPLY) {
        if (y != 0.0f) {
            cmf = sCurrentMatrix;

            sin = sinf(y);
            cos = cosf(y);

            tempX = cmf->xx;
            tempZ = cmf->xz;
            cmf->xx = tempX * cos - tempZ * sin;
            cmf->xz = tempX * sin + tempZ * cos;

            tempX = cmf->yx;
            tempZ = cmf->yz;
            cmf->yx = tempX * cos - tempZ * sin;
            cmf->yz = tempX * sin + tempZ * cos;

            tempX = cmf->zx;
            tempZ = cmf->zz;
            cmf->zx = tempX * cos - tempZ * sin;
            cmf->zz = tempX * sin + tempZ * cos;

            tempX = cmf->wx;
            tempZ = cmf->wz;
            cmf->wx = tempX * cos - tempZ * sin;
            cmf->wz = tempX * sin + tempZ * cos;
        }
    } else {
        cmf = sCurrentMatrix;

        if (y != 0.0f) {
            sin = sinf(y);
            cos = cosf(y);
        } else {
            cos = one;
            sin = zero;
        }

        cmf->yx = zero;
        cmf->wx = zero;
        cmf->xy = zero;
        cmf->zy = zero;
        cmf->wy = zero;
        cmf->yz = zero;
        cmf->wz = zero;
        cmf->xw = zero;
        cmf->yw = zero;
        cmf->zw = zero;
        cmf->yy = one;
        cmf->ww = one;
        cmf->xx = cos;
        cmf->zz = cos;
        cmf->zx = -sin;
        cmf->xz = sin;

        // @recomp Clear the current billboard state.
        *current_billboard_state = false;
    }
}

RECOMP_PATCH void Matrix_RotateZS(s16 z, MatrixMode mode) {
    MtxF* cmf;
    f32 sin;
    f32 cos;
    f32 tempX;
    f32 tempY;
    f32 zero = 0.0;
    f32 one = 1.0;

    if (mode == MTXMODE_APPLY) {
        if (z != 0) {
            cmf = sCurrentMatrix;

            sin = Math_SinS(z);
            cos = Math_CosS(z);

            tempX = cmf->xx;
            tempY = cmf->xy;
            cmf->xx = tempX * cos + tempY * sin;
            cmf->xy = tempY * cos - tempX * sin;

            tempX = cmf->yx;
            tempY = cmf->yy;
            cmf->yx = tempX * cos + tempY * sin;
            cmf->yy = tempY * cos - tempX * sin;

            tempX = cmf->zx;
            tempY = cmf->zy;
            cmf->zx = tempX * cos + tempY * sin;
            cmf->zy = tempY * cos - tempX * sin;

            tempX = cmf->wx;
            tempY = cmf->wy;
            cmf->wx = tempX * cos + tempY * sin;
            cmf->wy = tempY * cos - tempX * sin;
        }
    } else {
        cmf = sCurrentMatrix;

        if (z != 0) {
            sin = Math_SinS(z);
            cos = Math_CosS(z);
        } else {
            sin = zero;
            cos = one;
        }

        cmf->zx = zero;
        cmf->wx = zero;
        cmf->zy = zero;
        cmf->wy = zero;
        cmf->xz = zero;
        cmf->yz = zero;
        cmf->wz = zero;
        cmf->xw = zero;
        cmf->yw = zero;
        cmf->zw = zero;
        cmf->zz = one;
        cmf->ww = one;
        cmf->xx = cos;
        cmf->yy = cos;
        cmf->yx = sin;
        cmf->xy = -sin;

        // @recomp Clear the current billboard state.
        *current_billboard_state = false;
    }
}

RECOMP_PATCH void Matrix_RotateZF(f32 z, MatrixMode mode) {
    MtxF* cmf;
    f32 sin;
    f32 cos;
    f32 tempX;
    f32 tempY;

    if (mode == MTXMODE_APPLY) {
        if (z != 0) {
            cmf = sCurrentMatrix;

            sin = sinf(z);
            cos = cosf(z);

            tempX = cmf->xx;
            tempY = cmf->xy;
            cmf->xx = tempX * cos + tempY * sin;
            cmf->xy = tempY * cos - tempX * sin;

            tempX = cmf->yx;
            tempY = cmf->yy;
            cmf->yx = tempX * cos + tempY * sin;
            cmf->yy = tempY * cos - tempX * sin;

            tempX = cmf->zx;
            tempY = cmf->zy;
            cmf->zx = tempX * cos + tempY * sin;
            cmf->zy = tempY * cos - tempX * sin;

            tempX = cmf->wx;
            tempY = cmf->wy;
            cmf->wx = tempX * cos + tempY * sin;
            cmf->wy = tempY * cos - tempX * sin;
        }
    } else {
        cmf = sCurrentMatrix;

        if (z != 0) {
            sin = sinf(z);
            cos = cosf(z);
        } else {
            sin = 0.0f;
            cos = 1.0f;
        }

        cmf->zx = 0.0f;
        cmf->wx = 0.0f;
        cmf->zy = 0.0f;
        cmf->wy = 0.0f;
        cmf->xz = 0.0f;
        cmf->yz = 0.0f;
        cmf->wz = 0.0f;
        cmf->xw = 0.0f;
        cmf->yw = 0.0f;
        cmf->zw = 0.0f;
        cmf->zz = 1.0f;
        cmf->ww = 1.0f;
        cmf->xx = cos;
        cmf->yy = cos;
        cmf->yx = sin;
        cmf->xy = -sin;

        // @recomp Clear the current billboard state.
        *current_billboard_state = false;
    }
}

RECOMP_PATCH void Matrix_RotateZYX(s16 x, s16 y, s16 z, MatrixMode mode) {
    MtxF* cmf = sCurrentMatrix;
    f32 temp1;
    f32 temp2;
    f32 sin;
    f32 cos;

    if (mode == MTXMODE_APPLY) {
        if (z != 0) { // Added in MM, OoT always follows the nonzero path
            sin = Math_SinS(z);
            cos = Math_CosS(z);

            temp1 = cmf->xx;
            temp2 = cmf->xy;
            cmf->xx = temp1 * cos + temp2 * sin;
            cmf->xy = temp2 * cos - temp1 * sin;

            temp1 = cmf->yx;
            temp2 = cmf->yy;
            cmf->yx = temp1 * cos + temp2 * sin;
            cmf->yy = temp2 * cos - temp1 * sin;

            temp1 = cmf->zx;
            temp2 = cmf->zy;
            cmf->zx = temp1 * cos + temp2 * sin;
            cmf->zy = temp2 * cos - temp1 * sin;

            temp1 = cmf->wx;
            temp2 = cmf->wy;
            cmf->wx = temp1 * cos + temp2 * sin;
            cmf->wy = temp2 * cos - temp1 * sin;
        }

        if (y != 0) {
            sin = Math_SinS(y);
            cos = Math_CosS(y);

            temp1 = cmf->xx;
            temp2 = cmf->xz;
            cmf->xx = temp1 * cos - temp2 * sin;
            cmf->xz = temp1 * sin + temp2 * cos;

            temp1 = cmf->yx;
            temp2 = cmf->yz;
            cmf->yx = temp1 * cos - temp2 * sin;
            cmf->yz = temp1 * sin + temp2 * cos;

            temp1 = cmf->zx;
            temp2 = cmf->zz;
            cmf->zx = temp1 * cos - temp2 * sin;
            cmf->zz = temp1 * sin + temp2 * cos;

            temp1 = cmf->wx;
            temp2 = cmf->wz;
            cmf->wx = temp1 * cos - temp2 * sin;
            cmf->wz = temp1 * sin + temp2 * cos;
        }

        if (x != 0) {
            sin = Math_SinS(x);
            cos = Math_CosS(x);

            temp1 = cmf->xy;
            temp2 = cmf->xz;
            cmf->xy = temp1 * cos + temp2 * sin;
            cmf->xz = temp2 * cos - temp1 * sin;

            temp1 = cmf->yy;
            temp2 = cmf->yz;
            cmf->yy = temp1 * cos + temp2 * sin;
            cmf->yz = temp2 * cos - temp1 * sin;

            temp1 = cmf->zy;
            temp2 = cmf->zz;
            cmf->zy = temp1 * cos + temp2 * sin;
            cmf->zz = temp2 * cos - temp1 * sin;

            temp1 = cmf->wy;
            temp2 = cmf->wz;
            cmf->wy = temp1 * cos + temp2 * sin;
            cmf->wz = temp2 * cos - temp1 * sin;
        }
    } else {
        SkinMatrix_SetRotateRPY(cmf, x, y, z);

        // @recomp Clear the current billboard state.
        *current_billboard_state = false;
    }
}

RECOMP_PATCH void Matrix_SetTranslateRotateYXZ(f32 x, f32 y, f32 z, Vec3s* rot) {
    MtxF* cmf = sCurrentMatrix;
    f32 sinY = Math_SinS(rot->y);
    f32 cosY = Math_CosS(rot->y);
    f32 cosTemp;
    f32 sinTemp;

    cmf->xx = cosY;
    cmf->zx = -sinY;
    cmf->xw = x;
    cmf->yw = y;
    cmf->zw = z;
    cmf->wx = 0.0f;
    cmf->wy = 0.0f;
    cmf->wz = 0.0f;
    cmf->ww = 1.0f;

    if (rot->x != 0) {
        sinTemp = Math_SinS(rot->x);
        cosTemp = Math_CosS(rot->x);

        cmf->zz = cosY * cosTemp;
        cmf->zy = cosY * sinTemp;
        cmf->xz = sinY * cosTemp;
        cmf->xy = sinY * sinTemp;
        cmf->yz = -sinTemp;
        cmf->yy = cosTemp;
    } else {
        cmf->zz = cosY;
        cmf->xz = sinY;
        cmf->yz = 0.0f;
        cmf->zy = 0.0f;
        cmf->xy = 0.0f;
        cmf->yy = 1.0f;
    }

    if (rot->z != 0) {
        sinTemp = Math_SinS(rot->z);
        cosTemp = Math_CosS(rot->z);

        sinY = cmf->xx;
        cosY = cmf->xy;
        cmf->xx = sinY * cosTemp + cosY * sinTemp;
        cmf->xy = cosY * cosTemp - sinY * sinTemp;

        sinY = cmf->zx;
        cosY = cmf->zy;
        cmf->zx = sinY * cosTemp + cosY * sinTemp;
        cmf->zy = cosY * cosTemp - sinY * sinTemp;

        cosY = cmf->yy;
        cmf->yx = cosY * sinTemp;
        cmf->yy = cosY * cosTemp;
    } else {
        cmf->yx = 0.0f;
    }

    // @recomp Clear the current billboard state.
    *current_billboard_state = false;
}

RECOMP_PATCH void Matrix_RotateAxisF(f32 angle, Vec3f* axis, MatrixMode mode) {
    MtxF* cmf;
    f32 sin;
    f32 cos;
    f32 versin;
    f32 temp1;
    f32 temp2;
    f32 temp3;
    f32 temp4;
    f32 temp5;

    if (mode == MTXMODE_APPLY) {
        if (angle != 0) {
            cmf = sCurrentMatrix;

            sin = sinf(angle);
            cos = cosf(angle);

            temp1 = cmf->xx;
            temp2 = cmf->xy;
            temp3 = cmf->xz;
            temp4 = (axis->x * temp1 + axis->y * temp2 + axis->z * temp3) * (1.0f - cos);
            cmf->xx = temp1 * cos + axis->x * temp4 + sin * (temp2 * axis->z - temp3 * axis->y);
            cmf->xy = temp2 * cos + axis->y * temp4 + sin * (temp3 * axis->x - temp1 * axis->z);
            cmf->xz = temp3 * cos + axis->z * temp4 + sin * (temp1 * axis->y - temp2 * axis->x);

            temp1 = cmf->yx;
            temp2 = cmf->yy;
            temp3 = cmf->yz;
            temp4 = (axis->x * temp1 + axis->y * temp2 + axis->z * temp3) * (1.0f - cos);
            cmf->yx = temp1 * cos + axis->x * temp4 + sin * (temp2 * axis->z - temp3 * axis->y);
            cmf->yy = temp2 * cos + axis->y * temp4 + sin * (temp3 * axis->x - temp1 * axis->z);
            cmf->yz = temp3 * cos + axis->z * temp4 + sin * (temp1 * axis->y - temp2 * axis->x);

            temp1 = cmf->zx;
            temp2 = cmf->zy;
            temp3 = cmf->zz;
            temp4 = (axis->x * temp1 + axis->y * temp2 + axis->z * temp3) * (1.0f - cos);
            cmf->zx = temp1 * cos + axis->x * temp4 + sin * (temp2 * axis->z - temp3 * axis->y);
            cmf->zy = temp2 * cos + axis->y * temp4 + sin * (temp3 * axis->x - temp1 * axis->z);
            cmf->zz = temp3 * cos + axis->z * temp4 + sin * (temp1 * axis->y - temp2 * axis->x);
        }
    } else {
        cmf = sCurrentMatrix;

        if (angle != 0) {
            sin = sinf(angle);
            cos = cosf(angle);
            versin = 1.0f - cos;

            cmf->xx = axis->x * axis->x * versin + cos;
            cmf->yy = axis->y * axis->y * versin + cos;
            cmf->zz = axis->z * axis->z * versin + cos;

            if (0) {}

            temp2 = axis->x * versin * axis->y;
            temp3 = axis->z * sin;
            cmf->yx = temp2 + temp3;
            cmf->xy = temp2 - temp3;

            temp2 = axis->x * versin * axis->z;
            temp3 = axis->y * sin;
            cmf->zx = temp2 - temp3;
            cmf->xz = temp2 + temp3;

            temp2 = axis->y * versin * axis->z;
            temp3 = axis->x * sin;
            cmf->zy = temp2 + temp3;
            cmf->yz = temp2 - temp3;

            cmf->wx = cmf->wy = cmf->wz = cmf->xw = cmf->yw = cmf->zw = 0.0f;
            cmf->ww = 1.0f;
        } else {
            cmf->xx = 1.0f;
            cmf->yx = 0.0f;
            cmf->zx = 0.0f;
            cmf->wx = 0.0f;
            cmf->xy = 0.0f;
            cmf->yy = 1.0f;
            cmf->zy = 0.0f;
            cmf->wy = 0.0f;
            cmf->xz = 0.0f;
            cmf->yz = 0.0f;
            cmf->zz = 1.0f;
            cmf->wz = 0.0f;
            cmf->xw = 0.0f;
            cmf->yw = 0.0f;
            cmf->zw = 0.0f;
            cmf->ww = 1.0f;
        }

        // @recomp Clear the current billboard state.
        *current_billboard_state = false;
    }
}

RECOMP_PATCH void Matrix_RotateAxisS(s16 angle, Vec3f* axis, MatrixMode mode) {
    MtxF* cmf;
    f32 cos;
    f32 sin;
    f32 versin;
    f32 temp1;
    f32 temp2;
    f32 temp3;
    f32 temp4;

    if (mode == MTXMODE_APPLY) {
        if (angle != 0) {
            cmf = sCurrentMatrix;

            sin = Math_SinS(angle);
            cos = Math_CosS(angle);

            temp1 = cmf->xx;
            temp2 = cmf->xy;
            temp3 = cmf->xz;
            temp4 = (axis->x * temp1 + axis->y * temp2 + axis->z * temp3) * (1.0f - cos);
            cmf->xx = temp1 * cos + axis->x * temp4 + sin * (temp2 * axis->z - temp3 * axis->y);
            cmf->xy = temp2 * cos + axis->y * temp4 + sin * (temp3 * axis->x - temp1 * axis->z);
            cmf->xz = temp3 * cos + axis->z * temp4 + sin * (temp1 * axis->y - temp2 * axis->x);

            temp1 = cmf->yx;
            temp2 = cmf->yy;
            temp3 = cmf->yz;
            temp4 = (axis->x * temp1 + axis->y * temp2 + axis->z * temp3) * (1.0f - cos);
            cmf->yx = temp1 * cos + axis->x * temp4 + sin * (temp2 * axis->z - temp3 * axis->y);
            cmf->yy = temp2 * cos + axis->y * temp4 + sin * (temp3 * axis->x - temp1 * axis->z);
            cmf->yz = temp3 * cos + axis->z * temp4 + sin * (temp1 * axis->y - temp2 * axis->x);

            temp1 = cmf->zx;
            temp2 = cmf->zy;
            temp3 = cmf->zz;
            temp4 = (axis->x * temp1 + axis->y * temp2 + axis->z * temp3) * (1.0f - cos);
            cmf->zx = temp1 * cos + axis->x * temp4 + sin * (temp2 * axis->z - temp3 * axis->y);
            cmf->zy = temp2 * cos + axis->y * temp4 + sin * (temp3 * axis->x - temp1 * axis->z);
            cmf->zz = temp3 * cos + axis->z * temp4 + sin * (temp1 * axis->y - temp2 * axis->x);
        }
    } else {
        cmf = sCurrentMatrix;

        if (angle != 0) {
            sin = Math_SinS(angle);
            cos = Math_CosS(angle);
            versin = 1.0f - cos;

            cmf->xx = axis->x * axis->x * versin + cos;
            cmf->yy = axis->y * axis->y * versin + cos;
            cmf->zz = axis->z * axis->z * versin + cos;

            if (0) {}

            temp2 = axis->x * versin * axis->y;
            temp3 = axis->z * sin;
            cmf->yx = temp2 + temp3;
            cmf->xy = temp2 - temp3;

            temp2 = axis->x * versin * axis->z;
            temp3 = axis->y * sin;
            cmf->zx = temp2 - temp3;
            cmf->xz = temp2 + temp3;

            temp2 = axis->y * versin * axis->z;
            temp3 = axis->x * sin;
            cmf->zy = temp2 + temp3;
            cmf->yz = temp2 - temp3;

            cmf->wx = cmf->wy = cmf->wz = cmf->xw = cmf->yw = cmf->zw = 0.0f;
            cmf->ww = 1.0f;
        } else {
            cmf->xx = 1.0f;
            cmf->yx = 0.0f;
            cmf->zx = 0.0f;
            cmf->wx = 0.0f;
            cmf->xy = 0.0f;
            cmf->yy = 1.0f;
            cmf->zy = 0.0f;
            cmf->wy = 0.0f;
            cmf->xz = 0.0f;
            cmf->yz = 0.0f;
            cmf->zz = 1.0f;
            cmf->wz = 0.0f;
            cmf->xw = 0.0f;
            cmf->yw = 0.0f;
            cmf->zw = 0.0f;
            cmf->ww = 1.0f;
        }

        // @recomp Clear the current billboard state.
        *current_billboard_state = false;
    }
}
