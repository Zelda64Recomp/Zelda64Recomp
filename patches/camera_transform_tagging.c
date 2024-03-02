#include "patches.h"
#include "transform_ids.h"
#include "z64cutscene.h"

static bool interpolate_camera = true;

s32 Play_ChangeCameraStatus(PlayState* this, s16 camId, s16 status) {
    s16 camIdx = (camId == CAM_ID_NONE) ? this->activeCamId : camId;

    if (status == CAM_STATUS_ACTIVE) {
        this->activeCamId = camIdx;
    }

    recomp_printf("Changed play camera status %d %d\n", camId, status);

    return Camera_ChangeStatus(this->cameraPtrs[camIdx], status);
}

s32 View_ApplyPerspective(View* view);
s32 View_ApplyOrtho(View* view);

/**
 * Apply view to POLY_OPA_DISP, POLY_XLU_DISP (and OVERLAY_DISP if ortho)
 */
void View_Apply(View* view, s32 mask) {
    mask = (view->flags & mask) | (mask >> 4);

    if (mask & VIEW_PROJECTION_ORTHO) {
        View_ApplyOrtho(view);
    } else {
        View_ApplyPerspective(view);
    }
    
    // @recomp Tag the camera matrices
    GraphicsContext* gfxCtx = view->gfxCtx;
    OPEN_DISPS(gfxCtx);

    if (interpolate_camera) {
        // Simple interpolation works much better for cameras because they orbit around a focus.
        gEXMatrixGroupSimple(POLY_OPA_DISP++, CAMERA_TRANSFORM_ID_START, G_EX_NOPUSH, G_MTX_PROJECTION,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);
        gEXMatrixGroupSimple(POLY_XLU_DISP++, CAMERA_TRANSFORM_ID_START, G_EX_NOPUSH, G_MTX_PROJECTION,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);
    }
    else {
        // Skip interpolation for this frame.
        gEXMatrixGroupSimple(POLY_OPA_DISP++, CAMERA_TRANSFORM_ID_START, G_EX_NOPUSH, G_MTX_PROJECTION,
            G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);
        gEXMatrixGroupSimple(POLY_XLU_DISP++, CAMERA_TRANSFORM_ID_START, G_EX_NOPUSH, G_MTX_PROJECTION,
            G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);
        recomp_printf("Skipped camera interpolation\n");
    }

    interpolate_camera = true;

    CLOSE_DISPS(gfxCtx);
}
