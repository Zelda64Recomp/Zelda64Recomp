// Required to include MM decomp headers without having built the repo

#ifndef OBJECT_BOSS03_H
#define OBJECT_BOSS03_H 1

typedef enum GyorgLimb {
    /* 0x00 */ GYORG_LIMB_NONE,
    /* 0x01 */ GYORG_LIMB_ROOT,
    /* 0x02 */ GYORG_LIMB_HEAD,
    /* 0x03 */ GYORG_LIMB_BODY_ROOT,
    /* 0x04 */ GYORG_LIMB_UPPER_TRUNK,
    /* 0x05 */ GYORG_LIMB_LOWER_TRUNK,
    /* 0x06 */ GYORG_LIMB_TAIL,
    /* 0x07 */ GYORG_LIMB_RIGHT_FIN_ROOT,
    /* 0x08 */ GYORG_LIMB_UPPER_RIGHT_FIN,
    /* 0x09 */ GYORG_LIMB_LOWER_RIGHT_FIN,
    /* 0x0A */ GYORG_LIMB_LEFT_FIN_ROOT,
    /* 0x0B */ GYORG_LIMB_UPPER_LEFT_FIN,
    /* 0x0C */ GYORG_LIMB_LOWER_LEFT_FIN,
    /* 0x0D */ GYORG_LIMB_JAW_ROOT,
    /* 0x0E */ GYORG_LIMB_JAW,
    /* 0x0F */ GYORG_LIMB_MAX
} GyorgLimb;

typedef enum GyorgSmallFishLimb {
    /* 0x00 */ GYORG_SMALL_FISH_LIMB_NONE,
    /* 0x01 */ GYORG_SMALL_FISH_LIMB_ROOT,
    /* 0x02 */ GYORG_SMALL_FISH_LIMB_BODY_ROOT,
    /* 0x03 */ GYORG_SMALL_FISH_LIMB_TRUNK_ROOT,
    /* 0x04 */ GYORG_SMALL_FISH_LIMB_TAIL_FIN,
    /* 0x05 */ GYORG_SMALL_FISH_LIMB_TRUNK,
    /* 0x06 */ GYORG_SMALL_FISH_LIMB_LEFT_FIN,
    /* 0x07 */ GYORG_SMALL_FISH_LIMB_DORSAL_FIN,
    /* 0x08 */ GYORG_SMALL_FISH_LIMB_RIGHT_FIN,
    /* 0x09 */ GYORG_SMALL_FISH_LIMB_HEAD,
    /* 0x0A */ GYORG_SMALL_FISH_LIMB_MAX
} GyorgSmallFishLimb;

