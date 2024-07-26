#include "patches.h"
#include "misc_funcs.h"
#include "loadfragment.h"

void Main_ClearMemory(void* begin, void* end);
void Main_InitMemory(void);
void Main_InitScreen(void);


// @recomp Patched to load the code segment in the recomp runtime.
RECOMP_PATCH void Main_Init(void) {
    DmaRequest dmaReq;
    OSMesgQueue mq;
    OSMesg msg[1];
    size_t prevSize;

    osCreateMesgQueue(&mq, msg, ARRAY_COUNT(msg));

    prevSize = gDmaMgrDmaBuffSize;
    gDmaMgrDmaBuffSize = 0;

    // @recomp Load the code segment in the recomp runtime.
    recomp_load_overlays(SEGMENT_ROM_START(code), SEGMENT_START(code), SEGMENT_ROM_END(code) - SEGMENT_ROM_START(code));

    DmaMgr_SendRequestImpl(&dmaReq, SEGMENT_START(code), SEGMENT_ROM_START(code),
                           SEGMENT_ROM_END(code) - SEGMENT_ROM_START(code), 0, &mq, NULL);
    Main_InitScreen();
    Main_InitMemory();
    osRecvMesg(&mq, NULL, OS_MESG_BLOCK);

    gDmaMgrDmaBuffSize = prevSize;

    Main_ClearMemory(SEGMENT_BSS_START(code), SEGMENT_BSS_END(code));
}

void Overlay_Relocate(void* allocatedRamAddr, OverlayRelocationSection* ovlRelocs, uintptr_t vramStart);

// @recomp Patched to load the overlay in the recomp runtime.
RECOMP_PATCH size_t Overlay_Load(uintptr_t vromStart, uintptr_t vromEnd, void* ramStart, void* ramEnd, void* allocatedRamAddr) {
    uintptr_t vramStart = (uintptr_t)ramStart;
    uintptr_t vramEnd = (uintptr_t)ramEnd;
    s32 size = vromEnd - vromStart;
    uintptr_t end;
    OverlayRelocationSection* ovlRelocs;
    
    // @recomp Load the overlay in the recomp runtime.
    recomp_load_overlays(vromStart, allocatedRamAddr, vromEnd - vromStart);

    if (gOverlayLogSeverity >= 3) {}
    if (gOverlayLogSeverity >= 3) {}

    end = (uintptr_t)allocatedRamAddr + size;
    DmaMgr_SendRequest0(allocatedRamAddr, vromStart, size);

    ovlRelocs = (OverlayRelocationSection*)(end - ((s32*)end)[-1]);

    if (gOverlayLogSeverity >= 3) {}
    if (gOverlayLogSeverity >= 3) {}

    Overlay_Relocate(allocatedRamAddr, ovlRelocs, vramStart);

    if (ovlRelocs->bssSize != 0) {
        if (gOverlayLogSeverity >= 3) {}
        bzero((void*)end, ovlRelocs->bssSize);
    }

    size = vramEnd - vramStart;

    osWritebackDCache(allocatedRamAddr, size);
    osInvalICache(allocatedRamAddr, size);

    if (gOverlayLogSeverity >= 3) {}

    return size;
}
