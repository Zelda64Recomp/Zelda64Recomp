// Required to include MM decomp headers without having built the repo

#ifndef OBJECT_BOSS_HAKUGIN_H
#define OBJECT_BOSS_HAKUGIN_H 1

typedef enum GohtLimb {
    /* 0x00 */ GOHT_LIMB_NONE,
    /* 0x01 */ GOHT_LIMB_ROOT,
    /* 0x02 */ GOHT_LIMB_PELVIS,
    /* 0x03 */ GOHT_LIMB_THORAX_ROOT,
    /* 0x04 */ GOHT_LIMB_THORAX_WRAPPER,
    /* 0x05 */ GOHT_LIMB_FRONT_RIGHT_LEG_ROOT,
    /* 0x06 */ GOHT_LIMB_FRONT_RIGHT_UPPER_LEG,
    /* 0x07 */ GOHT_LIMB_FRONT_RIGHT_LOWER_LEG_ROOT,
    /* 0x08 */ GOHT_LIMB_FRONT_RIGHT_LOWER_LEG,
    /* 0x09 */ GOHT_LIMB_FRONT_RIGHT_HOOF,
    /* 0x0A */ GOHT_LIMB_FRONT_LEFT_LEG_ROOT,
    /* 0x0B */ GOHT_LIMB_FRONT_LEFT_UPPER_LEG,
    /* 0x0C */ GOHT_LIMB_FRONT_LEFT_LOWER_LEG_ROOT,
    /* 0x0D */ GOHT_LIMB_FRONT_LEFT_LOWER_LEG,
    /* 0x0E */ GOHT_LIMB_FRONT_LEFT_HOOF,
    /* 0x0F */ GOHT_LIMB_THORAX,
    /* 0x10 */ GOHT_LIMB_HEAD,
    /* 0x11 */ GOHT_LIMB_JAW_ROOT,
    /* 0x12 */ GOHT_LIMB_JAW,
    /* 0x13 */ GOHT_LIMB_BACK_RIGHT_LEG_ROOT,
    /* 0x14 */ GOHT_LIMB_BACK_RIGHT_LEG_WRAPPER,
    /* 0x15 */ GOHT_LIMB_BACK_RIGHT_THIGH,
    /* 0x16 */ GOHT_LIMB_BACK_RIGHT_SHIN,
    /* 0x17 */ GOHT_LIMB_BACK_RIGHT_PASTERN_ROOT,
    /* 0x18 */ GOHT_LIMB_BACK_RIGHT_PASTERN,
    /* 0x19 */ GOHT_LIMB_BACK_RIGHT_HOOF,
    /* 0x1A */ GOHT_LIMB_BACK_LEFT_LEG_ROOT,
    /* 0x1B */ GOHT_LIMB_BACK_LEFT_LEG_WRAPPER,
    /* 0x1C */ GOHT_LIMB_BACK_LEFT_THIGH,
    /* 0x1D */ GOHT_LIMB_BACK_LEFT_SHIN,
    /* 0x1E */ GOHT_LIMB_BACK_LEFT_PASTERN_ROOT,
    /* 0x1F */ GOHT_LIMB_BACK_LEFT_PASTERN,
    /* 0x20 */ GOHT_LIMB_BACK_LEFT_HOOF,
    /* 0x21 */ GOHT_LIMB_MAX
} GohtLimb;

