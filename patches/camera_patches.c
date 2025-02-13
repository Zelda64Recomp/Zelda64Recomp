#include "patches.h"
#include "input.h"
#include "z64quake.h"
#include "play_patches.h"
#include "camera_patches.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"
#include "overlays/actors/ovl_Boss_04/z_boss_04.h"
#include "overlays/actors/ovl_En_Clear_Tag/z_en_clear_tag.h"
#include "z64shrink_window.h"
#include "z64player.h"

static bool camera_fixes = false;

static bool prev_analog_cam_active = false;
static bool can_use_analog_cam = false;
static bool analog_cam_active = false;
static bool analog_cam_skip_once = false;

VecGeo analog_camera_pos = { .r = 66.0f, .pitch = 0, .yaw = 0 };

float analog_camera_yaw_vel = 0.0f;
float analog_camera_pitch_vel = 0.0f;

float analog_deadzone = 0.2f;
float analog_camera_x_sensitivity = 1500.0f;
float analog_camera_y_sensitivity = 500.0f;
// float analog_camera_acceleration = 500.0f;

static const float analog_cam_threshold = 0.1f;

RECOMP_EXPORT void recomp_set_camera_fixes(bool new_val) {
    camera_fixes = new_val;
}

void update_analog_camera_params(Camera* camera) {
    // recomp_printf("Camera at: %.2f %.2f %.2f\n"
    //               "      eye: %.2f %.2f %.2f\n"
    //               "      yaw: %d",
    //               camera->at.x, camera->at.y, camera->at.z,
    //               camera->eye.x, camera->eye.y, camera->eye.z,
    //               analog_camera_pos.yaw);
    // Check if the analog camera was usable in this frame.
    if (!can_use_analog_cam) {
        // It wasn't, so mark the analog cam as being off for this frame.
        analog_cam_active = false;
    }
    prev_analog_cam_active = analog_cam_active;
    if (!analog_cam_active) {
        // recomp_printf("Analog cam not active\n");
        analog_camera_pos.yaw = Math_Atan2S(camera->eye.x - camera->at.x, camera->eye.z - camera->at.z);
        analog_camera_pos.pitch = Math_Vec3f_Pitch(&camera->eye, &camera->at);
    }
}

void update_analog_cam(Camera* c) {
    can_use_analog_cam = true;

    Player* player = GET_PLAYER(c->play);
    // recomp_printf("  val: %d\n", func_80123434(player));

    // Check if the player just started Z targeting and reset to auto cam if so.
    static bool prev_targeting_held = false;
    bool targeting_held = func_80123434(player) || player->lockOnActor != NULL;
    if (targeting_held && !prev_targeting_held) {
        analog_cam_active = false;
    }

    // Enable analog cam if the right stick is held.
    float input_x, input_y;
    recomp_get_camera_inputs(&input_x, &input_y);

    if (fabsf(input_x) >= analog_cam_threshold || fabsf(input_y) >= analog_cam_threshold) {
        analog_cam_active = true;
    }

    if (analog_cam_skip_once) {
        analog_cam_active = false;
        analog_cam_skip_once = false;
    }

    // Record the Z targeting state.
    prev_targeting_held = targeting_held;

    if (analog_cam_active) {
        s32 inverted_x, inverted_y;
        recomp_get_analog_inverted_axes(&inverted_x, &inverted_y);

        if (inverted_x) {
            input_x = -input_x;
        }

        if (inverted_y) {
            input_y = -input_y;
        }

        analog_camera_yaw_vel = -input_x * analog_camera_x_sensitivity;
        analog_camera_pitch_vel = input_y * analog_camera_y_sensitivity;

        analog_camera_pos.pitch += analog_camera_pitch_vel;
        analog_camera_pos.yaw += analog_camera_yaw_vel;

        if (analog_camera_pos.pitch > 0x36B0) {
            analog_camera_pos.pitch = 0x36B0;
        }

        if (analog_camera_pos.pitch < -0x16D0) {
            analog_camera_pos.pitch = -0x16D0;
        }

        // recomp_printf("analog cam pitch: %05X\n", analog_camera_pos.pitch);
    }
}

extern s32 sUpdateCameraDirection;
extern s32 sCameraInterfaceFlags;

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

extern CameraSetting sCameraSettings[];
s32 Camera_GetFocalActorPos(Vec3f* dst, Camera* camera);
f32 Camera_GetFocalActorHeight(Camera* camera);
f32 Camera_Vec3fMagnitude(Vec3f* vec);
f32 Camera_GetFloorY(Camera* camera, Vec3f* pos);
f32 Camera_GetFloorYNorm(Camera* camera, Vec3f* floorNorm, Vec3f* chkPos, s32* bgId);
s16 Camera_ScaledStepToFloorS(s16 target, s16 cur, f32 stepScale, s16 minDiff);
s32 func_800CBC84(Camera* camera, Vec3f* from, CameraCollision* to, s32 arg3);
void func_800CBFA4(Camera* camera, Vec3f* arg1, Vec3f* arg2, s32 arg3);
s32 Camera_CalcAtForParallel(Camera* camera, VecGeo* arg1, f32 yOffset, f32 xzOffsetMax, f32* focalActorPosY,
                             s16 flags);
Vec3s* Camera_GetBgCamOrActorCsCamFuncData(Camera* camera, u32 camDataId);
s32 Camera_IsClimbingLedge(Camera* camera);
void Camera_ScaledStepToCeilVec3f(Vec3f* target, Vec3f* cur, f32 xzStepScale, f32 yStepScale, f32 minDiff);
void Camera_SetFocalActorAtOffset(Camera* camera, Vec3f* focalActorPos);
s32 Camera_CalcAtForHorse(Camera* camera, VecGeo* eyeAtDir, f32 yOffset, f32* yPosOffset, s16 calcSlope);
f32 Camera_fabsf(f32 f);
f32 Camera_GetRunSpeedLimit(Camera* camera);

#define CAM_CHANGE_SETTING_0 (1 << 0)
#define CAM_CHANGE_SETTING_1 (1 << 1)
#define CAM_CHANGE_SETTING_2 (1 << 2)
#define CAM_CHANGE_SETTING_3 (1 << 3)

#if 0
static s32 sIsFalse = false;
extern s32 sCameraInitSceneTimer;

extern s16 sCameraNextUID;
extern s32 sCameraHudVisibility;
extern s32 sCameraLetterboxSize;
extern s32 sCameraNegOne1;

#define CAM_DATA_IS_BG (1 << 12) // if not set, then cam data is for actor cutscenes

typedef s32 (*CameraUpdateFunc)(Camera*);

extern CameraUpdateFunc sCameraUpdateHandlers[];

Vec3f Camera_CalcUpVec(s16 pitch, s16 yaw, s16 roll);
f32 Camera_fabsf(f32 f);
s32 Camera_GetBgCamIndex(Camera* camera, s32* bgId, CollisionPoly* poly);
f32 Camera_GetRunSpeedLimit(Camera* camera);
s32 Camera_IsDekuHovering(Camera* camera);
s32 Camera_IsMountedOnHorse(Camera* camera);
s32 Camera_IsUnderwaterAsZora(Camera* camera);
s32 Camera_IsUsingZoraFins(Camera* camera);
void Camera_UpdateInterface(s32 interfaceFlags);
s32 func_800CB7CC(Camera* camera);
s32 func_800CB854(Camera* camera);

