#ifndef __PATCHES_H__
#define __PATCHES_H__

// TODO fix renaming symbols in patch recompilation
#define osRecvMesg osRecvMesg_recomp
#define osSendMesg osSendMesg_recomp
#include "global.h"
#include "rt64_extended_gbi.h"

int recomp_printf(const char* fmt, ...);

static inline void* actor_relocate(Actor* actor, void* addr) {
    return (void*)((uintptr_t)addr -
            (intptr_t)((uintptr_t)actor->overlayEntry->vramStart - (uintptr_t)actor->overlayEntry->loadedRamAddr));
}

#endif
