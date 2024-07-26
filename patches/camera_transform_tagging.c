#include "patches.h"
#include "camera_patches.h"
#include "transform_ids.h"
#include "z64cutscene.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"
#include "overlays/gamestates/ovl_file_choose/z_file_select.h"

// #define PRINT_CAMERA_INFO

static bool camera_interpolation_forced = false;
static bool camera_skip_interpolation_forced = false;
static bool camera_ignore_tracking = false;
static bool in_kaleido = false;
static bool prev_in_kaleido = false;

static bool camera_skipped = false;

void set_camera_skipped(bool skipped) {
    camera_skipped = skipped;
}

void clear_camera_skipped() {
    camera_skipped = false;
}

bool camera_was_skipped() {
    return camera_skipped;
}

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

        if (play->activeCamId >= 0 && play->activeCamId < NUM_CAMS) {
            Camera *active_cam = play->cameraPtrs[play->activeCamId];
#ifdef PRINT_CAMERA_INFO
            recomp_printf("active_cam->setting %d  active_cam->mode %d play->sceneId %d\n", active_cam->setting, active_cam->mode, play->sceneId);
#endif

            // Dedicated section for workarounds where the heuristic fails to detect the large amount of movements the camera does in these particular areas.
            bool force_interpolation = false;
            if (active_cam->setting == CAM_SET_BOSS_MAJORA) {
                // Majora's Mask final fight. All cameras should transition smoothly during this fight while not in a cutscene.
                force_interpolation = true;
            }
            else if (active_cam->setting == CAM_SET_NORMAL0 || active_cam->setting == CAM_SET_DUNGEON0) {
                force_interpolation =
                    // Pirates' Fortress Moat. Pushing the switch that unlocks the fortress will cause very large camera movement.
                    play->sceneId == SCENE_TORIDE ||
                    // Z-targetting an actor.
                    active_cam->mode == CAM_MODE_FOLLOWTARGET ||
                    // Z-targetting nothing.
                    active_cam->mode == CAM_MODE_TARGET ||
                    // Z-targetting in battle.
                    active_cam->mode == CAM_MODE_BATTLE;
            }
            // TODO: This setting claims "Smoothly and gradually return camera to Player after a cutscene "CONNECT0"". It might be worth it to enable this globally regardless of the scene.
            else if (active_cam->setting == CAM_SET_CONNECT0) {
                // Stone tower and inverted stone tower. The block puzzles will cause very large camera movement after they're activated.
                force_interpolation = play->sceneId == SCENE_F40 || play->sceneId == SCENE_F41;
            }
            else if (active_cam->setting == CAM_SET_FREE0) {
                // Cutscene after Majora fight. The camera zooms out while facing the moon with a very large movement.
                force_interpolation = play->sceneId == SCENE_00KEIKOKU && play->csCtx.scriptIndex == 0 && play->csCtx.curFrame <= 98 && gSaveContext.sceneLayer == 8;
            }

            if (force_interpolation) {
                force_camera_interpolation();
            }
        }
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

RECOMP_PATCH void KaleidoScope_SetView(PauseContext* pauseCtx, f32 eyeX, f32 eyeY, f32 eyeZ) {
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


RECOMP_PATCH void FileSelect_SetView(FileSelectState* this, f32 eyeX, f32 eyeY, f32 eyeZ) {
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

    if (velocity_diff > 50.0f || at_dist > 50.0f || eye_dist > 300.0f) {
        eye_velocity.x = 0.0f;
        eye_velocity.y = 0.0f;
        eye_velocity.z = 0.0f;
        at_velocity.x = 0.0f;
        at_velocity.y = 0.0f;
        at_velocity.z = 0.0f;
        return false;
    }

    return true;
}

/**
 * Apply view to POLY_OPA_DISP, POLY_XLU_DISP (and OVERLAY_DISP if ortho)
 */
RECOMP_PATCH void View_Apply(View* view, s32 mask) {
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
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);
        gEXMatrixGroupSimple(POLY_XLU_DISP++, CAMERA_TRANSFORM_ID, G_EX_NOPUSH, G_MTX_PROJECTION,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);
    }
    else {
        // Skip interpolation for this frame.
        gEXMatrixGroupSimple(POLY_OPA_DISP++, CAMERA_TRANSFORM_ID, G_EX_NOPUSH, G_MTX_PROJECTION,
            G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);
        gEXMatrixGroupSimple(POLY_XLU_DISP++, CAMERA_TRANSFORM_ID, G_EX_NOPUSH, G_MTX_PROJECTION,
            G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);
    }

    // Record whether the camera was skipped for later use.
    set_camera_skipped(!interpolate_camera);

    camera_interpolation_forced = false;
    camera_skip_interpolation_forced = false;

    CLOSE_DISPS(gfxCtx);
}

