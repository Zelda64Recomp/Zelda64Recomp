#include "patches.h"
#include "input.h"
#include "z64quake.h"
#if 0
RecompCameraMode recomp_camera_mode = RECOMP_CAMERA_NORMAL;

VecGeo recomp_camera_pos = { .r = 66.0f, .pitch = 0, .yaw = 0 };

float recomp_camera_yaw_vel = 0.0f;
float recomp_camera_pitch_vel = 0.0f;


float recomp_deadzone = 0.2f;
float recomp_camera_x_sensitivity = 1500.0f;
float recomp_camera_y_sensitivity = 500.0f;
// float recomp_camera_acceleration = 500.0f;

void update_recomp_camera_params(Camera* camera) {
    recomp_camera_pos.yaw = Math_Atan2S(-camera->at.x + camera->eye.x, -camera->at.z + camera->eye.z);
    // recomp_printf("Camera at: %.2f %.2f %.2f\n"
    //               "      eye: %.2f %.2f %.2f\n"
    //               "      yaw: %d",
    //               camera->at.x, camera->at.y, camera->at.z,
    //               camera->eye.x, camera->eye.y, camera->eye.z,
    //               recomp_camera_pos.yaw);

    float input_x, input_y;
    recomp_get_camera_inputs(&input_x, &input_y);

    // Math_StepToF(&recomp_camera_yaw_vel, input_x * recomp_camera_x_sensitivity, recomp_camera_acceleration);
    // Math_StepToF(&recomp_camera_pitch_vel, input_y * recomp_camera_y_sensitivity, recomp_camera_acceleration);
    if (fabsf(input_x) > recomp_deadzone) {
        recomp_camera_yaw_vel = input_x * recomp_camera_x_sensitivity;
    }
    else {
        recomp_camera_yaw_vel = 0;
    }
    
    if (fabsf(input_y) > recomp_deadzone) {
        recomp_camera_pitch_vel = input_y * recomp_camera_y_sensitivity;
    }
    else {
        recomp_camera_pitch_vel = 0;
    }

    recomp_camera_pos.pitch += recomp_camera_pitch_vel;
    recomp_camera_pos.yaw += recomp_camera_yaw_vel;
}

extern s32 sUpdateCameraDirection;
static s32 sIsFalse = false;
extern s32 sCameraInitSceneTimer;

extern s16 sCameraNextUID;
extern s32 sCameraInterfaceFlags;
extern s32 sCameraHudVisibility;
extern s32 sCameraLetterboxSize;
extern s32 sCameraNegOne1;

#define CAM_DATA_IS_BG (1 << 12) // if not set, then cam data is for actor cutscenes

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
} CameraSetting;

extern CameraUpdateFunc sCameraUpdateHandlers[];
extern CameraSetting sCameraSettings[];

Vec3f Camera_CalcUpVec(s16 pitch, s16 yaw, s16 roll);
f32 Camera_fabsf(f32 f);
s32 Camera_GetBgCamIndex(Camera* camera, s32* bgId, CollisionPoly* poly);
f32 Camera_GetFocalActorHeight(Camera* camera);
f32 Camera_GetRunSpeedLimit(Camera* camera);
s32 Camera_IsDekuHovering(Camera* camera);
s32 Camera_IsMountedOnHorse(Camera* camera);
s32 Camera_IsUnderwaterAsZora(Camera* camera);
s32 Camera_IsUsingZoraFins(Camera* camera);
void Camera_UpdateInterface(s32 interfaceFlags);
f32 Camera_Vec3fMagnitude(Vec3f* vec);
s32 func_800CB7CC(Camera* camera);
s32 func_800CB854(Camera* camera);

