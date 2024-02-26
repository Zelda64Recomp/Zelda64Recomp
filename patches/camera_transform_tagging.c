#include "patches.h"
#include "transform_ids.h"

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

    // Simple interpolation works much better for cameras because they orbit around a focus and 
    gEXMatrixGroupSimple(POLY_OPA_DISP++, CAMERA_TRANSFORM_ID_START, G_EX_NOPUSH, G_MTX_PROJECTION,
        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);
    gEXMatrixGroupSimple(POLY_XLU_DISP++, CAMERA_TRANSFORM_ID_START, G_EX_NOPUSH, G_MTX_PROJECTION,
        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);

    CLOSE_DISPS(gfxCtx);
}
