// Required to include MM decomp headers without having built the repo

#ifndef OBJECT_HA_H
#define OBJECT_HA_H 1

typedef enum DonkeyBanditLimb {
    /* 0x00 */ HORSE_BANDIT_LIMB_NONE,
    /* 0x01 */ HORSE_BANDIT_LIMB_ROOT,
    /* 0x02 */ HORSE_BANDIT_LIMB_PELVIS,
    /* 0x03 */ HORSE_BANDIT_LIMB_TORSO,
    /* 0x04 */ HORSE_BANDIT_LIMB_LEFT_FRONT_LEG_ROOT,
    /* 0x05 */ HORSE_BANDIT_LIMB_LEFT_FRONT_FOREARM,
    /* 0x06 */ HORSE_BANDIT_LIMB_LEFT_FRONT_CANNON,
    /* 0x07 */ HORSE_BANDIT_LIMB_LEFT_FRONT_FOOT,
    /* 0x08 */ HORSE_BANDIT_LIMB_LOWER_NECK,
    /* 0x09 */ HORSE_BANDIT_LIMB_UPPER_NECK,
    /* 0x0A */ HORSE_BANDIT_LIMB_HEAD,
    /* 0x0B */ HORSE_BANDIT_LIMB_RIGHT_FRONT_LEG_ROOT,
    /* 0x0C */ HORSE_BANDIT_LIMB_RIGHT_FRONT_FOREARM,
    /* 0x0D */ HORSE_BANDIT_LIMB_RIGHT_FRONT_CANNON,
    /* 0x0E */ HORSE_BANDIT_LIMB_RIGHT_FRONT_FOOT,
    /* 0x0F */ HORSE_BANDIT_LIMB_TAIL_DOCK,
    /* 0x10 */ HORSE_BANDIT_LIMB_TAIL_MIDDLE,
    /* 0x11 */ HORSE_BANDIT_LIMB_TAIL_END,
    /* 0x12 */ HORSE_BANDIT_LIMB_LEFT_HIND_THIGH,
    /* 0x13 */ HORSE_BANDIT_LIMB_LEFT_HIND_STIFLE,
    /* 0x14 */ HORSE_BANDIT_LIMB_LEFT_HIND_CANNON,
    /* 0x15 */ HORSE_BANDIT_LIMB_LEFT_HIND_FOOT,
    /* 0x16 */ HORSE_BANDIT_LIMB_RIGHT_HIND_THIGH,
    /* 0x17 */ HORSE_BANDIT_LIMB_RIGHT_HIND_STIFLE,
    /* 0x18 */ HORSE_BANDIT_LIMB_RIGHT_HIND_CANNON,
    /* 0x19 */ HORSE_BANDIT_LIMB_RIGHT_HIND_FOOT,
    /* 0x1A */ HORSE_BANDIT_LIMB_MAX
} DonkeyBanditLimb;

typedef enum DonkeyLimb {
    /* 0x00 */ DONKEY_LIMB_NONE,
    /* 0x01 */ DONKEY_LIMB_ROOT,
    /* 0x02 */ DONKEY_LIMB_PELVIS,
    /* 0x03 */ DONKEY_LIMB_TORSO,
    /* 0x04 */ DONKEY_LIMB_LEFT_FRONT_LEG_ROOT,
    /* 0x05 */ DONKEY_LIMB_LEFT_FRONT_FOREARM,
    /* 0x06 */ DONKEY_LIMB_LEFT_FRONT_CANNON,
    /* 0x07 */ DONKEY_LIMB_LEFT_FRONT_FOOT,
    /* 0x08 */ DONKEY_LIMB_LOWER_NECK,
    /* 0x09 */ DONKEY_LIMB_UPPER_NECK,
    /* 0x0A */ DONKEY_LIMB_HEAD,
    /* 0x0B */ DONKEY_LIMB_RIGHT_FRONT_LEG_ROOT,
    /* 0x0C */ DONKEY_LIMB_RIGHT_FRONT_FOREARM,
    /* 0x0D */ DONKEY_LIMB_RIGHT_FRONT_CANNON,
    /* 0x0E */ DONKEY_LIMB_RIGHT_FRONT_FOOT,
    /* 0x0F */ DONKEY_LIMB_TAIL_DOCK,
    /* 0x10 */ DONKEY_LIMB_TAIL_MIDDLE,
    /* 0x11 */ DONKEY_LIMB_TAIL_END,
    /* 0x12 */ DONKEY_LIMB_LEFT_HIND_THIGH,
    /* 0x13 */ DONKEY_LIMB_LEFT_HIND_STIFLE,
    /* 0x14 */ DONKEY_LIMB_LEFT_HIND_CANNON,
    /* 0x15 */ DONKEY_LIMB_LEFT_HIND_FOOT,
    /* 0x16 */ DONKEY_LIMB_RIGHT_HIND_THIGH,
    /* 0x17 */ DONKEY_LIMB_RIGHT_HIND_STIFLE,
    /* 0x18 */ DONKEY_LIMB_RIGHT_HIND_CANNON,
    /* 0x19 */ DONKEY_LIMB_RIGHT_HIND_FOOT,
    /* 0x1A */ DONKEY_LIMB_MAX
} DonkeyLimb;

