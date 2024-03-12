#include "patches.h"
#include "transform_ids.h"
#include "z64cutscene.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"
#include "overlays/gamestates/ovl_file_choose/z_file_select.h"

static bool camera_interpolation_forced = false;
static bool camera_skip_interpolation_forced = false;
static bool camera_ignore_tracking = false;
static bool in_kaleido = false;
static bool prev_in_kaleido = false;

void camera_pre_play_update(PlayState* play) {
}

void camera_post_play_update(PlayState* play) {
    // Track whether the game is in kaleido.
    prev_in_kaleido = in_kaleido;

    if ((play->pauseCtx.state != 0) || (play->pauseCtx.debugEditor != DEBUG_EDITOR_NONE)) {
        in_kaleido = true;
    }
    else {
        in_kaleido = false;
    }
}

s32 View_ApplyPerspective(View* view);
s32 View_ApplyOrtho(View* view);

void force_camera_interpolation() {
    camera_interpolation_forced = true;
}

void force_camera_skip_interpolation() {
    camera_skip_interpolation_forced = true;
}

void force_camera_ignore_tracking() {
    camera_ignore_tracking = true;
}

void KaleidoScope_SetView(PauseContext* pauseCtx, f32 eyeX, f32 eyeY, f32 eyeZ) {
    Vec3f eye;
    Vec3f at;
    Vec3f up;

    eye.x = eyeX;
    eye.y = eyeY;
    eye.z = eyeZ;
    at.x = at.y = at.z = 0.0f;
    up.x = up.z = 0.0f;
    up.y = 1.0f;

    // @recomp Force interpolation for this view and skip tracking positions.
    force_camera_interpolation();
    force_camera_ignore_tracking();

    View_LookAt(&pauseCtx->view, &eye, &at, &up);
    View_Apply(&pauseCtx->view,
               VIEW_ALL | VIEW_FORCE_VIEWING | VIEW_FORCE_VIEWPORT | VIEW_FORCE_PROJECTION_PERSPECTIVE);
}


void FileSelect_SetView(FileSelectState* this, f32 eyeX, f32 eyeY, f32 eyeZ) {
    Vec3f eye;
    Vec3f lookAt;
    Vec3f up;

    eye.x = eyeX;
    eye.y = eyeY;
    eye.z = eyeZ;

    lookAt.x = lookAt.y = lookAt.z = 0.0f;

    up.x = up.z = 0.0f;
    up.y = 1.0f;
    
    // @recomp Force interpolation for this view and skip tracking positions.
    force_camera_interpolation();
    force_camera_ignore_tracking();

    View_LookAt(&this->view, &eye, &lookAt, &up);
    View_Apply(&this->view, VIEW_ALL | VIEW_FORCE_VIEWING | VIEW_FORCE_VIEWPORT | VIEW_FORCE_PROJECTION_PERSPECTIVE);
}

bool should_interpolate_perspective(Vec3f* eye, Vec3f* at) {
    static Vec3f prev_eye = {0,0,0};
    static Vec3f prev_at = {0,0,0};
    static Vec3f eye_velocity = {0,0,0};
    static Vec3f at_velocity = {0,0,0};

    Vec3f predicted_eye;
    Vec3f predicted_at;
    // Predict the new eye and at positions based on the previous velocity and positions.
    Math_Vec3f_Sum(&prev_eye, &eye_velocity, &predicted_eye);
    Math_Vec3f_Sum(&prev_at, &at_velocity, &predicted_at);

    // Calculate the current velocities from the previous and current positions.
    Math_Vec3f_Diff(eye, &prev_eye, &eye_velocity);
    Math_Vec3f_Diff(at, &prev_at, &at_velocity);

    // Compare the predicted positions to the real positions.
    float eye_dist = Math_Vec3f_DistXYZ(&predicted_eye, eye);
    float at_dist = Math_Vec3f_DistXYZ(&predicted_at, at);

    // Compare the velocities of the eye and at positions.
    float velocity_diff = Math_Vec3f_DistXYZ(&eye_velocity, &at_velocity);

    // Update the tracking for the previous positions with the new ones.
    prev_eye = *eye;
    prev_at = *at;

    // These numbers are all picked via testing.

    // If the velocity of both positions was the same, then they're moving together and should interpolate.
    if (velocity_diff <= 3.0f && eye_dist <= 100.0f && at_dist <= 100.0f) {
        return true;
    }

    // If the focus or position are basically the same across frames and the eye didn't move too far then it should probably be interpolated.
    if (at_dist <= 10.0f && eye_dist <= 300.0f) {
        return true;
    }
    if (eye_dist <= 10.0f && at_dist <= 300.0f) {
        return true;
    }

    if (velocity_diff > 50.0f) {
        return false;
    }

    if (at_dist > 50.0f) {
        return false;
    }

    if (eye_dist > 300.0f) {
        return false;
    }

    return true;
}

/**
 * Apply view to POLY_OPA_DISP, POLY_XLU_DISP (and OVERLAY_DISP if ortho)
 */
void View_Apply(View* view, s32 mask) {
    mask = (view->flags & mask) | (mask >> 4);
    
    // @recomp Determine if the camera should be interpolated this frame.
    bool interpolate_camera = false;

    if (mask & VIEW_PROJECTION_ORTHO) {
        View_ApplyOrtho(view);
    } else {
        View_ApplyPerspective(view);

        // @recomp Determine if interpolation should occur based on the new eye and at positions.
        if (!camera_ignore_tracking) {
            interpolate_camera = should_interpolate_perspective(&view->eye, &view->at);
        }
    }
    camera_ignore_tracking = false;

    // Force skipping interpolation if the previous view was kaleido and this one isn't.
    if (prev_in_kaleido && !in_kaleido) {
        camera_skip_interpolation_forced = true;
    }

    // @recomp Apply camera interpolation overrides.
    if (camera_skip_interpolation_forced) {
        interpolate_camera = false;
    }
    else if (camera_interpolation_forced) {
        interpolate_camera = true;
    }

    // @recomp Tag the camera matrices
    GraphicsContext* gfxCtx = view->gfxCtx;
    OPEN_DISPS(gfxCtx);

    if (interpolate_camera) {
        // Simple interpolation works much better for cameras because they orbit around a focus.
        gEXMatrixGroupSimple(POLY_OPA_DISP++, CAMERA_TRANSFORM_ID, G_EX_NOPUSH, G_MTX_PROJECTION,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);
        gEXMatrixGroupSimple(POLY_XLU_DISP++, CAMERA_TRANSFORM_ID, G_EX_NOPUSH, G_MTX_PROJECTION,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);
    }
    else {
        // Skip interpolation for this frame.
        gEXMatrixGroupSimple(POLY_OPA_DISP++, CAMERA_TRANSFORM_ID, G_EX_NOPUSH, G_MTX_PROJECTION,
            G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);
        gEXMatrixGroupSimple(POLY_XLU_DISP++, CAMERA_TRANSFORM_ID, G_EX_NOPUSH, G_MTX_PROJECTION,
            G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);
    }

    camera_interpolation_forced = false;
    camera_skip_interpolation_forced = false;

    CLOSE_DISPS(gfxCtx);
}