Vec3s Camera_Update(Camera* camera) {
    Vec3f viewAt;
    Vec3f viewEye;
    Vec3f viewUp;
    Vec3f focalActorPos;
    s32 bgId;
    s32 sp98;
    s32 changeCamSceneDataType;
    CollisionPoly* sp90;
    CollisionPoly* sp8C;
    f32 runSpeedLimit;
    f32 speed;
    f32 viewFov;
    DynaPolyActor* meshActor;
    PosRot focalActorPosRot;
    ShakeInfo camShake;
    Actor* focalActor = camera->focalActor;
    VecGeo sp3C;
    s16 bgCamIndex;
    s16 numQuakesApplied;
    f32 focalActorFloorHeight;

    // Camera of status CUT only updates to this point
    if (camera->status == CAM_STATUS_CUT) {
        return camera->inputDir;
    }

    sUpdateCameraDirection = false;
    sIsFalse = false;

    if (camera->play->view.unk164 == 0) {
        if (camera->focalActor != NULL) {
            // Updates camera info on the actor it's tracking

            if (camera->focalActor == &GET_PLAYER(camera->play)->actor) {
                focalActorPosRot = Actor_GetWorldPosShapeRot(camera->focalActor);
            } else {
                focalActorPosRot = Actor_GetWorld(camera->focalActor);
            }
            camera->unk_0F0.x = focalActorPosRot.pos.x - camera->focalActorPosRot.pos.x;
            camera->unk_0F0.y = focalActorPosRot.pos.y - camera->focalActorPosRot.pos.y;
            camera->unk_0F0.z = focalActorPosRot.pos.z - camera->focalActorPosRot.pos.z;

            // bg related to tracked actor
            sp98 = 0;
            if (Camera_IsMountedOnHorse(camera)) {
                if (((Player*)focalActor)->rideActor->floorPoly != NULL) {
                    sp90 = ((Player*)focalActor)->rideActor->floorPoly;
                    camera->bgId = ((Player*)focalActor)->rideActor->floorBgId;
                    camera->focalActorFloorHeight = ((Player*)focalActor)->rideActor->floorHeight;
                    sp98 = 3;
                }
            } else if (func_800CB7CC(camera)) {
                if (camera->focalActor->floorPoly != NULL) {
                    sp90 = camera->focalActor->floorPoly;
                    camera->bgId = camera->focalActor->floorBgId;
                    camera->focalActorFloorHeight = camera->focalActor->floorHeight;
                    sp98 = 1;
                }
            } else {
                focalActorPos = focalActorPosRot.pos;
                focalActorPos.y += Camera_GetFocalActorHeight(camera);
                focalActorFloorHeight = BgCheck_EntityRaycastFloor5_3(camera->play, &camera->play->colCtx, &sp90, &bgId,
                                                                      camera->focalActor, &focalActorPos);
                if (focalActorFloorHeight != BGCHECK_Y_MIN) {
                    camera->bgId = bgId;
                    camera->focalActorFloorHeight = focalActorFloorHeight;
                    sp98 = 2;
                }
            }

            if ((sp98 != 0) && (Camera_fabsf(camera->focalActorPosRot.pos.y - camera->focalActorFloorHeight) < 11.0f)) {
                meshActor = DynaPoly_GetActor(&camera->play->colCtx, camera->bgId);
                if (meshActor != NULL) {
                    camera->floorNorm.x = COLPOLY_GET_NORMAL(sp90->normal.x);
                    camera->floorNorm.y = COLPOLY_GET_NORMAL(sp90->normal.y);
                    camera->floorNorm.z = COLPOLY_GET_NORMAL(sp90->normal.z);
                    camera->unk_0F0.x -= meshActor->actor.world.pos.x - camera->meshActorPos.x;
                    camera->unk_0F0.y -= meshActor->actor.world.pos.y - camera->meshActorPos.y;
                    camera->unk_0F0.z -= meshActor->actor.world.pos.z - camera->meshActorPos.z;
                    camera->meshActorPos = meshActor->actor.world.pos;
                }
            }

            // Set camera speed
            runSpeedLimit = Camera_GetRunSpeedLimit(camera) * 1.5f;
            speed = Camera_Vec3fMagnitude(&camera->unk_0F0);
            camera->xzSpeed = OLib_ClampMaxDist(speed, runSpeedLimit);
            camera->speedRatio = OLib_ClampMaxDist(speed / runSpeedLimit, 1.8f);
            camera->focalActorPosRot = focalActorPosRot;

            if (camera->camId == CAM_ID_MAIN) {
                Camera_UpdateWater(camera);
                Camera_UpdateHotRoom(camera);
                Camera_EarthquakeDay3(camera);
                Camera_SetSwordDistortion(camera);
            }

            /**
             * This section is about updating the camera setting based on the camera scene data
             *
             */

            // If doorTimer1 is active, set CAM_STATE_10 which suppresses bg camera scene data from being read
            if (camera->doorTimer1 != 0) {
                Camera_SetStateFlag(camera, CAM_STATE_10);
            } else if (!(camera->stateFlags & CAM_STATE_2)) {
                camera->nextCamSceneDataId = -1;
            }

            changeCamSceneDataType = 0; // default to no change in the cam scene data
            bgId = camera->bgId;

            // Sets the next cam scene data Index based on the bg surface
            if ((camera->stateFlags & CAM_STATE_0) && (camera->stateFlags & CAM_STATE_2) &&
                !(camera->stateFlags & CAM_STATE_10) &&
                (!(camera->stateFlags & CAM_STATE_9) || Camera_IsUnderwaterAsZora(camera)) &&
                !(camera->stateFlags & CAM_STATE_15) && !Camera_IsMountedOnHorse(camera) &&
                !Camera_RequestGiantsMaskSetting(camera) && !Camera_IsDekuHovering(camera) && (sp98 != 0)) {

                bgCamIndex = Camera_GetBgCamIndex(camera, &bgId, sp90);
                if ((bgCamIndex != -1) && (camera->bgId == BGCHECK_SCENE)) {
                    if (Camera_IsUsingZoraFins(camera) == 0) {
                        camera->nextCamSceneDataId = bgCamIndex | CAM_DATA_IS_BG;
                    }
                }

                focalActorPos = focalActorPosRot.pos;
                focalActorPos.y += Camera_GetFocalActorHeight(camera);
                focalActorFloorHeight =
                    BgCheck_CameraRaycastFloor2(&camera->play->colCtx, &sp8C, &bgId, &focalActorPos);

                if ((focalActorFloorHeight != BGCHECK_Y_MIN) && (sp8C != sp90) && (bgId == BGCHECK_SCENE) &&
                    ((camera->focalActorFloorHeight - 2.0f) < focalActorFloorHeight)) {
                    bgCamIndex = Camera_GetBgCamIndex(camera, &bgId, sp8C);
                    if ((bgCamIndex != -1) && (bgId == BGCHECK_SCENE)) {
                        camera->nextCamSceneDataId = bgCamIndex | CAM_DATA_IS_BG;
                        changeCamSceneDataType = 1; // change cam scene data based on the bg cam data
                    }
                }
            }

            if (camera->doorTimer1 != 0) {
                camera->doorTimer1--;
                if (camera->doorTimer1 == 0) {
                    Camera_UnsetStateFlag(camera, CAM_STATE_10);
                    changeCamSceneDataType = 5; // change cam scene data based on the cutscene cam data
                }
            }

            if (((camera->camId == CAM_ID_MAIN) || (camera->stateFlags & CAM_STATE_6)) &&
                ((camera->bgId == BGCHECK_SCENE) || ((bgId == BGCHECK_SCENE) && (changeCamSceneDataType != 0))) &&
                (camera->nextCamSceneDataId != -1) && (camera->doorTimer1 == 0) &&
                ((Camera_fabsf(camera->focalActorPosRot.pos.y - camera->focalActorFloorHeight) < 11.0f) ||
                 (changeCamSceneDataType != 0)) &&
                (!(camera->stateFlags & CAM_STATE_9) || Camera_IsUnderwaterAsZora(camera))) {

                Camera_ChangeActorCsCamIndex(camera, camera->nextCamSceneDataId);
                camera->nextCamSceneDataId = -1;
                if (camera->doorTimer2 != 0) {
                    camera->doorTimer1 = camera->doorTimer2;
                    camera->doorTimer2 = 0;
                }
            }
        }

        // Camera of status WAIT only updates to this point
        if (camera->status == CAM_STATUS_WAIT) {
            return camera->inputDir;
        }

        camera->behaviorFlags = 0;
        Camera_UnsetStateFlag(camera, CAM_STATE_10 | CAM_STATE_DISABLE_MODE_CHANGE);
        Camera_SetStateFlag(camera, CAM_STATE_4);
    }

    // Call the camera update function
    sCameraUpdateHandlers[sCameraSettings[camera->setting].cameraModes[camera->mode].funcId](camera);

    // @recomp
    update_recomp_camera_params(camera);

    // Update the interface
    if (sCameraInitSceneTimer != 0) {
        sCameraInitSceneTimer--;
    }
    if (camera->status == CAM_STATUS_ACTIVE) {
        if (((sCameraInitSceneTimer != 0) || func_800CB854(camera)) && (camera->camId == CAM_ID_MAIN)) {
            // Surpresses the interface for the first few frames of a scene
            sCameraInterfaceFlags = CAM_INTERFACE_FLAGS(CAM_LETTERBOX_LARGE, CAM_HUD_VISIBILITY_NONE_ALT, 0);
            Camera_UpdateInterface(sCameraInterfaceFlags);
        } else if ((camera->play->transitionMode != TRANS_MODE_OFF) && (camera->camId != CAM_ID_MAIN)) {
            sCameraInterfaceFlags = CAM_INTERFACE_FLAGS(CAM_LETTERBOX_IGNORE, CAM_HUD_VISIBILITY_IGNORE, 0);
            Camera_UpdateInterface(sCameraInterfaceFlags);
        } else {
            Camera_UpdateInterface(sCameraInterfaceFlags);
        }
    }

    // Camera of status UNK3 only updates to this point
    if (camera->status == CAM_STATUS_UNK3) {
        return camera->inputDir;
    }

    /**
     * This section is about updating view structs from the active camera,
     * which view uses to calculate the viewing/projection matrices
     */
    numQuakesApplied = Quake_Update(camera, &camShake);

    bgId = numQuakesApplied; // required to match

    if (numQuakesApplied != 0) {
        viewAt.x = camera->at.x + camShake.atOffset.x;
        viewAt.y = camera->at.y + camShake.atOffset.y;
        viewAt.z = camera->at.z + camShake.atOffset.z;
        viewEye.x = camera->eye.x + camShake.eyeOffset.x;
        viewEye.y = camera->eye.y + camShake.eyeOffset.y;
        viewEye.z = camera->eye.z + camShake.eyeOffset.z;
        sp3C = OLib_Vec3fDiffToVecGeo(&viewEye, &viewAt);
        viewUp = Camera_CalcUpVec(sp3C.pitch, sp3C.yaw, camera->roll + camShake.upRollOffset);
        viewFov = camera->fov + CAM_BINANG_TO_DEG(camShake.fovOffset);
    } else if (sIsFalse) {
        //! condition is impossible to achieve
        viewAt = camera->at;
        viewEye = camera->eye;
        sp3C = OLib_Vec3fDiffToVecGeo(&viewEye, &viewAt);
        viewUp = camera->up;
        viewFov = camera->fov;
    } else {
        viewAt = camera->at;
        viewEye = camera->eye;
        sp3C = OLib_Vec3fDiffToVecGeo(&viewEye, &viewAt);
        viewUp = Camera_CalcUpVec(sp3C.pitch, sp3C.yaw, camera->roll);
        viewFov = camera->fov;
    }

    // set view up
    if (camera->viewFlags & CAM_VIEW_UP) {
        camera->viewFlags &= ~CAM_VIEW_UP;
        viewUp = camera->up;
    } else {
        camera->up = viewUp;
    }

    camera->quakeOffset = camShake.eyeOffset;
    View_SetScale(&camera->play->view, (OREG(67) * 0.01f) + 1.0f);
    camera->play->view.fovy = viewFov;
    View_LookAt(&camera->play->view, &viewEye, &viewAt, &viewUp);
    camera->camDir.x = sp3C.pitch;
    camera->camDir.y = sp3C.yaw;
    camera->camDir.z = 0;

    if (!sUpdateCameraDirection) {
        camera->inputDir.x = sp3C.pitch;
        camera->inputDir.y = sp3C.yaw;
        camera->inputDir.z = 0;
    }

    return camera->inputDir;
}