extern s16 sHorseUnusedAnimFrameData[];
extern JointIndex sHorseUnusedAnimJointIndices[];
extern AnimationHeader gHorseUnusedAnim;
extern Vtx object_haVtx_0000E0[];
extern Gfx gHorseBanditRightHindFootDL[];
extern Gfx gHorseBanditRightHindCannonDL[];
extern Gfx gHorseBanditRightHindStifleDL[];
extern Gfx gHorseBanditRightHindThighDL[];
extern Gfx gHorseBanditLeftHindFootDL[];
extern Gfx gHorseBanditLeftHindCannonDL[];
extern Gfx gHorseBanditLeftHindStifleDL[];
extern Gfx gHorseBanditLeftHindThighDL[];
extern Gfx gHorseBanditTailEndDL[];
extern Gfx gHorseBanditTailMiddleDL[];
extern Gfx gHorseBanditTailDockDL[];
extern Gfx gHorseBanditPelvisDL[];
extern Gfx gHorseBanditRightFrontFootDL[];
extern Gfx gHorseBanditRightFrontCannonDL[];
extern Gfx gHorseBanditRightFrontForearmDL[];
extern Gfx gHorseBanditRightFrontLegRootDL[];
extern Gfx gHorseBanditHeadDL[];
extern Gfx gHorseBanditUpperNeckDL[];
extern Gfx gHorseBanditLowerNeckDL[];
extern Gfx gHorseBanditLeftFrontFootDL[];
extern Gfx gHorseBanditLeftFrontCannonDL[];
extern Gfx gHorseBanditLeftFrontForearmDL[];
extern Gfx gHorseBanditLeftFrontLegRootDL[];
extern Gfx gHorseBanditTorsoDL[];
extern u64 gHorseBanditTLUT[];
extern u64 gHorseBanditMaskTex[];
extern u64 gHorseBanditSpottedDetailTex[];
extern u64 gHorseBanditEyebrowTex[];
extern u64 gHorseBanditTeethTex[];
extern u64 gHorseBanditTailAndFeetTex[];
extern u64 gHorseBanditEyeTex[];
extern u64 gHorseBanditMouthTex[];
extern u64 gHorseBanditSpottedSkinTex[];
extern u64 gHorseBanditTasselTex[];
extern u64 gHorseBanditSaddleSideTex[];
extern u64 gHorseBanditSaddleTopTex[];
extern u64 gHorseBanditSaddleBackTex[];
extern StandardLimb gHorseBanditRootLimb;
extern StandardLimb gHorseBanditPelvisLimb;
extern StandardLimb gHorseBanditTorsoLimb;
extern StandardLimb gHorseBanditLeftFrontLegRootLimb;
extern StandardLimb gHorseBanditLeftFrontForearmLimb;
extern StandardLimb gHorseBanditLeftFrontCannonLimb;
extern StandardLimb gHorseBanditLeftFrontFootLimb;
extern StandardLimb gHorseBanditLowerNeckLimb;
extern StandardLimb gHorseBanditUpperNeckLimb;
extern StandardLimb gHorseBanditHeadLimb;
extern StandardLimb gHorseBanditRightFrontLegRootLimb;
extern StandardLimb gHorseBanditRightFrontForearmLimb;
extern StandardLimb gHorseBanditRightFrontCannonLimb;
extern StandardLimb gHorseBanditRightFrontFootLimb;
extern StandardLimb gHorseBanditTailDockLimb;
extern StandardLimb gHorseBanditTailMiddleLimb;
extern StandardLimb gHorseBanditTailEndLimb;
extern StandardLimb gHorseBanditLeftHindThighLimb;
extern StandardLimb gHorseBanditLeftHindStifleLimb;
extern StandardLimb gHorseBanditLeftHindCannonLimb;
extern StandardLimb gHorseBanditLeftHindFootLimb;
extern StandardLimb gHorseBanditRightHindThighLimb;
extern StandardLimb gHorseBanditRightHindStifleLimb;
extern StandardLimb gHorseBanditRightHindCannonLimb;
extern StandardLimb gHorseBanditRightHindFootLimb;
extern void* gHorseBanditSkelLimbs[];
extern FlexSkeletonHeader gHorseBanditSkel;
extern s16 sHorseGallopAnimFrameData[];
extern JointIndex sHorseGallopAnimJointIndices[];
extern AnimationHeader gHorseGallopAnim;
extern s16 sHorseJumpLowAnimFrameData[];
extern JointIndex sHorseJumpLowAnimJointIndices[];
extern AnimationHeader gHorseJumpLowAnim;
extern s16 sHorseJumpHighAnimFrameData[];
extern JointIndex sHorseJumpHighAnimJointIndices[];
extern AnimationHeader gHorseJumpHighAnim;
extern s16 sHorseTrotAnimFrameData[];
extern JointIndex sHorseTrotAnimJointIndices[];
extern AnimationHeader gHorseTrotAnim;
extern s16 sHorseWhinnyAnimFrameData[];
extern JointIndex sHorseWhinnyAnimJointIndices[];
extern AnimationHeader gHorseWhinnyAnim;
extern s16 sHorseStopAnimFrameData[];
extern JointIndex sHorseStopAnimJointIndices[];
extern AnimationHeader gHorseStopAnim;
extern s16 sHorseIdleAnimFrameData[];
extern JointIndex sHorseIdleAnimJointIndices[];
extern AnimationHeader gHorseIdleAnim;
extern s16 sHorseShakeHeadAnimFrameData[];
extern JointIndex sHorseShakeHeadAnimJointIndices[];
extern AnimationHeader gHorseShakeHeadAnim;
extern s16 sHorseWalkAnimFrameData[];
extern JointIndex sHorseWalkAnimJointIndices[];
extern AnimationHeader gHorseWalkAnim;
extern Vtx object_haVtx_00D660[];
extern Gfx gDonkeyRightHindFootDL[];
extern Gfx gDonkeyRightHindCannonDL[];
extern Gfx gDonkeyRightHindStifleDL[];
extern Gfx gDonkeyRightHindThighDL[];
extern Gfx gDonkeyLeftHindFootDL[];
extern Gfx gDonkeyLeftHindCannonDL[];
extern Gfx gDonkeyLeftHindStifleDL[];
extern Gfx gDonkeyLeftHindThighDL[];
extern Gfx gDonkeyTailEndDL[];
extern Gfx gDonkeyTailMiddleDL[];
extern Gfx gDonkeyTailDockDL[];
extern Gfx gDonkeyPelvisDL[];
extern Gfx gDonkeyRightFrontFootDL[];
extern Gfx gDonkeyRightFrontCannonDL[];
extern Gfx gDonkeyRightFrontForearmDL[];
extern Gfx gDonkeyRightFrontLegRootDL[];
extern Gfx gDonkeyHeadDL[];
extern Gfx gDonkeyUpperNeckDL[];
extern Gfx gDonkeyLowerNeckDL[];
extern Gfx gDonkeyLeftFrontFootDL[];
extern Gfx gDonkeyLeftFrontCannonDL[];
extern Gfx gDonkeyLeftFrontForearmDL[];
extern Gfx gDonkeyLeftFrontLegRootDL[];
extern Gfx gDonkeyTorsoDL[];
extern u64 gDonkeyTLUT[];
extern u64 gDonkeyMouthTex[];
extern u64 gDonkeyHeadTex[];
extern u64 gDonkeyEyeTex[];
extern u64 gDonkeyManeTex[];
extern u64 gDonkeySkinTex[];
extern u64 gDonkeyHarnessTex[];
extern u64 gDonkeyTailAndFeetTex[];
extern StandardLimb gDonkeyRootLimb;
extern StandardLimb gDonkeyPelvisLimb;
extern StandardLimb gDonkeyTorsoLimb;
extern StandardLimb gDonkeyLeftFrontLegRootLimb;
extern StandardLimb gDonkeyLeftFrontForearmLimb;
extern StandardLimb gDonkeyLeftFrontCannonLimb;
extern StandardLimb gDonkeyLeftFrontFootLimb;
extern StandardLimb gDonkeyLowerNeckLimb;
extern StandardLimb gDonkeyUpperNeckLimb;
extern StandardLimb gDonkeyHeadLimb;
extern StandardLimb gDonkeyRightFrontLegRootLimb;
extern StandardLimb gDonkeyRightFrontForearmLimb;
extern StandardLimb gDonkeyRightFrontCannonLimb;
extern StandardLimb gDonkeyRightFrontFootLimb;
extern StandardLimb gDonkeyTailDockLimb;
extern StandardLimb gDonkeyTailMiddleLimb;
extern StandardLimb gDonkeyTailEndLimb;
extern StandardLimb gDonkeyLeftHindThighLimb;
extern StandardLimb gDonkeyLeftHindStifleLimb;
extern StandardLimb gDonkeyLeftHindCannonLimb;
extern StandardLimb gDonkeyLeftHindFootLimb;
extern StandardLimb gDonkeyRightHindThighLimb;
extern StandardLimb gDonkeyRightHindStifleLimb;
extern StandardLimb gDonkeyRightHindCannonLimb;
extern StandardLimb gDonkeyRightHindFootLimb;
extern void* gDonkeySkelLimbs[];
extern FlexSkeletonHeader gDonkeySkel;
#endif