typedef s32 (*CameraUpdateFunc)(Camera*);

typedef struct {
    /* 0x0 */ s16 val;
    /* 0x2 */ s16 param;
} CameraModeValue; // size = 0x4

typedef struct {
    /* 0x0 */ s16 funcId;
    /* 0x2 */ s16 numValues;
    /* 0x4 */ CameraModeValue* values;
} CameraMode; // size = 0x8

typedef struct {
    /* 0x0 */ u32 validModes;
    /* 0x4 */ u32 flags;
    /* 0x8 */ CameraMode* cameraModes;
} CameraSetting; // size = 0xC

extern CameraSetting sCameraSettings[];
extern s32 sCameraInterfaceFlags;

s32 Camera_BgCheck(Camera* camera, Vec3f* from, Vec3f* to);
Vec3s* Camera_GetBgCamOrActorCsCamFuncData(Camera* camera, u32 camDataId);
f32 Camera_GetFocalActorHeight(Camera* camera);
f32 Camera_ScaledStepToCeilF(f32 target, f32 cur, f32 stepScale, f32 minDiff);
s16 Camera_ScaledStepToCeilS(s16 target, s16 cur, f32 stepScale, s16 minDiff);
void Camera_ScaledStepToCeilVec3f(Vec3f* target, Vec3f* cur, f32 xzStepScale, f32 yStepScale, f32 minDiff);
void Camera_SetFocalActorAtOffset(Camera* camera, Vec3f* focalActorPos);
void Camera_SetUpdateRatesSlow(Camera* camera);
Vec3f Camera_Vec3sToVec3f(Vec3s* src);

/**
 * Used for many fixed-based camera settings i.e. camera is fixed in rotation, and often position (but not always)
 */