extern s16 sGohtThawAndBreakWallAnimFrameData[];
extern JointIndex sGohtThawAndBreakWallAnimJointIndices[];
extern AnimationHeader gGohtThawAndBreakWallAnim;
extern s16 sGohtWritheOnSideAnimFrameData[];
extern JointIndex sGohtWritheOnSideAnimJointIndices[];
extern AnimationHeader gGohtWritheOnSideAnim;
extern s16 sGohtTwitchOnSideAnimFrameData[];
extern JointIndex sGohtTwitchOnSideAnimJointIndices[];
extern AnimationHeader gGohtTwitchOnSideAnim;
extern s16 sGohtFallOnSideAnimFrameData[];
extern JointIndex sGohtFallOnSideAnimJointIndices[];
extern AnimationHeader gGohtFallOnSideAnim;
extern s16 sGohtStationaryAnimFrameData[];
extern JointIndex sGohtStationaryAnimJointIndices[];
extern AnimationHeader gGohtStationaryAnim;
extern Vtx object_boss_hakuginVtx_0031B0[];
extern Gfx gGohtPelvisDL[];
extern Gfx gGohtBackLeftHoofDL[];
extern Gfx gGohtBackLeftPasternDL[];
extern Gfx gGohtBackLeftShinDL[];
extern Gfx gGohtBackLeftThighDL[];
extern Gfx gGohtBackRightHoofDL[];
extern Gfx gGohtBackRightPasternDL[];
extern Gfx gGohtBackRightShinDL[];
extern Gfx gGohtBackRightThighDL[];
extern Gfx gGohtHeadDL[];
extern Gfx gGohtJawDL[];
extern Gfx gGohtThoraxDL[];
extern Gfx gGohtFrontLeftHoofDL[];
extern Gfx gGohtFrontLeftLowerLegDL[];
extern Gfx gGohtFrontLeftUpperLegDL[];
extern Gfx gGohtFrontRightHoofDL[];
extern Gfx gGohtFrontRightLowerLegDL[];
extern Gfx gGohtFrontRightUpperLegDL[];
extern u64 gGohtMachineryTex[];
extern u64 gGohtMetalPlateWithCirclePatternTex[];
extern u64 gGohtMetalPlateWithMulticoloredPatternTex[];
extern u64 gGohtEyeTex[];
extern u64 gGohtFaceAndKneePatternTex[];
extern u64 gGohtHornTex[];
extern Vtx object_boss_hakuginVtx_0102D8[];
extern Gfx gGohtRockMaterialDL[];
extern Gfx gGohtRockModelDL[];
extern u64 gGohtRockTex[];
extern Vtx object_boss_hakuginVtx_010F80[];
extern Gfx gGohtStalactiteMaterialDL[];
extern Gfx gGohtStalactiteModelDL[];
extern Vtx object_boss_hakuginVtx_0111C8[];
extern Gfx gGohtLightningMaterialDL[];
extern Gfx gGohtLightningModelDL[];
extern u64 gGohtLightningTex[];
extern u8 gGohtUnusedZeroesBlob[];
extern u64 gGohtLightOrbTex[];
extern Vtx object_boss_hakuginVtx_012E90[];
extern Gfx gGohtLightOrbMaterialDL[];
extern Gfx gGohtLightOrbModelDL[];
extern StandardLimb gGohtRootLimb;
extern StandardLimb gGohtPelvisLimb;
extern StandardLimb gGohtThoraxRootLimb;
extern StandardLimb gGohtThoraxWrapperLimb;
extern StandardLimb gGohtFrontRightLegRootLimb;
extern StandardLimb gGohtFrontRightUpperLegLimb;
extern StandardLimb gGohtFrontRightLowerLegRootLimb;
extern StandardLimb gGohtFrontRightLowerLegLimb;
extern StandardLimb gGohtFrontRightHoofLimb;
extern StandardLimb gGohtFrontLeftLegRootLimb;
extern StandardLimb gGohtFrontLeftUpperLegLimb;
extern StandardLimb gGohtFrontLeftLowerLegRootLimb;
extern StandardLimb gGohtFrontLeftLowerLegLimb;
extern StandardLimb gGohtFrontLeftHoofLimb;
extern StandardLimb gGohtThoraxLimb;
extern StandardLimb gGohtHeadLimb;
extern StandardLimb gGohtJawRootLimb;
extern StandardLimb gGohtJawLimb;
extern StandardLimb gGohtBackRightLegRootLimb;
extern StandardLimb gGohtBackRightLegWrapperLimb;
extern StandardLimb gGohtBackRightThighLimb;
extern StandardLimb gGohtBackRightShinLimb;
extern StandardLimb gGohtBackRightPasternRootLimb;
extern StandardLimb gGohtBackRightPasternLimb;
extern StandardLimb gGohtBackRightHoofLimb;
extern StandardLimb gGohtBackLeftLegRootLimb;
extern StandardLimb gGohtBackLeftLegWrapperLimb;
extern StandardLimb gGohtBackLeftThighLimb;
extern StandardLimb gGohtBackLeftShinLimb;
extern StandardLimb gGohtBackLeftPasternRootLimb;
extern StandardLimb gGohtBackLeftPasternLimb;
extern StandardLimb gGohtBackLeftHoofLimb;
extern void* gGohtSkelLimbs[];
extern FlexSkeletonHeader gGohtSkel;
extern s16 sGohtFallDownAnimFrameData[];
extern JointIndex sGohtFallDownAnimJointIndices[];
extern AnimationHeader gGohtFallDownAnim;
extern s16 sGohtRunAnimFrameData[];
extern JointIndex sGohtRunAnimJointIndices[];
extern AnimationHeader gGohtRunAnim;
extern s16 sGohtGetUpFromSideAnimFrameData[];
extern JointIndex sGohtGetUpFromSideAnimJointIndices[];
extern AnimationHeader gGohtGetUpFromSideAnim;
extern u64 gGohtTitleCardTex[];
#endif
