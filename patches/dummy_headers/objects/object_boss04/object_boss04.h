// Required to include MM decomp headers without having built the repo

#ifndef OBJECT_BOSS04_H
#define OBJECT_BOSS04_H 1

typedef enum WartLimb {
    /* 0x00 */ WART_LIMB_NONE,
    /* 0x01 */ WART_LIMB_ROOT,
    /* 0x02 */ WART_LIMB_BODY,
    /* 0x03 */ WART_LIMB_EYE_ROOT,
    /* 0x04 */ WART_LIMB_EYE,
    /* 0x05 */ WART_LIMB_TOP_EYELID_ROOT,
    /* 0x06 */ WART_LIMB_TOP_EYELID,
    /* 0x07 */ WART_LIMB_BOTTOM_EYELID_ROOT,
    /* 0x08 */ WART_LIMB_BOTTOM_EYELID,
    /* 0x09 */ WART_LIMB_MAX
} WartLimb;

extern s16 sWartIdleAnimFrameData[];
extern JointIndex sWartIdleAnimJointIndices[];
extern AnimationHeader gWartIdleAnim;
extern Vtx object_boss04Vtx_000060[];
extern Gfx gWartShellDL[];
extern Gfx gWartBottomEyelidDL[];
extern Gfx gWartTopEyelidDL[];
extern Gfx gWartEyeDL[];
extern u64 gWartShellTLUT[];
extern u64 gWartRidgesTLUT[];
extern u64 gWartShellTex[];
extern u64 gWartRidgesTex[];
extern u64 gWartEyeTex[];
extern Vtx object_boss04Vtx_0033A8[];
extern Gfx gWartBubbleOpaqueMaterialDL[];
extern Gfx gWartBubbleMaterialDL[];
extern Gfx gWartBubbleModelDL[];
extern u64 gWartBubbleTex[];
extern Vtx object_boss04Vtx_003CE0[];
extern u64 gWartShadowTex[];
extern Gfx gWartShadowMaterialDL[];
extern Gfx gWartShadowModelDL[];
extern StandardLimb gWartRootLimb;
extern StandardLimb gWartBodyLimb;
extern StandardLimb gWartEyeRootLimb;
extern StandardLimb gWartEyeLimb;
extern StandardLimb gWartTopEyelidRootLimb;
extern StandardLimb gWartTopEyelidLimb;
extern StandardLimb gWartBottomEyelidRootLimb;
extern StandardLimb gWartBottomEyelidLimb;
extern void* gWartSkelLimbs[];
extern FlexSkeletonHeader gWartSkel;
#endif
