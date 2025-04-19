#include "patches.h"

// @recomp Leave the entire KSEG0 range unmodified when translating to a virtual address. This will allow
// using the entirety of the extended RAM address space for custom assets. 
RECOMP_PATCH void* Lib_SegmentedToVirtual(void* ptr) {
    if (IS_KSEG0(ptr)) {
        return ptr;
    }
    else {
        return SEGMENTED_TO_K0(ptr);
    }
}

AnimationEntry* AnimationContext_AddEntry(AnimationContext* animationCtx, AnimationType type);
#define LINK_ANIMETION_OFFSET(addr, offset) \
    (SEGMENT_ROM_START(link_animetion) + ((uintptr_t)addr & 0xFFFFFF) + ((u32)offset))

// @recomp Skip the DMA if the animation is already in RAM. Allows mods to play custom animations. 
RECOMP_PATCH void AnimationContext_SetLoadFrame(PlayState* play, PlayerAnimationHeader* animation, s32 frame, s32 limbCount,
                                      Vec3s* frameTable) {
    AnimationEntry* task = AnimationContext_AddEntry(&play->animationCtx, ANIMATION_LINKANIMETION);

    if (task != NULL) {
        PlayerAnimationHeader* playerAnimHeader = Lib_SegmentedToVirtual(animation);
        s32 pad;

        osCreateMesgQueue(&task->data.load.msgQueue, task->data.load.msg,
                          ARRAY_COUNT(task->data.load.msg));
        
        if (IS_KSEG0(playerAnimHeader->linkAnimSegment)) {
            osSendMesg(&task->data.load.msgQueue, NULL, OS_MESG_NOBLOCK);
            Lib_MemCpy(frameTable, ((u8*)playerAnimHeader->segmentVoid) + (sizeof(Vec3s) * limbCount + sizeof(s16)) * frame,
                sizeof(Vec3s) * limbCount + sizeof(s16));
        }
        else {
            DmaMgr_SendRequestImpl(
                &task->data.load.req, frameTable,
                LINK_ANIMETION_OFFSET(playerAnimHeader->linkAnimSegment, (sizeof(Vec3s) * limbCount + sizeof(s16)) * frame),
                sizeof(Vec3s) * limbCount + sizeof(s16), 0, &task->data.load.msgQueue, NULL);
        }
    }
}