extern SwingAnimation D_801EDC30[4];
s32 Camera_CalcAtDefault(Camera* camera, VecGeo* eyeAtDir, f32 yOffset, s16 calcSlope);
s32 Camera_CalcAtForNormal1(Camera* camera, VecGeo* arg1, f32 yOffset, f32 forwardDist);
s32 Camera_CalcAtForScreen(Camera* camera, VecGeo* eyeAtDir, f32 yOffset, f32* focalActorPosY, f32 deltaYMax);
s16 Camera_CalcDefaultPitch(Camera* camera, s16 pitch, s16 flatSurfacePitchTarget, s16 slopePitchAdj);
void Camera_CalcDefaultSwing(Camera* camera, VecGeo* arg1, VecGeo* arg2, f32 arg3, f32 arg4, SwingAnimation* swing2,
                             s16* flags);
s16 Camera_CalcDefaultYaw(Camera* camera, s16 yaw, s16 target, f32 attenuationYawDiffRange,
                          f32 attenuationYawDiffInterpParam);
f32 Camera_ClampDist1(Camera* camera, f32 dist, f32 minDist, f32 maxDist, s16 timer);
f32 Camera_ClampDist2(Camera* camera, f32 dist, f32 minDist, f32 maxDist, s16 timer);
f32 Camera_ClampLerpScale(Camera* camera, f32 maxLerpScale);
s16 Camera_GetPitchAdjFromFloorHeightDiffs(Camera* camera, s16 viewYaw, s16 shouldInit);
f32 Camera_ScaledStepToCeilF(f32 target, f32 cur, f32 stepScale, f32 minDiff);
s16 Camera_ScaledStepToCeilS(s16 target, s16 cur, f32 stepScale, s16 minDiff);
void Camera_SetUpdateRatesFastYaw(Camera* camera);
s32 func_800CB924(Camera* camera);
s32 func_800CB950(Camera* camera);
s32 func_800CBA7C(Camera* camera);