extern s16 sGyorgIdleAnimFrameData[];
extern JointIndex sGyorgIdleAnimJointIndices[];
extern AnimationHeader gGyorgIdleAnim;
extern u64 gGyorgUnusedMajorasWrathWhipTex[];
extern Vtx object_boss03Vtx_0001A0[];
extern Gfx gGyorgUnusedMajorasWrathWhipDL1[];
extern Gfx gGyorgUnusedMajorasWrathWhipDL2[];
extern Gfx gGyorgUnusedMajorasWrathWhipDL3[];
extern Vtx object_boss03Vtx_0002D8[];
extern Gfx gGyorgHeadDL[];
extern Gfx gGyorgJawDL[];
extern Gfx gGyorgUpperLeftFinDL[];
extern Gfx gGyorgLowerLeftFinDL[];
extern Gfx gGyorgUpperRightFinDL[];
extern Gfx gGyorgLowerRightFinDL[];
extern Gfx gGyorgUpperTrunkDL[];
extern Gfx gGyorgLowerTrunkDL[];
extern Gfx gGyorgTailDL[];
extern AnimatedMaterial gGyorgUnused5388TexAnim[];
extern u64 gGyorgFinsSpikesAndJawTLUT[];
extern u64 gGyorgSidesTLUT[];
extern u64 gGyorgMouthAndSpikeBacksideTLUT[];
extern u64 gGyorgBellyAndFinFleshTLUT[];
extern u64 gGyorgEyeTex[];
extern u64 gGyorgFinsSpikesAndJawTex[];
extern u64 gGyorgSidesTex[];
extern u64 gGyorgMouthAndSpikeBacksideTex[];
extern u64 gGyorgBellyAndFinFleshTex[];
extern u64 gGyorgHornsTeethAndClawsTex[];
extern Vtx object_boss03Vtx_007E10[];
extern Gfx gGyorgBubbleMaterialDL[];
extern Gfx gGyorgBubbleModelDL[];
extern u64 gGyorgTitleCardTex[];
extern StandardLimb gGyorgRootLimb;
extern StandardLimb gGyorgHeadLimb;
extern StandardLimb gGyorgBodyRootLimb;
extern StandardLimb gGyorgUpperTrunkLimb;
extern StandardLimb gGyorgLowerTrunkLimb;
extern StandardLimb gGyorgTailLimb;
extern StandardLimb gGyorgRightFinRootLimb;
extern StandardLimb gGyorgUpperRightFinLimb;
extern StandardLimb gGyorgLowerRightFinLimb;
extern StandardLimb gGyorgLeftFinRootLimb;
extern StandardLimb gGyorgUpperLeftFinLimb;
extern StandardLimb gGyorgLowerLeftFinLimb;
extern StandardLimb gGyorgJawRootLimb;
extern StandardLimb gGyorgJawLimb;
extern void* gGyorgSkelLimbs[];
extern FlexSkeletonHeader gGyorgSkel;
extern s16 sGyorgFloppingAnimFrameData[];
extern JointIndex sGyorgFloppingAnimJointIndices[];
extern AnimationHeader gGyorgFloppingAnim;
extern s16 sGyorgJumpingAnimFrameData[];
extern JointIndex sGyorgJumpingAnimJointIndices[];
extern AnimationHeader gGyorgJumpingAnim;
extern s16 sGyorgStunnedAnimFrameData[];
extern JointIndex sGyorgStunnedAnimJointIndices[];
extern AnimationHeader gGyorgStunnedAnim;
extern s16 sGyorgBackingUpAnimFrameData[];
extern JointIndex sGyorgBackingUpAnimJointIndices[];
extern AnimationHeader gGyorgBackingUpAnim;
extern s16 sGyorgFastSwimmingAnimFrameData[];
extern JointIndex sGyorgFastSwimmingAnimJointIndices[];
extern AnimationHeader gGyorgFastSwimmingAnim;
extern s16 sGyorgGentleSwimmingAnimFrameData[];
extern JointIndex sGyorgGentleSwimmingAnimJointIndices[];
extern AnimationHeader gGyorgGentleSwimmingAnim;
extern s16 sGyorgTailSweepAnimFrameData[];
extern JointIndex sGyorgTailSweepAnimJointIndices[];
extern AnimationHeader gGyorgTailSweepAnim;
extern s16 sGyorgCrawlingAnimFrameData[];
extern JointIndex sGyorgCrawlingAnimJointIndices[];
extern AnimationHeader gGyorgCrawlingAnim;
extern Vtx object_boss03Vtx_00A6E0[];
extern Gfx gGyorgSeaweedPiece1DL[];
extern Gfx gGyorgSeaweedPiece2DL[];
extern Gfx gGyorgSeaweedPiece3DL[];
extern Gfx gGyorgSeaweedPiece4DL[];
extern Gfx gGyorgSeaweedPiece5DL[];
extern Gfx gGyorgSeaweedTopDL[];
extern u64 gGyorgSeaweedTopTLUT[];
extern u64 gGyorgSeaweedTLUT[];
extern u64 gGyorgSeaweedTopTex[];
extern u64 gGyorgSeaweedTex[];
extern AnimatedMaterial gGyorgUnusedBB00TexAnim[];
extern Vtx object_boss03Vtx_00BB10[];
extern Gfx gUnusedGyorgSmallFishHeadDL[];
extern Gfx gUnusedGyorgSmallFishTrunkDL[];
extern Gfx gUnusedGyorgSmallFishRightFinDL[];
extern Gfx gUnusedGyorgSmallFishDorsalFinDL[];
extern Gfx gUnusedGyorgSmallFishLeftFinDL[];
extern Gfx gUnusedGyorgSmallFishTailFinDL[];
extern u64 gUnusedGyorgSmallFishTLUT[];
extern u64 gUnusedGyorgSmallFishTex[];
extern Vtx object_boss03Vtx_00CA50[];
extern Gfx gGyorgSmallFishHeadDL[];
extern Gfx gGyorgSmallFishTrunkDL[];
extern Gfx gGyorgSmallFishRightFinDL[];
extern Gfx gGyorgSmallFishDorsalFinDL[];
extern Gfx gGyorgSmallFishLeftFinDL[];
extern Gfx gGyorgSmallFishTailFinDL[];
extern u64 gGyorgSmallFishTLUT[];
extern u64 gGyorgSmallFishTex[];
extern StandardLimb gGyorgSmallFishRootLimb;
extern StandardLimb gGyorgSmallFishBodyRootLimb;
extern StandardLimb gGyorgSmallFishTrunkRootLimb;
extern StandardLimb gGyorgSmallFishTailFinLimb;
extern StandardLimb gGyorgSmallFishTrunkLimb;
extern StandardLimb gGyorgSmallFishLeftFinLimb;
extern StandardLimb gGyorgSmallFishDorsalFinLimb;
extern StandardLimb gGyorgSmallFishRightFinLimb;
extern StandardLimb gGyorgSmallFishHeadLimb;
extern void* gGyorgSmallFishSkelLimbs[];
extern FlexSkeletonHeader gGyorgSmallFishSkel;
extern s16 sGyorgSmallFishSwimAnimFrameData[];
extern JointIndex sGyorgSmallFishSwimAnimJointIndices[];
extern AnimationHeader gGyorgSmallFishSwimAnim;
#endif