// @recomp Modified to not force interpolation while panning.
RECOMP_PATCH s32 Camera_Fixed1(Camera* camera) {
    s32 pad[2];
    s32 yawDiff;
    VecGeo eyeOffset;
    VecGeo eyeAtOffset;
    VecGeo sp7C;
    u32 negOne;
    Vec3f adjustedPos;
    BgCamFuncData* bgCamFuncData;
    Vec3f* eye = &camera->eye;
    Vec3f* at = &camera->at;
    PosRot* focalActorPosRot = &camera->focalActorPosRot;
    f32 focalActorHeight = Camera_GetFocalActorHeight(camera);
    CameraModeValue* values;
    PosRot* targetHome;
    PosRot* targetWorld;
    VecGeo sp44;
    Fixed1ReadOnlyData* roData = &camera->paramData.fixd1.roData;
    Fixed1ReadWriteData* rwData = &camera->paramData.fixd1.rwData;

    sp7C = OLib_Vec3fDiffToVecGeo(at, eye);

    if (!RELOAD_PARAMS(camera)) {
    } else {
        values = sCameraSettings[camera->setting].cameraModes[camera->mode].values;
        bgCamFuncData = (BgCamFuncData*)Camera_GetBgCamOrActorCsCamFuncData(camera, camera->bgCamIndex);
        rwData->eyePosRotTarget.pos = Camera_Vec3sToVec3f(&bgCamFuncData->pos);

        rwData->eyePosRotTarget.rot = bgCamFuncData->rot;
        rwData->fov = bgCamFuncData->fov;
        rwData->focalActor = camera->focalActor;

        roData->unk_00 = GET_NEXT_SCALED_RO_DATA(values) * focalActorHeight;
        roData->unk_04 = GET_NEXT_SCALED_RO_DATA(values);
        roData->fov = GET_NEXT_RO_DATA(values);
        roData->interfaceFlags = GET_NEXT_RO_DATA(values);

        if (roData->interfaceFlags & FIXED1_FLAG_4) {
            if (camera->target == NULL) {
                return false;
            }

            targetHome = &camera->target->home;
            targetWorld = &camera->target->world;

            sp44 = OLib_Vec3fDiffToVecGeo(&targetHome->pos, &rwData->eyePosRotTarget.pos);
            sp44.yaw = targetWorld->rot.y + (s16)(sp44.yaw - targetHome->rot.y);
            rwData->eyePosRotTarget.pos = OLib_AddVecGeoToVec3f(&targetWorld->pos, &sp44);
            yawDiff = (s16)(rwData->eyePosRotTarget.rot.y - targetHome->rot.y);
            rwData->eyePosRotTarget.rot.y = targetWorld->rot.y + yawDiff;
        }
    }

    negOne = -1;

    if (rwData->focalActor != camera->focalActor) {
        camera->animState = 20;
    }

    if (rwData->fov == (s32)negOne) {
        rwData->fov = roData->fov * 100;
    } else if (rwData->fov <= 360) {
        rwData->fov *= 100;
    }

    sCameraInterfaceFlags = roData->interfaceFlags;

    if (camera->animState == 0) {
        camera->animState++;
        Camera_SetUpdateRatesSlow(camera);
        if (rwData->fov != (s32)negOne) {
            roData->fov = CAM_RODATA_UNSCALE(rwData->fov);
        }

        if (bgCamFuncData->unk_0E != (s32)negOne) {
            roData->unk_04 = CAM_RODATA_UNSCALE(bgCamFuncData->unk_0E);
        }
    }

    // @recomp Camera interpolation should always apply on this mode unless something else modified it externally.
    // We check for the approach percentage as well to detect when it wants to force an interpolation to the next position.
    static Vec3f lastEye = {};
    static Vec3f lastAt = {};
    if (OLib_Vec3fDist(eye, &lastEye) < 1e-6f && OLib_Vec3fDist(at, &lastAt) < 1e-6f && (roData->unk_04 < 0.999f)) {
        force_camera_interpolation();
    }

    eyeAtOffset = OLib_Vec3fDiffToVecGeo(eye, at);
    Camera_ScaledStepToCeilVec3f(&rwData->eyePosRotTarget.pos, eye, roData->unk_04, roData->unk_04, 0.2f);
    adjustedPos = focalActorPosRot->pos;
    adjustedPos.y += focalActorHeight;
    camera->dist = OLib_Vec3fDist(&adjustedPos, eye);
    eyeOffset.r = camera->dist;
    eyeOffset.pitch =
        Camera_ScaledStepToCeilS(rwData->eyePosRotTarget.rot.x * -1, eyeAtOffset.pitch, roData->unk_04, 5);
    eyeOffset.yaw = Camera_ScaledStepToCeilS(rwData->eyePosRotTarget.rot.y, eyeAtOffset.yaw, roData->unk_04, 5);
    *at = OLib_AddVecGeoToVec3f(eye, &eyeOffset);
    camera->eyeNext = *eye;
    Camera_BgCheck(camera, eye, at);

    camera->fov = Camera_ScaledStepToCeilF(roData->fov, camera->fov, roData->unk_04, 0.1f);
    camera->roll = 0;
    camera->atLerpStepScale = 0.0f;
    Camera_SetFocalActorAtOffset(camera, &focalActorPosRot->pos);
    camera->roll = Camera_ScaledStepToCeilS(rwData->eyePosRotTarget.rot.z, camera->roll, roData->unk_04, 5);

    // @recomp
    lastEye = *eye;
    lastAt = *at;

    return 1;
}