#define GET_NEXT_RO_DATA(values) ((values++)->val)
#define RELOAD_PARAMS(camera) ((camera->animState == 0) || (camera->animState == 10) || (camera->animState == 20))

s32 Camera_Normal1(Camera* camera) {
    Vec3f* eye = &camera->eye;
    Vec3f* at = &camera->at;
    Vec3f* eyeNext = &camera->eyeNext;
    Vec3f spD8;
    f32 spD4;
    f32 spD0;
    Vec3f* temp;
    f32 spC8;
    f32 spC4;
    f32 spC0;
    f32 phi_f0_4;
    VecGeo spB4;
    VecGeo spAC;
    VecGeo spA4;
    VecGeo sp9C;
    PosRot* sp40 = &camera->focalActorPosRot;
    Normal1ReadOnlyData* roData = &camera->paramData.norm1.roData;
    Normal1ReadWriteData* rwData = &camera->paramData.norm1.rwData;
    s16 phi_v1_2;
    s16 temp_a0_3;
    f32 sp88 = Camera_GetFocalActorHeight(camera);
    CameraModeValue* values = sCameraSettings[camera->setting].cameraModes[camera->mode].values;
    f32 phi_f2;
    f32 rand;

    roData->unk_00 = GET_NEXT_RO_DATA(values) * (sp88 * 0.01f * (0.8f - ((68.0f / sp88) * -0.2f)));
    roData->unk_04 = GET_NEXT_RO_DATA(values) * (sp88 * 0.01f * (0.8f - ((68.0f / sp88) * -0.2f)));
    roData->unk_08 = GET_NEXT_RO_DATA(values) * (sp88 * 0.01f * (0.8f - ((68.0f / sp88) * -0.2f)));
    roData->unk_04 = roData->unk_08 - (roData->unk_08 - roData->unk_04);

    if (RELOAD_PARAMS(camera)) {
        roData->unk_20 = CAM_DEG_TO_BINANG(GET_NEXT_RO_DATA(values));
        roData->unk_0C = GET_NEXT_RO_DATA(values);
        roData->unk_0C = 40.0f - (40.0f - roData->unk_0C);
        roData->unk_10 = GET_NEXT_RO_DATA(values);
        roData->unk_14 = GET_NEXT_RO_DATA(values) * 0.01f;
        roData->unk_14 = 1.0f - (1.0f - roData->unk_14);
        roData->unk_18 = GET_NEXT_RO_DATA(values);
        roData->unk_1C = GET_NEXT_RO_DATA(values) * 0.01f;
        roData->interfaceFlags = GET_NEXT_RO_DATA(values);
    }

    sCameraInterfaceFlags = roData->interfaceFlags;

    spA4 = OLib_Vec3fDiffToVecGeo(at, eye);
    sp9C = OLib_Vec3fDiffToVecGeo(at, eyeNext);

    switch (camera->animState) {
        case 20:
            Camera_SetUpdateRatesFastYaw(camera);
            // fallthrough
        case 0:
            rwData->unk_0C = 1;
            if (!(roData->interfaceFlags & NORMAL1_FLAG_3) && (camera->animState != 20)) {
                rwData->unk_0C |= 0x1000;
            }
            // fallthrough
        case 10:
            if (camera->animState == 10) {
                rwData->unk_0C = 0;
            }
            rwData->unk_08 = 0;
            D_801EDC30[camera->camId].yaw = D_801EDC30[camera->camId].pitch = D_801EDC30[camera->camId].unk_64 = 0;
            rwData->unk_0A = 0x514;
            D_801EDC30[camera->camId].swingUpdateRate = roData->unk_0C;
            rwData->unk_00 = sp40->pos.y;
            rwData->unk_04 = camera->xzSpeed;
            D_801EDC30[camera->camId].timer = 0;
            sUpdateCameraDirection = false;
            rwData->unk_10 = 120.0f;
            break;

        default:
            break;
    }

    camera->animState = 1;
    sUpdateCameraDirection = true;

    if ((camera->speedRatio < 0.01f) || (rwData->unk_0A > 0x4B0)) {
        if (rwData->unk_0A > -0x4B0) {
            rwData->unk_0A--;
        }
    } else {
        rwData->unk_0A = 0x4B0;
    }

    if (func_800CB950(camera)) {
        rwData->unk_00 = sp40->pos.y;
    }

    if (rwData->unk_0C & 0x1000) {
        spC8 = camera->speedRatio;
    } else {
        spC8 = ((camera->speedRatio * 3.0f) + 1.0f) * 0.25f;
    }

    spD8 = camera->focalActorAtOffset;
    spD8.y -= sp88 + roData->unk_00;
    spC4 = Camera_Vec3fMagnitude(&spD8);

    if ((roData->unk_04 + roData->unk_08) < spC4) {
        spC4 = 1.0f;
    } else {
        spC4 = spC4 / (roData->unk_04 + roData->unk_08);
    }

    spD0 = 0.2f;

    phi_f0_4 = (camera->xzSpeed - rwData->unk_04) * (0.2f * 1.0f);
    if (phi_f0_4 < 0.0f) {
        phi_f0_4 = 0.0f;
    }

    spC0 = OLib_ClampMaxDist(SQ(phi_f0_4), 1.0f);

    camera->yOffsetUpdateRate =
        Camera_ScaledStepToCeilF(0.05f, camera->yOffsetUpdateRate, (0.5f * spC8) + (0.5f * spC4), 0.0001f);
    camera->xzOffsetUpdateRate =
        Camera_ScaledStepToCeilF(0.05f, camera->xzOffsetUpdateRate, (0.5f * spC8) + (0.5f * spC4), 0.0001f);
    camera->fovUpdateRate =
        Camera_ScaledStepToCeilF(0.05f, camera->fovUpdateRate, (0.5f * spC8) + (0.5f * spC4), 0.0001f);

    if (D_801EDC30[camera->camId].unk_64 == 1) {
        phi_f2 = 0.5f;
    } else {
        phi_f2 = (0.5f * spC8) + (0.5f * spC4);
    }

    rwData->unk_04 = camera->xzSpeed;

    if (D_801EDC30[camera->camId].timer != 0) {
        camera->yawUpdateRateInv =
            Camera_ScaledStepToCeilF(D_801EDC30[camera->camId].swingUpdateRate + (D_801EDC30[camera->camId].timer * 2),
                                     camera->yawUpdateRateInv, phi_f2, 0.1f);
        if (roData->interfaceFlags & NORMAL1_FLAG_3) {
            camera->pitchUpdateRateInv = Camera_ScaledStepToCeilF(100.0f, camera->pitchUpdateRateInv, 0.5f, 0.1f);
        } else {
            camera->pitchUpdateRateInv = Camera_ScaledStepToCeilF((D_801EDC30[camera->camId].timer * 2) + 16.0f,
                                                                  camera->pitchUpdateRateInv, 0.2f, 0.1f);
        }
        D_801EDC30[camera->camId].timer--;
    } else {
        camera->yawUpdateRateInv = Camera_ScaledStepToCeilF(
            D_801EDC30[camera->camId].swingUpdateRate - (D_801EDC30[camera->camId].swingUpdateRate * 0.7f * spC0),
            camera->yawUpdateRateInv, phi_f2, 0.1f);
        if ((roData->interfaceFlags & NORMAL1_FLAG_3) && (camera->speedRatio > 0.01f)) {
            camera->pitchUpdateRateInv = Camera_ScaledStepToCeilF(100.0f, camera->pitchUpdateRateInv, 0.5f, 0.1f);
        } else if (D_801ED920 != NULL) {
            camera->pitchUpdateRateInv = Camera_ScaledStepToCeilF(32.0f, camera->pitchUpdateRateInv, 0.5f, 0.1f);
        } else {
            camera->pitchUpdateRateInv = Camera_ScaledStepToCeilF(16.0f, camera->pitchUpdateRateInv, 0.2f, 0.1f);
        }
    }

    if (roData->interfaceFlags & NORMAL1_FLAG_0) {
        //! FAKE:
        if (spC8) {}

        temp_a0_3 = Camera_GetPitchAdjFromFloorHeightDiffs(camera, spA4.yaw + 0x8000, rwData->unk_0C & 1);
        phi_f2 = (1.0f / roData->unk_10) * 0.7f;
        spD0 = (1.0f / roData->unk_10) * 0.3f * (1.0f - camera->speedRatio);
        rwData->unk_08 = Camera_ScaledStepToCeilS(temp_a0_3, rwData->unk_08, phi_f2 + spD0, 5);
    } else {
        rwData->unk_08 = 0;
    }

    if ((D_801EDC30[camera->camId].unk_64 == 1) && (roData->unk_00 > -40.0f)) {
        spD0 = Math_SinS(D_801EDC30[camera->camId].pitch);
        phi_f2 = (-40.0f * spD0) + roData->unk_00 * (1.0f - spD0);
        camera->yawUpdateRateInv = 80.0f;
        camera->pitchUpdateRateInv = 80.0f;
    } else {
        phi_f2 = roData->unk_00;
    }

    if (roData->interfaceFlags & (NORMAL1_FLAG_6 | NORMAL1_FLAG_5)) {
        if (camera->dist < roData->unk_04) {
            spD0 = 0.0f;
        } else if (roData->unk_08 < camera->dist) {
            spD0 = 1.0f;
        } else if (roData->unk_08 == roData->unk_04) {
            spD0 = 1.0f;
        } else {
            spD0 = (camera->dist - roData->unk_04) / (roData->unk_08 - roData->unk_04);
        }

        Camera_CalcAtForNormal1(camera, &sp9C, phi_f2, 25.0f * spD0 * camera->speedRatio);
        rwData->unk_10 = 120.0f;
    } else if ((roData->interfaceFlags & NORMAL1_FLAG_7) && (rwData->unk_0A < 0)) {
        phi_f0_4 = rwData->unk_0A / -1200.0f;
        Camera_CalcAtForNormal1(
            camera, &sp9C, phi_f2 - ((phi_f2 - ((0.8f - ((68.0f / sp88) * -0.2f)) * sp88 * -0.45f)) * phi_f0_4 * 0.75f),
            10.0f * phi_f0_4);
        rwData->unk_10 = 120.0f;
    } else if (roData->interfaceFlags & NORMAL1_FLAG_3) {
        Camera_CalcAtForScreen(camera, &sp9C, roData->unk_00, &rwData->unk_00, rwData->unk_10);
        if (rwData->unk_10 > 20.0f) {
            rwData->unk_10 -= 0.2f;
        }
    } else {
        Camera_CalcAtDefault(camera, &sp9C, phi_f2, roData->interfaceFlags & NORMAL1_FLAG_0);
        rwData->unk_10 = 120.0f;
    }

    spB4 = OLib_Vec3fDiffToVecGeo(at, eyeNext);

    if ((roData->interfaceFlags & NORMAL1_FLAG_7) && (rwData->unk_0A < 0)) {
        if (camera->focalActor == &GET_PLAYER(camera->play)->actor) {
            switch (((Player*)camera->focalActor)->transformation) {
                case PLAYER_FORM_HUMAN:
                    spD0 = 66.0f;
                    break;

                case PLAYER_FORM_DEKU:
                    spD0 = 66.0f;
                    break;

                case PLAYER_FORM_GORON:
                    spD0 = 115.0f;
                    break;

                case PLAYER_FORM_ZORA:
                    spD0 = 115.0f;
                    break;

                case PLAYER_FORM_FIERCE_DEITY:
                    spD0 = roData->unk_04;
                    break;

                default:
                    spD0 = roData->unk_04;
                    break;
            }
        }
        phi_f0_4 = Camera_ClampDist2(camera, spB4.r, spD0, spD0, 0);
    } else if (roData->interfaceFlags & NORMAL1_FLAG_7) {
        phi_f0_4 = Camera_ClampDist2(camera, spB4.r, roData->unk_04, roData->unk_08, 1);
    } else {
        phi_f0_4 = Camera_ClampDist1(camera, spB4.r, roData->unk_04, roData->unk_08, rwData->unk_0A > 0);
    }

    camera->dist = spB4.r = phi_f0_4;

    if (D_801EDC30[camera->camId].unk_64 != 0) {
        spB4.pitch =
            Camera_ScaledStepToCeilS(D_801EDC30[camera->camId].pitch, sp9C.pitch, 1.0f / camera->yawUpdateRateInv, 5);
        spB4.yaw =
            Camera_ScaledStepToCeilS(D_801EDC30[camera->camId].yaw, sp9C.yaw, 1.0f / camera->yawUpdateRateInv, 5);
    } else if (roData->interfaceFlags & NORMAL1_FLAG_5) {
        spB4.yaw = sp9C.yaw;
        spB4.pitch = sp9C.pitch;
        camera->animState = 20;
    } else if (D_801ED920 != NULL) {
        VecGeo sp74;
        s16 sp72;
        f32 sp6C;

        //! FAKE:
        if (1) {}

        temp = &D_801ED920->world.pos;
        sp74 = OLib_Vec3fDiffToVecGeo(&sp40->pos, temp);
        sp72 = sp40->rot.y - sp74.yaw;
        // Interface and shrink-window flags
        if ((roData->interfaceFlags & 0xFF00) == 0xFF00) {
            sp6C = 1.0f;
        } else {
            sp6C = 1.0f - (ABS(sp72) / 10922.0f);
        }

        if (ABS((s16)(sp9C.yaw - sp74.yaw)) < 0x4000) {
            sp74.yaw += 0x8000;
        }

        if (!(roData->interfaceFlags & NORMAL1_FLAG_3) || !func_800CB924(camera)) {
            spB4.yaw =
                Camera_CalcDefaultYaw(camera, sp9C.yaw, (s16)(sp40->rot.y - (s16)(sp72 * sp6C)), roData->unk_14, spC0);
        }

        if (!(roData->interfaceFlags & NORMAL1_FLAG_3) || (camera->speedRatio < 0.01f)) {
            spB4.pitch = Camera_CalcDefaultPitch(camera, sp9C.pitch,
                                                 roData->unk_20 + (s16)((roData->unk_20 - sp74.pitch) * sp6C * 0.75f),
                                                 rwData->unk_08);
        }
    } else if (roData->interfaceFlags & NORMAL1_FLAG_1) {
        VecGeo sp64;

        if ((camera->speedRatio > 0.1f) || (rwData->unk_0A > 0x4B0)) {
            sp64 = OLib_Vec3fToVecGeo(&camera->unk_0F0);
            if (!(roData->interfaceFlags & NORMAL1_FLAG_3) || !func_800CB924(camera)) {
                spB4.yaw = Camera_CalcDefaultYaw(camera, sp9C.yaw, sp64.yaw, roData->unk_14, spC0);
            }
            if (!(roData->interfaceFlags & NORMAL1_FLAG_3)) {
                spB4.pitch = Camera_CalcDefaultPitch(camera, sp9C.pitch, roData->unk_20, rwData->unk_08);
            } else if ((camera->unk_0F0.y > 0.0f) && func_800CB924(camera)) {
                spB4.pitch = Camera_CalcDefaultPitch(camera, sp9C.pitch, roData->unk_20, rwData->unk_08);
            }
        } else {
            spB4.yaw = sp9C.yaw;
            spB4.pitch = sp9C.pitch;
        }
    } else {
        spB4.yaw = Camera_CalcDefaultYaw(camera, sp9C.yaw, sp40->rot.y, roData->unk_14, spC0);
        if (!(roData->interfaceFlags & NORMAL1_FLAG_3) || (camera->speedRatio < 0.1f)) {
            spB4.pitch = Camera_CalcDefaultPitch(camera, sp9C.pitch, roData->unk_20, rwData->unk_08);
        }
    }

    // @recomp
    if (recomp_camera_mode == RECOMP_CAMERA_DUALANALOG) {
        spB4.pitch = recomp_camera_pos.pitch;
        // spB4.r = recomp_camera_pos.r;
        spB4.yaw = recomp_camera_pos.yaw;
    }

    // 76.9 degrees
    if (spB4.pitch > 0x36B0) {
        spB4.pitch = 0x36B0;
    }

    // -76.9 degrees
    if (spB4.pitch < -0x36B0) {
        spB4.pitch = -0x36B0;
    }

    // @recomp
    recomp_camera_pos.pitch = spB4.pitch;

    *eyeNext = OLib_AddVecGeoToVec3f(at, &spB4);

    if ((camera->status == CAM_STATUS_ACTIVE) && !(roData->interfaceFlags & NORMAL1_FLAG_4) && (spC4 <= 0.9f)) {
        if (!func_800CBA7C(camera)) {
            CollisionPoly* sp60;
            s32 sp5C; // bgId
            f32 sp58;

            Camera_CalcDefaultSwing(camera, &spB4, &sp9C, roData->unk_04, roData->unk_0C, &D_801EDC30[camera->camId],
                                    &rwData->unk_0C);
            sp58 = BgCheck_CameraRaycastFloor2(&camera->play->colCtx, &sp60, &sp5C, eye);
            if ((roData->interfaceFlags & NORMAL1_FLAG_3) && func_800CB924(camera)) {
                spD0 = 25.0f;
            } else {
                spD0 = 5.0f;
            }

            phi_f2 = eye->y - sp58;
            if ((sp58 != BGCHECK_Y_MIN) && (phi_f2 < spD0)) {
                eye->y = sp58 + spD0;
            } else if ((camera->waterYPos != camera->focalActorFloorHeight) && ((eye->y - camera->waterYPos) < 5.0f) &&
                       ((eye->y - camera->waterYPos) > -5.0f)) {
                eye->y = camera->waterYPos + 5.0f;
            }
        }

        spAC = OLib_Vec3fDiffToVecGeo(eye, at);
        camera->inputDir.x = spAC.pitch;
        camera->inputDir.y = spAC.yaw;
        camera->inputDir.z = 0;

        // crit wiggle
        if (gSaveContext.save.saveInfo.playerData.health <= 0x10) {
            phi_v1_2 = ((s32)(camera->play->state.frames << 0x18) >> 0x15) & 0xFD68;
            camera->inputDir.y += phi_v1_2;
        }
    } else {
        D_801EDC30[camera->camId].swingUpdateRate = roData->unk_0C;
        D_801EDC30[camera->camId].unk_64 = 0;
        sUpdateCameraDirection = false;
        *eye = *eyeNext;
    }

    phi_f2 = (gSaveContext.save.saveInfo.playerData.health <= 0x10) ? 0.8f : 1.0f;

    // @recomp
    // // Don't zoom in on low health when dual analog is used
    // if (recomp_camera_mode == RECOMP_CAMERA_DUALANALOG) {
    //     phi_f2;
    // }

    camera->fov = Camera_ScaledStepToCeilF(roData->unk_18 * phi_f2, camera->fov, camera->fovUpdateRate, 0.1f);

    if (roData->interfaceFlags & NORMAL1_FLAG_2) {
        spD4 = Math_SinS((s16)(spA4.yaw - spB4.yaw));
        rand = Rand_ZeroOne() - 0.5f;
        camera->roll = Camera_ScaledStepToCeilS((rand * 500.0f * camera->speedRatio) + (spD4 * spD4 * spD4 * 10000.0f),
                                                camera->roll, 0.1f, 5);
    } else {
        if (gSaveContext.save.saveInfo.playerData.health <= 0x10) {
            rand = Rand_ZeroOne() - 0.5f;
            phi_v1_2 = rand * 100.0f * camera->speedRatio;
        } else {
            phi_v1_2 = 0.0f;
        }
        camera->roll = Camera_ScaledStepToCeilS(phi_v1_2, camera->roll, 0.2f, 5);
    }

    camera->atLerpStepScale = Camera_ClampLerpScale(camera, roData->unk_1C);
    rwData->unk_0C &= ~1;

    return true;
}
#endif // #if 0