RECOMP_PATCH Vec3s Camera_Update(Camera* camera) {
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

#endif

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

// @recomp Patched for analog cam.
RECOMP_PATCH s32 Camera_Normal1(Camera* camera) {
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

    // 76.9 degrees
    if (spB4.pitch > 0x36B0) {
        spB4.pitch = 0x36B0;
    }

    // -76.9 degrees
    if (spB4.pitch < -0x36B0) {
        spB4.pitch = -0x36B0;
    }

    // @recomp Update the analog camera.
    if (recomp_get_analog_cam_enabled()) {
        update_analog_cam(camera);

        if (analog_cam_active) {
            spB4.pitch = analog_camera_pos.pitch;
            // spB4.r = analog_camera_pos.r;
            spB4.yaw = analog_camera_pos.yaw;
        }
    }

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

    // @recomp Don't zoom in on low health when dual analog is used
    if (recomp_get_analog_cam_enabled()) {
        phi_f2 = 1.0f;
    }

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


/**
 * Camera for climbing structures
 */
// @recomp Patched for analog cam.
RECOMP_PATCH s32 Camera_Jump2(Camera* camera) {
    Vec3f* eye = &camera->eye;
    Vec3f* at = &camera->at;
    Vec3f* eyeNext = &camera->eyeNext;
    Vec3f spC8;
    Vec3f spBC;
    VecGeo spB4;
    VecGeo spAC;
    VecGeo spA4;
    VecGeo sp9C;
    s16 temp_t2;
    s16 yawDiff;
    s32 pad;
    f32 sp90;
    f32 sp8C;
    s32 sp88;
    CameraCollision sp60;
    PosRot* focalActorPosRot = &camera->focalActorPosRot;
    Jump2ReadOnlyData* roData = &camera->paramData.jump2.roData;
    Jump2ReadWriteData* rwData = &camera->paramData.jump2.rwData;
    f32 phi_f2;
    f32 yNormal; // used twice
    f32 focalActorHeight = Camera_GetFocalActorHeight(camera);
    f32 temp_f16;

    if (RELOAD_PARAMS(camera)) {
        CameraModeValue* values = sCameraSettings[camera->setting].cameraModes[camera->mode].values;

        yNormal = 0.8f - (-0.2f * (68.0f / focalActorHeight));

        if (camera->unk_0F0.y > 0.0f) {
            phi_f2 = -10.0f;
        } else {
            phi_f2 = 10.0f;
        }

        roData->unk_00 = CAM_RODATA_UNSCALE(phi_f2 + GET_NEXT_RO_DATA(values)) * focalActorHeight * yNormal;
        roData->unk_04 = GET_NEXT_SCALED_RO_DATA(values) * focalActorHeight * yNormal;
        roData->unk_08 = GET_NEXT_SCALED_RO_DATA(values) * focalActorHeight * yNormal;
        roData->unk_0C = GET_NEXT_SCALED_RO_DATA(values);
        roData->unk_10 = GET_NEXT_RO_DATA(values);
        roData->unk_14 = GET_NEXT_SCALED_RO_DATA(values);
        roData->unk_18 = GET_NEXT_RO_DATA(values);
        roData->unk_1C = GET_NEXT_SCALED_RO_DATA(values);
        roData->interfaceFlags = GET_NEXT_RO_DATA(values);
    }

    sp9C = OLib_Vec3fDiffToVecGeo(at, eye);
    spA4 = OLib_Vec3fDiffToVecGeo(at, eyeNext);

    sCameraInterfaceFlags = roData->interfaceFlags;

    if (RELOAD_PARAMS(camera)) {
        spC8 = focalActorPosRot->pos;
        rwData->unk_00 = Camera_GetFloorY(camera, &spC8);
        rwData->unk_04 = spA4.yaw;
        rwData->unk_06 = 0;

        if (rwData->unk_00 == BGCHECK_Y_MIN) {
            rwData->unk_0A = -1;
            rwData->unk_00 = focalActorPosRot->pos.y - 1000.0f;
        } else if ((focalActorPosRot->pos.y - rwData->unk_00) < focalActorHeight) {
            rwData->unk_0A = 1;
        } else {
            rwData->unk_0A = -1;
        }

        yawDiff = BINANG_SUB(BINANG_ROT180(focalActorPosRot->rot.y), spA4.yaw);
        rwData->unk_06 = ((yawDiff / 6) / 4) * 3;

        if (roData->interfaceFlags & JUMP2_FLAG_1) {
            rwData->unk_08 = 10;
        } else {
            rwData->unk_08 = 10000;
        }

        focalActorPosRot->pos.x -= camera->unk_0F0.x;
        focalActorPosRot->pos.y -= camera->unk_0F0.y;
        focalActorPosRot->pos.z -= camera->unk_0F0.z;

        rwData->timer = 6;
        camera->animState++;
        camera->atLerpStepScale = roData->unk_1C;
    }

    sp90 = camera->speedRatio * 0.5f;
    sp8C = camera->speedRatio * 0.2f;

    camera->yawUpdateRateInv = Camera_ScaledStepToCeilF(roData->unk_10, camera->yawUpdateRateInv, sp90, 0.1f);
    camera->yOffsetUpdateRate = Camera_ScaledStepToCeilF(roData->unk_14, camera->yOffsetUpdateRate, sp90, 0.0001f);
    camera->xzOffsetUpdateRate = Camera_ScaledStepToCeilF(0.05f, camera->xzOffsetUpdateRate, sp8C, 0.0001f);
    camera->fovUpdateRate = Camera_ScaledStepToCeilF(0.05f, camera->fovUpdateRate, camera->speedRatio * .05f, 0.0001f);
    camera->rUpdateRateInv = 1800.0f;

    Camera_CalcAtDefault(camera, &spA4, roData->unk_00, 0);
    spB4 = OLib_Vec3fDiffToVecGeo(at, eye);

    //! FAKE: Unused
    yNormal = roData->unk_04;

    phi_f2 = roData->unk_08 + (roData->unk_08 * roData->unk_0C);
    temp_f16 = roData->unk_04 - (roData->unk_04 * roData->unk_0C);

    if (spB4.r > phi_f2) {
        spB4.r = phi_f2;
    } else if (spB4.r < temp_f16) {
        spB4.r = temp_f16;
    }

    yawDiff = BINANG_SUB(BINANG_ROT180(focalActorPosRot->rot.y), spB4.yaw);
    if (rwData->timer != 0) {
        rwData->unk_04 = BINANG_ROT180(focalActorPosRot->rot.y);
        rwData->timer--;
        spB4.yaw = Camera_ScaledStepToCeilS(rwData->unk_04, spA4.yaw, 0.5f, 5);
    } else if (rwData->unk_08 < ABS(yawDiff)) {
        temp_t2 = BINANG_ROT180(focalActorPosRot->rot.y);
        spB4.yaw = Camera_ScaledStepToFloorS((yawDiff < 0) ? (temp_t2 + rwData->unk_08) : (temp_t2 - rwData->unk_08),
                                             spA4.yaw, 0.1f, 1);
    } else {
        spB4.yaw = Camera_ScaledStepToCeilS(spB4.yaw, spA4.yaw, 0.25f, 5);
    }

    spC8.x = focalActorPosRot->pos.x + (Math_SinS(focalActorPosRot->rot.y) * 25.0f);
    spC8.y = focalActorPosRot->pos.y + (focalActorHeight * 2.2f);
    spC8.z = focalActorPosRot->pos.z + (Math_CosS(focalActorPosRot->rot.y) * 25.0f);

    yNormal = Camera_GetFloorYNorm(camera, &spBC, &spC8, &sp88);
    if (camera->focalActor->bgCheckFlags & 0x10) {
        camera->pitchUpdateRateInv = Camera_ScaledStepToCeilF(20.0f, camera->pitchUpdateRateInv, 0.2f, 0.1f);
        camera->rUpdateRateInv = Camera_ScaledStepToCeilF(20.0f, camera->rUpdateRateInv, 0.2f, 0.1f);
        spB4.pitch = Camera_ScaledStepToCeilS(-DEG_TO_BINANG(27.47f), spA4.pitch, 0.2f, 5);
    } else if ((yNormal != BGCHECK_Y_MIN) && (focalActorPosRot->pos.y < yNormal)) {
        camera->pitchUpdateRateInv = Camera_ScaledStepToCeilF(20.0f, camera->pitchUpdateRateInv, 0.2f, 0.1f);
        camera->rUpdateRateInv = Camera_ScaledStepToCeilF(20.0f, camera->rUpdateRateInv, 0.2f, 0.1f);
        if (camera->unk_0F0.y > 1.0f) {
            spB4.pitch = Camera_ScaledStepToCeilS(0x1F4, spA4.pitch, 1.0f / camera->pitchUpdateRateInv, 5);
        }
    } else if ((focalActorPosRot->pos.y - rwData->unk_00) < focalActorHeight) {
        camera->pitchUpdateRateInv = Camera_ScaledStepToCeilF(20.0f, camera->pitchUpdateRateInv, 0.2f, 0.1f);
        camera->rUpdateRateInv = Camera_ScaledStepToCeilF(20.0f, camera->rUpdateRateInv, 0.2f, 0.1f);
        if (camera->unk_0F0.y > 1.0f) {
            spB4.pitch = Camera_ScaledStepToCeilS(0x1F4, spA4.pitch, 1.0f / camera->pitchUpdateRateInv, 5);
        }
    } else {
        camera->pitchUpdateRateInv = 100.0f;
        camera->rUpdateRateInv = 100.0f;
    }

    spB4.pitch = CLAMP_MAX(spB4.pitch, DEG_TO_BINANG(60.43f));
    spB4.pitch = CLAMP_MIN(spB4.pitch, -DEG_TO_BINANG(60.43f));

    // @recomp Update the analog camera.
    if (recomp_get_analog_cam_enabled()) {
        update_analog_cam(camera);

        if (analog_cam_active) {
            spB4.pitch = analog_camera_pos.pitch;
            // spB4.r = analog_camera_pos.r;
            spB4.yaw = analog_camera_pos.yaw;
        }
    }

    *eyeNext = OLib_AddVecGeoToVec3f(at, &spB4);
    sp60.pos = *eyeNext;

    if (func_800CBC84(camera, at, &sp60, 0) != 0) {
        spC8 = sp60.pos;
        spAC.pitch = 0;
        spAC.r = spB4.r;
        spAC.yaw = spB4.yaw;
        sp60.pos = OLib_AddVecGeoToVec3f(at, &spAC);
        if (func_800CBC84(camera, at, &sp60, 0) != 0) {
            *eye = spC8;
        } else {
            spB4.pitch = Camera_ScaledStepToCeilS(0, spB4.pitch, 0.2f, 5);
            *eye = OLib_AddVecGeoToVec3f(at, &spB4);
            func_800CBFA4(camera, at, eye, 0);
        }
    } else {
        *eye = *eyeNext;
    }

    camera->dist = spB4.r;
    camera->fov = Camera_ScaledStepToCeilF(roData->unk_18, camera->fov, camera->fovUpdateRate, 0.1f);
    camera->roll = Camera_ScaledStepToCeilS(0, camera->roll, 0.5f, 5);

    return true;
}

/**
 * Used for targeting
 */
// @recomp Patched for analog cam.
RECOMP_PATCH s32 Camera_Parallel1(Camera* camera) {
    // @recomp
    static bool prev_targeting_held = false;
    bool isChargingDekuFlowerDive = !!(((Player*)camera->focalActor)->stateFlags3 & PLAYER_STATE3_100);

    Vec3f* eye = &camera->eye;
    Vec3f* at = &camera->at;
    Vec3f* eyeNext = &camera->eyeNext;
    Vec3f spB0;
    Vec3f spA4;
    f32 spA0;
    f32 sp9C;
    PosRot* focalActorPosRot = &camera->focalActorPosRot;
    VecGeo sp90;
    VecGeo sp88;
    VecGeo sp80;
    VecGeo sp78;
    BgCamFuncData* bgCamFuncData;
    s16 sp72;
    s16 tangle;
    Parallel1ReadOnlyData* roData = &camera->paramData.para1.roData;
    Parallel1ReadWriteData* rwData = &camera->paramData.para1.rwData;
    s32 parallelFlagCond;
    f32 focalActorHeight = Camera_GetFocalActorHeight(camera);
    s16 new_var2;
    s16 phi_a0;
    s32 phi_a0_2;
    CameraModeValue* values;
    f32 yNormal;

    // @recomp Read timer4 from timer2.
    s16 timer4 = rwData->timer2 >> 5; // @recomp Used to check if z-target can be quit. Mirrors timer2 values during z-target in unmodified function.
    rwData->timer2 &= 0x001F;

    if (!RELOAD_PARAMS(camera)) {
    } else {
        values = sCameraSettings[camera->setting].cameraModes[camera->mode].values;
        roData->unk_00 =
            GET_NEXT_SCALED_RO_DATA(values) * focalActorHeight * (0.8f - ((68.0f / focalActorHeight) * -0.2f));
        roData->unk_04 =
            GET_NEXT_SCALED_RO_DATA(values) * focalActorHeight * (0.8f - ((68.0f / focalActorHeight) * -0.2f));
        roData->unk_08 =
            GET_NEXT_SCALED_RO_DATA(values) * focalActorHeight * (0.8f - ((68.0f / focalActorHeight) * -0.2f));
        roData->unk_20 = CAM_DEG_TO_BINANG(GET_NEXT_RO_DATA(values));
        roData->unk_22 = CAM_DEG_TO_BINANG(GET_NEXT_RO_DATA(values));
        roData->unk_0C = GET_NEXT_RO_DATA(values);
        roData->unk_10 = GET_NEXT_RO_DATA(values);
        roData->unk_14 = GET_NEXT_RO_DATA(values);
        roData->unk_18 = GET_NEXT_SCALED_RO_DATA(values);
        roData->interfaceFlags = GET_NEXT_RO_DATA(values);
        roData->unk_1C = GET_NEXT_SCALED_RO_DATA(values);
        roData->unk_24 = GET_NEXT_RO_DATA(values);
        rwData->unk_00 = roData->unk_04;
    }

    sp80 = OLib_Vec3fDiffToVecGeo(at, eye);
    sp78 = OLib_Vec3fDiffToVecGeo(at, eyeNext);
    Camera_GetFocalActorPos(&spA4, camera);

    switch (camera->animState) {
        case 20:
            if ((roData->interfaceFlags & (PARALLEL1_FLAG_3 | PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1)) == 0) {
                Camera_SetUpdateRatesFastYaw(camera);
            }
            // fallthrough
        case 0:
        case 10:
            if ((roData->interfaceFlags & (PARALLEL1_FLAG_3 | PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1)) ==
                (PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1)) {
                rwData->unk_10 = focalActorPosRot->pos;
            } else {
                camera->xzOffsetUpdateRate = 0.5f;
                camera->yOffsetUpdateRate = 0.5f;
            }

            if ((roData->interfaceFlags & (PARALLEL1_FLAG_3 | PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1)) ==
                PARALLEL1_FLAG_3) {
                rwData->unk_10 = camera->focalActorPosRot.pos;
            }

            rwData->timer1 = 200.0f;

            if ((2.0f * roData->unk_04) < camera->dist) {
                camera->dist = 2.0f * roData->unk_04;
                sp78.r = camera->dist;
                sp80.r = sp78.r;
                *eye = OLib_AddVecGeoToVec3f(at, &sp80);
                *eyeNext = *eye;
            }

            rwData->unk_1C = 0;

            if (roData->interfaceFlags & PARALLEL1_FLAG_2) {
                rwData->timer2 = 20;
            } else {
                rwData->timer2 = 6;
                // @recomp Initiate timer4 for z-target.
                timer4 = 6;
            }

            if ((camera->focalActor == &GET_PLAYER(camera->play)->actor) && (camera->mode == CAM_MODE_CHARGE)) {
                rwData->timer2 = 30;
                if (((Player*)camera->focalActor)->transformation == PLAYER_FORM_DEKU) {
                    roData->unk_24 = -1;
                }
            }

            if ((roData->interfaceFlags & (PARALLEL1_FLAG_3 | PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1)) ==
                (PARALLEL1_FLAG_3 | PARALLEL1_FLAG_1)) {
                rwData->timer2 = 1;
                yNormal = 0.8f - ((68.0f / focalActorHeight) * -0.2f);

                bgCamFuncData = (BgCamFuncData*)Camera_GetBgCamOrActorCsCamFuncData(camera, camera->bgCamIndex);

                rwData->unk_20 = bgCamFuncData->rot.x;
                rwData->unk_1E = bgCamFuncData->rot.y;
                rwData->unk_08 = (bgCamFuncData->fov == -1)   ? roData->unk_14
                                 : (bgCamFuncData->fov > 360) ? CAM_RODATA_UNSCALE(bgCamFuncData->fov)
                                                              : bgCamFuncData->fov;
                rwData->unk_00 = (bgCamFuncData->unk_0E == -1)
                                     ? roData->unk_04
                                     : CAM_RODATA_UNSCALE(bgCamFuncData->unk_0E) * focalActorHeight * yNormal;
            //! FAKE
            dummy:;
            } else {
                rwData->unk_08 = roData->unk_14;
                rwData->unk_00 = roData->unk_04;
            }

            rwData->timer3 = roData->unk_24;
            rwData->unk_04 = focalActorPosRot->pos.y - camera->unk_0F0.y;
            rwData->unk_26 = 1;
            camera->animState = 1;
            sCameraInterfaceFlags = roData->interfaceFlags;

            // @recomp Reset prev_targeting_held after transition.
            prev_targeting_held = false;

            break;
    }

    // @recomp Change behavior for z-target only.
    if (camera_fixes) {
        if ((roData->interfaceFlags & (PARALLEL1_FLAG_3 | PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1)) == PARALLEL1_FLAG_1) {
            Player* player = GET_PLAYER(camera->play);
            bool targeting_held = func_80123434(player) || player->lockOnActor != NULL;

            // @recomp Fix camera rotating with player if z-target gets released too fast after transition.
            if ((targeting_held) && (!prev_targeting_held)) {
                // @recomp Reset timer2 to avoid immediate rotation, if player presses, releases and presses z-target in a very short time-window.
                rwData->timer2 = 6;
                rwData->unk_1E = BINANG_ROT180(camera->focalActorPosRot.rot.y) + roData->unk_22;
            }

            // @recomp Maintain vanilla behavior for quitting z-target.
            if ((timer4 == 0) && (!targeting_held)) {
                rwData->timer2 = 0;
            }

            // @recomp Decrease timer4 only in cases where timer2 would be decreased.
            if ((timer4 > 0)
            && (rwData->timer3 <= 0)) {
                timer4--;
            }

            prev_targeting_held = targeting_held;
        }
    }

    if (rwData->timer2 != 0) {
        switch (roData->interfaceFlags & (PARALLEL1_FLAG_3 | PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1)) {
            case PARALLEL1_FLAG_1:
                if (camera_fixes) {
                    // @recomp Fix camera rotating with player if z-target gets released too fast after transition.
                    if (isChargingDekuFlowerDive) {
                        // @recomp Fix camera wiggle during dive into deku flower.
                        rwData->unk_1E = BINANG_ROT180(camera->focalActorPosRot.rot.y) + roData->unk_22;
                    }
                    rwData->unk_20 = roData->unk_20;
                    break;
                }
                // @recomp fallthrough

            case (PARALLEL1_FLAG_3 | PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1):
                rwData->unk_1E = BINANG_ROT180(camera->focalActorPosRot.rot.y) + roData->unk_22;
                rwData->unk_20 = roData->unk_20;
                break;

            case PARALLEL1_FLAG_2:
                rwData->unk_1E = roData->unk_22;
                rwData->unk_20 = roData->unk_20;
                break;

            case (PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1):
                if (rwData->timer3 == 1) {
                    sp88 = OLib_Vec3fDiffToVecGeo(&rwData->unk_10, &spA4);
                    rwData->unk_1E = ((ABS(BINANG_SUB(sp88.yaw, sp80.yaw)) < 0x3A98) || Camera_IsClimbingLedge(camera))
                                         ? sp80.yaw
                                         : sp80.yaw + (s16)((BINANG_SUB(sp88.yaw, sp80.yaw) >> 2) * 3);
                }
                rwData->unk_20 = roData->unk_20;
                break;

            case PARALLEL1_FLAG_3:
                rwData->unk_1E = sp80.yaw;
                rwData->unk_20 = roData->unk_20;
                break;

            case (PARALLEL1_FLAG_3 | PARALLEL1_FLAG_1):
                break;

            default:
                rwData->unk_1E = sp78.yaw + roData->unk_22;
                rwData->unk_20 = roData->unk_20;
                break;
        }
    } else if (roData->interfaceFlags & PARALLEL1_FLAG_5) {
        rwData->unk_1E = BINANG_ROT180(camera->focalActorPosRot.rot.y) + roData->unk_22;
    }

    if (camera->animState == 21) {
        camera->animState = 1;
    } else if (camera->animState == 11) {
        camera->animState = 1;
    }

    spA0 = camera->speedRatio * 0.5f;
    sp9C = camera->speedRatio * 0.2f;

    if (((roData->interfaceFlags & (PARALLEL1_FLAG_3 | PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1)) ==
         (PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1)) ||
        ((roData->interfaceFlags & (PARALLEL1_FLAG_3 | PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1)) == PARALLEL1_FLAG_3) ||
        (roData->interfaceFlags & PARALLEL1_FLAG_5)) {
        camera->rUpdateRateInv = Camera_ScaledStepToCeilF(20.0f, camera->rUpdateRateInv, 0.5f, 0.1f);
        camera->yawUpdateRateInv = Camera_ScaledStepToCeilF(roData->unk_0C, camera->yawUpdateRateInv, 0.5f, 0.1f);
        camera->pitchUpdateRateInv = Camera_ScaledStepToCeilF(20.0f, camera->pitchUpdateRateInv, 0.5f, 0.1f);
    } else {
        camera->rUpdateRateInv = Camera_ScaledStepToCeilF(20.0f, camera->rUpdateRateInv, spA0, 0.1f);
        camera->yawUpdateRateInv = Camera_ScaledStepToCeilF(roData->unk_0C, camera->yawUpdateRateInv, spA0, 0.1f);
        camera->pitchUpdateRateInv = Camera_ScaledStepToCeilF(2.0f, camera->pitchUpdateRateInv, sp9C, 0.1f);
    }

    if ((roData->interfaceFlags & (PARALLEL1_FLAG_3 | PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1)) ==
        (PARALLEL1_FLAG_3 | PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1)) {
        camera->yOffsetUpdateRate = Camera_ScaledStepToCeilF(0.1f, camera->yOffsetUpdateRate, spA0, 0.0001f);
        camera->xzOffsetUpdateRate = Camera_ScaledStepToCeilF(0.1f, camera->xzOffsetUpdateRate, sp9C, 0.0001f);
    } else if (roData->interfaceFlags & PARALLEL1_FLAG_7) {
        camera->yOffsetUpdateRate = Camera_ScaledStepToCeilF(0.5f, camera->yOffsetUpdateRate, spA0, 0.0001f);
        camera->xzOffsetUpdateRate = Camera_ScaledStepToCeilF(0.5f, camera->xzOffsetUpdateRate, sp9C, 0.0001f);
    } else {
        camera->yOffsetUpdateRate = Camera_ScaledStepToCeilF(0.05f, camera->yOffsetUpdateRate, spA0, 0.0001f);
        camera->xzOffsetUpdateRate = Camera_ScaledStepToCeilF(0.05f, camera->xzOffsetUpdateRate, sp9C, 0.0001f);
    }

    // TODO: Extra trailing 0 in 0.050f needed?
    camera->fovUpdateRate =
        Camera_ScaledStepToCeilF(0.050f, camera->fovUpdateRate, camera->speedRatio * 0.05f, 0.0001f);

    if (roData->interfaceFlags & PARALLEL1_FLAG_0) {
        tangle = Camera_GetPitchAdjFromFloorHeightDiffs(camera, BINANG_ROT180(sp80.yaw), rwData->unk_26 = 1);
        spA0 = ((1.0f / roData->unk_10));
        spA0 *= 0.6f;
        sp9C = ((1.0f / roData->unk_10) * 0.4f) * (1.0f - camera->speedRatio);
        rwData->unk_1C = Camera_ScaledStepToCeilS(tangle, rwData->unk_1C, spA0 + sp9C, 5);
    } else {
        rwData->unk_1C = 0;
    }

    if (func_800CB950(camera) || (((Player*)camera->focalActor)->stateFlags1 & PLAYER_STATE1_1000) ||
        (((Player*)camera->focalActor)->stateFlags3 & PLAYER_STATE3_100)) {
        rwData->unk_04 = camera->focalActorPosRot.pos.y;
        sp72 = false;
    } else {
        sp72 = true;
    }

    if ((((Player*)camera->focalActor)->stateFlags1 & PLAYER_STATE1_4000) ||
        (((Player*)camera->focalActor)->stateFlags1 & PLAYER_STATE1_4) ||
        ((roData->interfaceFlags & (PARALLEL1_FLAG_3 | PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1)) ==
         (PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1))) {
        spB0 = spA4;
        spB0.y += ((focalActorHeight * 0.6f) + roData->unk_00);
        Camera_ScaledStepToCeilVec3f(&spB0, at, camera->xzOffsetUpdateRate, camera->yOffsetUpdateRate, 0.0001f);
        Camera_SetFocalActorAtOffset(camera, &focalActorPosRot->pos);
    } else if ((roData->interfaceFlags & (PARALLEL1_FLAG_3 | PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1)) ==
               (PARALLEL1_FLAG_3 | PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1)) {
        spB0 = focalActorPosRot->pos;
        spB0.y += focalActorHeight + roData->unk_00;
        Camera_ScaledStepToCeilVec3f(&spB0, at, camera->xzOffsetUpdateRate, camera->yOffsetUpdateRate, 0.0001f);
        Camera_SetFocalActorAtOffset(camera, &focalActorPosRot->pos);
    } else if (rwData->timer2 != 0) {
        Camera_CalcAtDefault(camera, &sp78, roData->unk_00, 0);
        rwData->timer1 = 200.0f;
    } else if (!(roData->interfaceFlags & PARALLEL1_FLAG_7) && !sp72) {
        Camera_CalcAtForParallel(camera, &sp78, roData->unk_00, roData->unk_08, &rwData->unk_04,
                                 roData->interfaceFlags & (PARALLEL1_FLAG_6 | PARALLEL1_FLAG_0));
        rwData->timer1 = 200.0f;
    } else {
        Camera_CalcAtForScreen(camera, &sp78, roData->unk_00, &rwData->unk_04, rwData->timer1);
        if (rwData->timer1 > 10.0f) {
            rwData->timer1--;
        }
    }

    camera->dist = Camera_ScaledStepToCeilF(rwData->unk_00, camera->dist, 1.0f / camera->rUpdateRateInv, 0.1f);

    if (rwData->timer2 != 0) {
        if (rwData->timer3 <= 0) {
            if (rwData->timer3 == 0) {
                Camera_SetStateFlag(camera, CAM_STATE_DISABLE_MODE_CHANGE);
            }

            tangle = ((rwData->timer2 + 1) * rwData->timer2) >> 1;
            sp90.yaw = sp80.yaw + ((BINANG_SUB(rwData->unk_1E, sp80.yaw) / tangle) * rwData->timer2);
            phi_a0 = ((roData->interfaceFlags & PARALLEL1_FLAG_0) ? BINANG_SUB(rwData->unk_20, rwData->unk_1C)
                                                                  : rwData->unk_20);
            new_var2 = BINANG_SUB(phi_a0, sp80.pitch);
            sp90.pitch = sp80.pitch + ((new_var2 / tangle) * rwData->timer2);
            sp90.r = camera->dist;
            rwData->timer2--;
        } else {
            sp90 = sp80;
            sp90.r = camera->dist;
        }
    } else {
        sp90 = OLib_Vec3fDiffToVecGeo(at, eyeNext);
        sp90.r = camera->dist;

        if (roData->interfaceFlags & PARALLEL1_FLAG_1) {
            sp90.yaw = Camera_ScaledStepToCeilS(rwData->unk_1E, sp78.yaw, 1.0f / camera->yawUpdateRateInv, 0xC8);
        }

        parallelFlagCond = (roData->interfaceFlags & (PARALLEL1_FLAG_3 | PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1));

        if (roData->interfaceFlags & PARALLEL1_FLAG_0) {
            phi_a0 = (rwData->unk_20 - rwData->unk_1C);
        } else {
            phi_a0 = rwData->unk_20;
        }

        if (parallelFlagCond == (PARALLEL1_FLAG_3 | PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1)) {
            spA0 = CLAMP_MAX(camera->speedRatio, 1.0f);
            phi_a0 = (sp90.pitch * spA0) + (phi_a0 * (1.0f - spA0));
            sp90.pitch = Camera_ScaledStepToCeilS(phi_a0, sp78.pitch, 1.0f / camera->pitchUpdateRateInv, 5);
        } else if (parallelFlagCond != PARALLEL1_FLAG_3) {
            sp90.pitch = Camera_ScaledStepToCeilS(phi_a0, sp78.pitch, 1.0f / camera->pitchUpdateRateInv, 5);
        }

        if (sp90.pitch > DEG_TO_BINANG(79.655f)) {
            sp90.pitch = DEG_TO_BINANG(79.655f);
        }

        if (sp90.pitch < -DEG_TO_BINANG(29.995f)) {
            sp90.pitch = -DEG_TO_BINANG(29.995f);
        }
    }

    if (rwData->timer3 > 0) {
        rwData->timer3--;
    }

    // @recomp Update the analog camera.
    if (recomp_get_analog_cam_enabled()) {
        update_analog_cam(camera);

        if (analog_cam_active) {
            sp90.pitch = analog_camera_pos.pitch;
            // sp90.r = analog_camera_pos.r;
            sp90.yaw = analog_camera_pos.yaw;
        }
    }

    *eyeNext = OLib_AddVecGeoToVec3f(at, &sp90);

    if (camera->status == CAM_STATUS_ACTIVE) {
        if ((camera->play->envCtx.skyboxDisabled == 0) || (roData->interfaceFlags & PARALLEL1_FLAG_4)) {
            spB0 = *at;
            if ((((Player*)camera->focalActor)->stateFlags1 & PLAYER_STATE1_4000) ||
                (((Player*)camera->focalActor)->stateFlags1 & PLAYER_STATE1_4) ||
                ((roData->interfaceFlags & (PARALLEL1_FLAG_3 | PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1)) ==
                 (PARALLEL1_FLAG_2 | PARALLEL1_FLAG_1))) {
                spB0.y += focalActorHeight;
            }
            *eye = *eyeNext;
            func_800CBFA4(camera, &spB0, eye, 0);
        } else {
            *eye = *eyeNext;
            func_800CBFA4(camera, at, eye, 3);
        }

        if (camera_fixes) {
            // @recomp Fix camera not updating input dir for the first few frames after transition.
            if ((isChargingDekuFlowerDive) && (rwData->timer2 != 0)) {
                // @recomp Fix camera wiggle during dive into deku flower.
                sUpdateCameraDirection = true;
            } else {
                sUpdateCameraDirection = false;
            }
        } else {
            if (rwData->timer2 != 0) {
                sUpdateCameraDirection = true;
            } else {
                sUpdateCameraDirection = false;
            }
        }
    }

    camera->fov = Camera_ScaledStepToCeilF(rwData->unk_08, camera->fov, camera->fovUpdateRate, 0.1f);
    camera->roll = Camera_ScaledStepToCeilS(0, camera->roll, 0.5f, 5);
    camera->atLerpStepScale = Camera_ClampLerpScale(camera, sp72 ? roData->unk_1C : roData->unk_18);
    rwData->unk_26 &= ~1;

    // @recomp Save timer4 in unused bits of timer2.
    rwData->timer2 |= timer4 << 5;

    return 1;
}

#define NORMAL3_RW_FLAG (1 << 0)

/**
 * Riding Epona and Zora
 */
// @recomp Patched for analog cam.
RECOMP_PATCH s32 Camera_Normal3(Camera* camera) {
    Normal3ReadOnlyData* roData = &camera->paramData.norm3.roData;
    Normal3ReadWriteData* rwData = &camera->paramData.norm3.rwData;
    f32 sp8C;
    f32 sp90;
    f32 temp_f2; // multi-use temp
    f32 sp88;
    VecGeo sp80;
    VecGeo sp78;
    VecGeo sp70;
    VecGeo sp68;
    f32 phi_f2;
    s16 sp62;
    s16 phi_v1_2;
    Player* player = (Player*)camera->focalActor;
    Vec3f* eye = &camera->eye;
    Vec3f* at = &camera->at;
    Vec3f* eyeNext = &camera->eyeNext;
    PosRot* focalActorPosRot = &camera->focalActorPosRot;

    temp_f2 = Camera_GetFocalActorHeight(camera);

    if ((camera->setting == CAM_SET_HORSE) && (player->rideActor == NULL)) {
        Camera_ChangeSettingFlags(camera, camera->prevSetting, CAM_CHANGE_SETTING_1);
        return 1;
    }

    if (RELOAD_PARAMS(camera)) {
        CameraModeValue* values = sCameraSettings[camera->setting].cameraModes[camera->mode].values;

        temp_f2 = CAM_RODATA_UNSCALE(temp_f2);

        roData->yOffset = GET_NEXT_RO_DATA(values) * temp_f2;
        roData->distMin = GET_NEXT_RO_DATA(values) * temp_f2;
        roData->distMax = GET_NEXT_RO_DATA(values) * temp_f2;
        roData->pitchTarget = CAM_DEG_TO_BINANG(GET_NEXT_RO_DATA(values));
        roData->yawUpdateRateInv = GET_NEXT_RO_DATA(values);
        roData->pitchUpdateRateInv = GET_NEXT_RO_DATA(values);
        roData->fovTarget = GET_NEXT_RO_DATA(values);
        roData->maxAtLERPScale = GET_NEXT_SCALED_RO_DATA(values);
        roData->interfaceFlags = GET_NEXT_RO_DATA(values);
    }

    sp70 = OLib_Vec3fDiffToVecGeo(at, eye);
    sp68 = OLib_Vec3fDiffToVecGeo(at, eyeNext);
    sUpdateCameraDirection = true;
    sCameraInterfaceFlags = roData->interfaceFlags;

    //! FAKE: fake temp
    phi_v1_2 = camera->animState;
    if (!(((phi_v1_2 == 0) || (phi_v1_2 == 10)) || (phi_v1_2 == 20))) {
    } else {
        rwData->isZero = 0;
        rwData->curPitch = 0;
        rwData->yPosOffset = camera->focalActorFloorHeight;

        D_801EDC30[camera->camId].yaw = D_801EDC30[camera->camId].pitch = D_801EDC30[camera->camId].unk_64 = 0;
        D_801EDC30[camera->camId].swingUpdateRate = roData->yawUpdateRateInv;
        rwData->yawUpdateRate = BINANG_SUB(BINANG_ROT180(camera->focalActorPosRot.rot.y), sp70.yaw) * (1.0f / 6.0f);
        rwData->distTimer = 0;
        rwData->is1200 = 1200;

        if (roData->interfaceFlags & NORMAL3_FLAG_1) {
            rwData->yawTimer = 6;
            Camera_SetStateFlag(camera, CAM_STATE_DISABLE_MODE_CHANGE);
        } else {
            rwData->yawTimer = 0;
        }

        camera->animState = 1;
        D_801EDC30[camera->camId].timer = 0;
        rwData->flag = NORMAL3_RW_FLAG;
    }

    if (rwData->distTimer != 0) {
        rwData->distTimer--;
    }

    sp90 = ((camera->speedRatio * 3.0f) + 1.0f) * 0.25f * 0.5f;
    sp8C = temp_f2 = camera->speedRatio * 0.2f;

    if (D_801EDC30[camera->camId].timer != 0) {
        camera->yawUpdateRateInv = Camera_ScaledStepToCeilF(
            (D_801EDC30[camera->camId].timer * 2) + roData->yawUpdateRateInv, camera->yawUpdateRateInv, sp90, 0.1f);
        camera->pitchUpdateRateInv = Camera_ScaledStepToCeilF((D_801EDC30[camera->camId].timer * 2) + 16.0f,
                                                              camera->pitchUpdateRateInv, sp8C, 0.1f);
        D_801EDC30[camera->camId].timer--;
    } else {
        camera->yawUpdateRateInv =
            Camera_ScaledStepToCeilF(roData->yawUpdateRateInv, camera->yawUpdateRateInv, sp90, 0.1f);
        camera->pitchUpdateRateInv = Camera_ScaledStepToCeilF(16.0f, camera->pitchUpdateRateInv, sp8C, 0.1f);
    }

    camera->yOffsetUpdateRate = Camera_ScaledStepToCeilF(0.05f, camera->yOffsetUpdateRate, sp90, 0.001f);
    camera->xzOffsetUpdateRate = Camera_ScaledStepToCeilF(0.05f, camera->xzOffsetUpdateRate, sp8C, 0.0001f);
    camera->fovUpdateRate = Camera_ScaledStepToCeilF(0.05f, camera->fovUpdateRate, sp8C, 0.0001f);

    phi_v1_2 = Camera_GetPitchAdjFromFloorHeightDiffs(camera, BINANG_ROT180(sp70.yaw), rwData->flag & NORMAL3_RW_FLAG);
    temp_f2 = ((1.0f / roData->pitchUpdateRateInv) * 0.5f) * (1.0f - camera->speedRatio);
    rwData->curPitch =
        Camera_ScaledStepToCeilS(phi_v1_2, rwData->curPitch, ((1.0f / roData->pitchUpdateRateInv) * 0.5f) + temp_f2, 5);

    if ((roData->interfaceFlags & NORMAL3_FLAG_6) || (player->rideActor == NULL)) {
        Camera_CalcAtDefault(camera, &sp68, roData->yOffset, 1);
    } else {
        Camera_CalcAtForHorse(camera, &sp68, roData->yOffset, &rwData->yPosOffset, 1);
    }

    sp88 = (roData->distMax + roData->distMin) * 0.5f;
    sp80 = OLib_Vec3fDiffToVecGeo(at, eyeNext);
    temp_f2 = Camera_ClampDist1(camera, sp80.r, roData->distMin, roData->distMax, rwData->distTimer);

    phi_f2 = sp88 - temp_f2;
    phi_f2 *= 0.002f;
    camera->dist = sp80.r = temp_f2 + phi_f2;

    if (roData->interfaceFlags & NORMAL3_FLAG_7) {
        sp80.pitch = Camera_ScaledStepToCeilS(camera->focalActor->focus.rot.x - rwData->curPitch, sp68.pitch, 0.25f, 5);
    } else {
        sp62 = roData->pitchTarget - rwData->curPitch;
        sp80.pitch = Camera_ScaledStepToCeilS(sp62, sp68.pitch, 1.0f / camera->pitchUpdateRateInv, 5);
    }

    sp80.pitch = CLAMP_MAX(sp80.pitch, DEG_TO_BINANG(79.655f));

    if (sp80.pitch < -DEG_TO_BINANG(29.995f)) {
        sp80.pitch = -DEG_TO_BINANG(29.995f);
    }

    if (roData->interfaceFlags & NORMAL3_FLAG_7) {
        sp62 = BINANG_SUB(camera->focalActor->focus.rot.y, BINANG_ROT180(sp68.yaw));
        temp_f2 = 1.0f;
    } else {
        sp62 = BINANG_SUB(focalActorPosRot->rot.y, BINANG_ROT180(sp68.yaw));
        sp78 = OLib_Vec3fToVecGeo(&camera->unk_0F0);
        phi_v1_2 = focalActorPosRot->rot.y - sp78.yaw;
        if (phi_v1_2 < 0) {
            phi_v1_2 *= -1;
        }

        if (phi_v1_2 < 0x555A) {
            temp_f2 = 1.0f;
        } else {
            temp_f2 = ((f32)0x8000 - phi_v1_2) / (f32)0x2AA6;
        }
    }

    sp90 = (sp62 * ((SQ(camera->speedRatio) * 0.8f) + 0.2f) * temp_f2) / camera->yawUpdateRateInv;
    if ((Camera_fabsf(sp90) > 150.0f) && (camera->speedRatio > 0.05f)) {
        sp80.yaw = sp68.yaw + sp90;
    }

    if (rwData->yawTimer > 0) {
        sp80.yaw += rwData->yawUpdateRate;
        rwData->yawTimer--;
        if (rwData->yawTimer == 0) {
            Camera_UnsetStateFlag(camera, CAM_STATE_DISABLE_MODE_CHANGE);
        }
    }

    // @recomp Update the analog camera.
    if (recomp_get_analog_cam_enabled()) {
        update_analog_cam(camera);

        if (analog_cam_active) {
            sp80.pitch = analog_camera_pos.pitch;
            // sp80.r = analog_camera_pos.r;
            sp80.yaw = analog_camera_pos.yaw;
        }
    }

    *eyeNext = OLib_AddVecGeoToVec3f(at, &sp80);

    if (camera->status == CAM_STATUS_ACTIVE) {
        *eye = *eyeNext;
        func_800CBFA4(camera, at, eye, 0);
    } else {
        *eye = *eyeNext;
    }

    camera->fov = Camera_ScaledStepToCeilF(roData->fovTarget, camera->fov, camera->fovUpdateRate, 0.1f);

    if (roData->interfaceFlags & NORMAL3_FLAG_5) {
        camera->roll = Camera_ScaledStepToCeilS(0, camera->roll, 0.05f, 5);
    } else {
        camera->roll = Camera_ScaledStepToCeilS(0, camera->roll, 0.1f, 5);
    }

    camera->atLerpStepScale = Camera_ClampLerpScale(camera, roData->maxAtLERPScale);
    rwData->flag &= ~NORMAL3_RW_FLAG;

    return 1;
}

/**
 * Used for water-based camera settings
 * e.g. Gyorg, Pinnacle Rock, whirlpool, water
 */
// @recomp Patched for analog cam.
RECOMP_PATCH s32 Camera_Jump3(Camera* camera) {
    Vec3f* sp48 = &camera->eye;
    Vec3f* sp44 = &camera->at;
    Vec3f* sp40 = &camera->eyeNext;
    f32 spD0;
    f32 spCC;
    PosRot* focalActorPosRot = &camera->focalActorPosRot;
    f32 phi_f0;
    f32 spC0;
    Vec3f spB4;
    VecGeo spAC;
    CameraModeValue* values;
    f32 phi_f14;
    VecGeo sp9C;
    VecGeo sp94;
    f32 phi_f2_2;
    f32 temp_f0;
    f32 temp1;
    f32 pad;
    Jump3ReadOnlyData* roData = &camera->paramData.jump3.roData;
    Jump3ReadWriteData* rwData = &camera->paramData.jump3.rwData;
    f32 focalActorHeight;
    PosRot focalActorFocus;
    f32 sp60;
    f32 sp5C;
    s32 sp58;

    focalActorHeight = Camera_GetFocalActorHeight(camera);
    focalActorFocus = Actor_GetFocus(camera->focalActor);
    sp60 = camera->waterYPos - sp48->y;

    sp58 = false;

    if (RELOAD_PARAMS(camera)) {
        rwData->unk_0A = camera->mode;
        rwData->timer2 = 0;
    }

    if (camera->mode == CAM_MODE_NORMAL) {
        if ((camera->focalActor->bgCheckFlags & 0x10) || (rwData->timer2 != 0)) {
            if (rwData->unk_0A != 0xF) {
                rwData->unk_0A = 0xF;
                sp58 = true;
                rwData->timer2 = 10;
            }
        } else if (sp60 < 50.0f) {
            if (rwData->unk_0A != 0) {
                rwData->unk_0A = 0;
                sp58 = true;
            }
        } else if (Camera_fabsf(camera->focalActorPosRot.pos.y - camera->focalActorFloorHeight) < 11.0f) {
            if (rwData->unk_0A != 5) {
                rwData->unk_0A = 5;
                sp58 = true;
            }
        } else if ((sp60 > 250.0f) && (rwData->unk_0A != 0x1A)) {
            rwData->unk_0A = 0x1A;
            sp58 = true;
        }
    }

    if (rwData->timer2 != 0) {
        rwData->timer2--;
    }

    sp9C = OLib_Vec3fDiffToVecGeo(sp44, sp48);
    sp94 = OLib_Vec3fDiffToVecGeo(sp44, sp40);

    if (!RELOAD_PARAMS(camera) && !sp58) {
    } else {
        values = sCameraSettings[camera->setting].cameraModes[rwData->unk_0A].values;

        sp5C = 0.8f - (-0.2f * (68.0f / focalActorHeight));
        spD0 = focalActorHeight * 0.01f * sp5C;

        roData->unk_00 = GET_NEXT_RO_DATA(values) * spD0;
        roData->unk_04 = GET_NEXT_RO_DATA(values) * spD0;
        roData->unk_08 = GET_NEXT_RO_DATA(values) * spD0;
        roData->unk_20 = CAM_DEG_TO_BINANG(GET_NEXT_RO_DATA(values));
        roData->unk_0C = GET_NEXT_RO_DATA(values);
        roData->unk_10 = GET_NEXT_RO_DATA(values);
        roData->unk_14 = GET_NEXT_SCALED_RO_DATA(values);
        roData->unk_18 = GET_NEXT_RO_DATA(values);
        roData->unk_1C = GET_NEXT_SCALED_RO_DATA(values);
        roData->interfaceFlags = GET_NEXT_RO_DATA(values);
    }

    sCameraInterfaceFlags = roData->interfaceFlags;

    switch (camera->animState) {
        case 0:
            rwData->unk_10 = 0x1000;
            // fallthrough
        case 10:
        case 20:
            rwData->unk_00 = camera->focalActorFloorHeight;
            D_801EDC30[camera->camId].yaw = D_801EDC30[camera->camId].pitch = D_801EDC30[camera->camId].unk_64 = 0;
            rwData->timer1 = 10;
            D_801EDC30[camera->camId].swingUpdateRate = roData->unk_0C;
            camera->animState++;
            D_801EDC30[camera->camId].timer = 0;
            break;

        default:
            if (rwData->timer1 != 0) {
                rwData->timer1--;
            }
            break;
    }

    spC0 = focalActorFocus.pos.y - focalActorPosRot->pos.y;
    spB4 = *sp48;

    spD0 = camera->speedRatio * 0.5f;
    spCC = camera->speedRatio * 0.2f;

    temp_f0 = (D_801EDC30[camera->camId].unk_64 == 1) ? 0.5f : spD0;

    if (D_801EDC30[camera->camId].timer != 0) {
        camera->yawUpdateRateInv =
            Camera_ScaledStepToCeilF((D_801EDC30[camera->camId].swingUpdateRate + D_801EDC30[camera->camId].timer * 2),
                                     camera->yawUpdateRateInv, spD0, 0.1f);
        camera->pitchUpdateRateInv = Camera_ScaledStepToCeilF((40.0f + D_801EDC30[camera->camId].timer * 2),
                                                              camera->pitchUpdateRateInv, spCC, 0.1f);
        D_801EDC30[camera->camId].timer--;
    } else {
        camera->yawUpdateRateInv = Camera_ScaledStepToCeilF(D_801EDC30[camera->camId].swingUpdateRate,
                                                            camera->yawUpdateRateInv, temp_f0, 0.1f);
        camera->pitchUpdateRateInv = Camera_ScaledStepToCeilF(40.0f, camera->pitchUpdateRateInv, spCC, 0.1f);
    }

    if (roData->interfaceFlags & JUMP3_FLAG_7) {
        camera->yOffsetUpdateRate = Camera_ScaledStepToCeilF(0.01f, camera->yOffsetUpdateRate, spD0, 0.0001f);
        sp5C = sqrtf((camera->unk_0F0.x * camera->unk_0F0.x) + (camera->unk_0F0.z * camera->unk_0F0.z)) /
               Camera_GetRunSpeedLimit(camera);
        camera->speedRatio = OLib_ClampMaxDist(sp5C / Camera_GetRunSpeedLimit(camera), 1.8f);
        spCC = camera->speedRatio * 0.2f;
    } else {
        camera->yOffsetUpdateRate = Camera_ScaledStepToCeilF(0.05f, camera->yOffsetUpdateRate, spD0, 0.0001f);
    }

    camera->xzOffsetUpdateRate = Camera_ScaledStepToCeilF(0.05f, camera->xzOffsetUpdateRate, spCC, 0.0001f);
    camera->fovUpdateRate =
        Camera_ScaledStepToCeilF(0.050f, camera->fovUpdateRate, camera->speedRatio * 0.05f, 0.0001f);

    if (sp60 < 50.0f) {
        sp5C = camera->waterYPos - spC0;

        Camera_CalcAtForScreen(camera, &sp94, roData->unk_00, &sp5C,
                               ((sp60 < 0.0f) ? 1.0f : 1.0f - (sp60 / 50.0f)) * 50.0f);
    } else {
        Camera_CalcAtDefault(camera, &sp94, roData->unk_00, roData->interfaceFlags);
    }

    spAC = OLib_Vec3fDiffToVecGeo(sp44, sp40);
    spAC.r = Camera_ClampDist1(camera, spAC.r, roData->unk_04, roData->unk_08, rwData->timer1);
    camera->dist = spAC.r;

    if (!(Camera_fabsf(focalActorPosRot->pos.y - camera->focalActorFloorHeight) < 10.0f) &&
        !(Camera_fabsf(focalActorFocus.pos.y - camera->waterYPos) < 50.f)) {
        camera->pitchUpdateRateInv = 100.0f;
    }

    if (roData->interfaceFlags & JUMP3_FLAG_5) {
        spD0 = CLAMP_MAX(camera->speedRatio * 1.3f, 0.6f);

        //! FAKE: spCC =
        spAC.pitch = Camera_ScaledStepToCeilS(
            (spAC.pitch * spD0) + (roData->unk_20 * (1.0f - spD0)), sp94.pitch,
            1.0f / (spCC = ((camera->pitchUpdateRateInv + 1.0f) - (camera->pitchUpdateRateInv * spD0))), 5);
    } else if (D_801EDC30[camera->camId].unk_64 == 1) {
        spAC.yaw =
            Camera_ScaledStepToCeilS(D_801EDC30[camera->camId].yaw, sp94.yaw, 1.0f / camera->yawUpdateRateInv, 5);

        // Bug? Should be pitchUpdateRateInv
        spAC.pitch =
            Camera_ScaledStepToCeilS(D_801EDC30[camera->camId].pitch, sp94.pitch, 1.0f / camera->yawUpdateRateInv, 5);
    } else if (roData->interfaceFlags & (JUMP3_FLAG_7 | JUMP3_FLAG_3)) {
        spAC.yaw = Camera_CalcDefaultYaw(camera, sp94.yaw, focalActorPosRot->rot.y, roData->unk_14, 0.0f);

        spD0 = CLAMP_MAX(camera->speedRatio * 1.3f, 1.0f);

        //! FAKE: spCC =
        spAC.pitch = Camera_ScaledStepToCeilS(
            (spAC.pitch * spD0) + (roData->unk_20 * (1.0f - spD0)), sp94.pitch,
            1.0f / (spCC = (camera->pitchUpdateRateInv + 1.0f) - (camera->pitchUpdateRateInv * spD0)), 5);
    } else {
        spAC.yaw = Camera_CalcDefaultYaw(camera, sp94.yaw, focalActorPosRot->rot.y, roData->unk_14, 0.0f);
        spAC.pitch = Camera_CalcDefaultPitch(camera, sp94.pitch, roData->unk_20, 0);
    }

    if (spAC.pitch > DEG_TO_BINANG(79.655f)) {
        spAC.pitch = DEG_TO_BINANG(79.655f);
    }

    if (spAC.pitch < -DEG_TO_BINANG(29.995f)) {
        spAC.pitch = -DEG_TO_BINANG(29.995f);
    }

    // @recomp Update the analog camera.
    if (recomp_get_analog_cam_enabled()) {
        update_analog_cam(camera);

        if (analog_cam_active) {
            spAC.pitch = analog_camera_pos.pitch;
            // spAC.r = analog_camera_pos.r;
            spAC.yaw = analog_camera_pos.yaw;
        }
    }

    *sp40 = OLib_AddVecGeoToVec3f(sp44, &spAC);

    if ((camera->status == CAM_STATUS_ACTIVE) && !(roData->interfaceFlags & JUMP3_FLAG_6)) {
        if (func_800CBA7C(camera) == 0) {
            Camera_CalcDefaultSwing(camera, &spAC, &sp9C, roData->unk_04, roData->unk_0C, &D_801EDC30[camera->camId],
                                    &rwData->unk_10);
        }

        if (roData->interfaceFlags & JUMP3_FLAG_2) {
            camera->inputDir.x = -sp9C.pitch;
            camera->inputDir.y = sp9C.yaw + 0x8000;
            camera->inputDir.z = 0;
        } else {
            spAC = OLib_Vec3fDiffToVecGeo(sp48, sp44);
            camera->inputDir.x = spAC.pitch;
            camera->inputDir.y = spAC.yaw;
            camera->inputDir.z = 0;
        }
    } else {
        D_801EDC30[camera->camId].swingUpdateRate = roData->unk_0C;
        D_801EDC30[camera->camId].unk_64 = 0;
        sUpdateCameraDirection = false;
        *sp48 = *sp40;
    }

    camera->fov = Camera_ScaledStepToCeilF(roData->unk_18, camera->fov, camera->fovUpdateRate, 0.1f);
    camera->roll = Camera_ScaledStepToCeilS(0, camera->roll, .5f, 5);
    camera->atLerpStepScale = Camera_ClampLerpScale(camera, roData->unk_1C);

    return 1;
}


void analog_cam_pre_play_update(PlayState* play) {
}

void analog_cam_post_play_update(PlayState* play) {
    Camera *active_cam = play->cameraPtrs[play->activeCamId];
    // recomp_printf("prev_analog_cam_active: %d can_use_analog_cam: %d\n", prev_analog_cam_active, can_use_analog_cam);
    // recomp_printf("setting: %d mode: %d func: %d\n", active_cam->setting, active_cam->mode, sCameraSettings[active_cam->setting].cameraModes[active_cam->mode].funcId);
    // recomp_printf("active cam yaw %d\n", Camera_GetInputDirYaw(GET_ACTIVE_CAM(play)));

    // Update parameters for the analog cam if the game is unpaused.
    if (play->pauseCtx.state == PAUSE_STATE_OFF && R_PAUSE_BG_PRERENDER_STATE == PAUSE_BG_PRERENDER_OFF) {
        update_analog_camera_params(active_cam);
        can_use_analog_cam = false;
    }

    if (analog_cam_active) {
        active_cam->inputDir.x = analog_camera_pos.pitch;
        active_cam->inputDir.y = analog_camera_pos.yaw + DEG_TO_BINANG(180);
    }
}

bool get_analog_cam_active() {
    return analog_cam_active;
}

void set_analog_cam_active(bool isActive) {
    analog_cam_active = isActive;
}

// Calling this will avoid analog cam taking over for the following game loop.
// E.g. using left stick inputs while in a deku flower taking priority over right stick.
void skip_analog_cam_once() {
    analog_cam_skip_once = true;
    analog_cam_active = false;
}

extern void func_809ECD00(Boss04* this, PlayState* play);
extern s32 func_800B7298(struct PlayState* play, Actor* csActor, u8 csAction);
extern u8 D_809EE4D0;

// @recomp Patch the Wart boss fight in the Great Bay temple so that the fight starts if you look at it with the right stick analog camera,
// instead of requiring entering first person mode mode.
RECOMP_PATCH void func_809EC568(Boss04* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    f32 x;
    f32 y;
    f32 z;
    s32 pad;
    u16 maxProjectedPosToStartFight;

    // @recomp Change the maximun projected position to start the fight depending on whether analog camera is enabled or not.
    // The default vanilla value makes it so that you have to pretty much start in the corner in order to be able to get the angle it wants.
    if (analog_cam_active) {
        maxProjectedPosToStartFight = 600.0f;
    } else {
        maxProjectedPosToStartFight = 300.0f; // vanilla value
    }

    this->unk_704++;
    this->unk_1FE = 15;
    if ((this->unk_708 != 0) && (this->unk_708 < 10)) {
        this->actor.world.pos.y = (Math_SinS(this->unk_1F4 * 512) * 10.0f) + (this->actor.floorHeight + 160.0f);
        Matrix_RotateYS(this->actor.yawTowardsPlayer, MTXMODE_NEW);
    }

    switch (this->unk_708) {
        case 0:
            this->unk_2C8 = 50;
            this->unk_2D0 = 2000.0f;
            // @recomp do not require being in c-up mode if analog cam is enabled
            // also, use the new variable instead of the vanilla value to check if the player is looking at the boss.
            if (((player->stateFlags1 & PLAYER_STATE1_100000) || (recomp_get_analog_cam_enabled())) && (this->actor.projectedPos.z > 0.0f) &&
                (fabsf(this->actor.projectedPos.x) < maxProjectedPosToStartFight) && (fabsf(this->actor.projectedPos.y) < maxProjectedPosToStartFight)) {
                if ((this->unk_704 >= 15) && (CutsceneManager_GetCurrentCsId() == CS_ID_NONE)) {
                    Actor* boss;

                    this->unk_708 = 10;
                    this->unk_704 = 0;
                    Cutscene_StartManual(play, &play->csCtx);
                    this->subCamId = Play_CreateSubCamera(play);
                    Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STATUS_WAIT);
                    Play_ChangeCameraStatus(play, this->subCamId, CAM_STATUS_ACTIVE);
                    func_800B7298(play, &this->actor, PLAYER_CSACTION_WAIT);
                    player->actor.world.pos.x = this->unk_6E8;
                    player->actor.world.pos.z = this->unk_6F0 + 410.0f;
                    player->actor.shape.rot.y = 0x7FFF;
                    player->actor.world.rot.y = player->actor.shape.rot.y;
                    Math_Vec3f_Copy(&this->subCamEye, &player->actor.world.pos);
                    this->subCamEye.y += 100.0f;
                    Math_Vec3f_Copy(&this->subCamAt, &this->actor.world.pos);
                    Play_EnableMotionBlur(150);
                    this->subCamFov = 60.0f;

                    boss = play->actorCtx.actorLists[ACTORCAT_BOSS].first;
                    while (boss != NULL) {
                        if (boss->id == ACTOR_EN_WATER_EFFECT) {
                            Actor_Kill(boss);
                        }
                        boss = boss->next;
                    }
                }
            } else {
                this->unk_704 = 0;
            }
            break;

        case 10:
            if (this->unk_704 == 3) {
                Actor_PlaySfx(&this->actor, NA_SE_EN_EYEGOLE_DEMO_EYE);
                this->unk_74A = 1;
            }
            this->unk_2D0 = 10000.0f;
            this->unk_2C8 = 300;
            Math_ApproachF(&this->subCamFov, 20.0f, 0.3f, 11.0f);
            if (this->unk_704 == 40) {
                this->unk_708 = 11;
                this->unk_704 = 0;
            }
            break;

        case 11:
            if (this->unk_704 > 50) {
                this->unk_708 = 12;
                this->unk_704 = 0;
                this->actor.gravity = -3.0f;
            }
            break;

        case 13:
            if (this->unk_704 == 45) {
                this->unk_708 = 1;
                this->unk_704 = 0;
                func_800B7298(play, &this->actor, PLAYER_CSACTION_21);
                this->actor.gravity = 0.0f;
                break;
            }

        case 12:
            Actor_PlaySfx(&this->actor, NA_SE_EN_ME_ATTACK - SFX_FLAG);
            Math_ApproachF(&this->subCamAt.x, this->actor.world.pos.x, 0.5f, 1000.0f);
            Math_ApproachF(&this->subCamAt.y, this->actor.world.pos.y, 0.5f, 1000.0f);
            Math_ApproachF(&this->subCamAt.z, this->actor.world.pos.z, 0.5f, 1000.0f);
            if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND_TOUCH) {
                Audio_PlaySfx(NA_SE_IT_BIG_BOMB_EXPLOSION);
                this->unk_6F4 = 15;
                this->unk_708 = 13;
                this->unk_704 = 0;
                this->unk_2DA = 10;
                Actor_Spawn(&play->actorCtx, play, ACTOR_EN_CLEAR_TAG, this->actor.world.pos.x, this->actor.world.pos.y,
                            this->actor.world.pos.z, 0, 0, 0, CLEAR_TAG_PARAMS(CLEAR_TAG_SPLASH));
                Actor_PlaySfx(&this->actor, NA_SE_EN_KONB_JUMP_LEV_OLD - SFX_FLAG);
                this->subCamAtOscillator = 20;
            }
            break;

        case 1:
            player->actor.shape.rot.y = 0x7FFF;
            player->actor.world.rot.y = player->actor.shape.rot.y;
            Matrix_MultVecZ(-100.0f, &this->subCamEye);

            this->subCamEye.x += player->actor.world.pos.x;
            this->subCamEye.y = Player_GetHeight(player) + player->actor.world.pos.y + 36.0f;
            this->subCamEye.z += player->actor.world.pos.z;

            this->subCamAt.x = player->actor.world.pos.x;
            this->subCamAt.y = (Player_GetHeight(player) + player->actor.world.pos.y) - 4.0f;
            this->subCamAt.z = player->actor.world.pos.z;

            if (this->unk_704 >= 35) {
                this->unk_704 = 0;
                this->unk_708 = 2;
                this->unk_728 = -200.0f;
            }
            break;

        case 2:
        case 3:
            Matrix_MultVecZ(500.0f, &this->subCamEye);
            this->subCamEye.x += this->actor.world.pos.x;
            this->subCamEye.y += this->actor.world.pos.y - 50.0f;
            this->subCamEye.z += this->actor.world.pos.z;
            this->subCamAt.x = this->actor.world.pos.x;
            this->subCamAt.z = this->actor.world.pos.z;
            this->subCamAt.y = (this->actor.world.pos.y - 70.0f) + this->unk_728;
            Math_ApproachZeroF(&this->unk_728, 0.05f, this->unk_73C);
            Math_ApproachF(&this->unk_73C, 3.0f, 1.0f, 0.05f);
            if (this->unk_704 == 20) {
                this->unk_708 = 3;
            }

            if (this->unk_704 == 70) {
                this->unk_2C8 = 300;
                this->unk_2D0 = 0.0f;

                D_809EE4D0 = 1;
                this->unk_2E2 = 60;
                this->unk_2E0 = 93;
            }

            if (this->unk_704 > 140) {
                Camera* mainCam = Play_GetCamera(play, CAM_ID_MAIN);

                this->unk_708 = 0;
                func_809ECD00(this, play);
                mainCam->eye = this->subCamEye;
                mainCam->eyeNext = this->subCamEye;
                mainCam->at = this->subCamAt;
                func_80169AFC(play, this->subCamId, 0);
                this->subCamId = SUB_CAM_ID_DONE;
                Cutscene_StopManual(play, &play->csCtx);
                func_800B7298(play, &this->actor, PLAYER_CSACTION_END);
                Play_DisableMotionBlur();
                SET_EVENTINF(EVENTINF_60);
            }
            break;
    }

    if (this->subCamId != SUB_CAM_ID_DONE) {
        Vec3f subCamAt;

        ShrinkWindow_Letterbox_SetSizeTarget(27);
        if (this->subCamAtOscillator != 0) {
            this->subCamAtOscillator--;
        }
        Math_Vec3f_Copy(&subCamAt, &this->subCamAt);
        subCamAt.y += Math_SinS(this->subCamAtOscillator * 0x4000) * this->subCamAtOscillator * 1.5f;
        Play_SetCameraAtEye(play, this->subCamId, &subCamAt, &this->subCamEye);
        Play_SetCameraFov(play, this->subCamId, this->subCamFov);
        Math_ApproachF(&this->subCamFov, 60.0f, 0.1f, 1.0f);
    }
    this->actor.shape.rot.y = this->actor.yawTowardsPlayer;
    x = player->actor.world.pos.x - this->actor.world.pos.x;
    y = player->actor.world.pos.y - this->actor.world.pos.y;
    z = player->actor.world.pos.z - this->actor.world.pos.z;
    this->actor.shape.rot.x = Math_Atan2S(-y, sqrtf(SQ(x) + SQ(z)));
}
