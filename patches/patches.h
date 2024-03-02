#ifndef __PATCHES_H__
#define __PATCHES_H__

// TODO fix renaming symbols in patch recompilation
#define osRecvMesg osRecvMesg_recomp
#define osSendMesg osSendMesg_recomp
#include "global.h"
#include "rt64_extended_gbi.h"

#define gEXMatrixGroupInterpolateOnlyTiles(cmd, push, proj) \
    gEXMatrixGroup(cmd, G_EX_ID_IGNORE, G_EX_INTERPOLATE_SIMPLE, push, proj, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR)


int recomp_printf(const char* fmt, ...);

static inline void* actor_relocate(Actor* actor, void* addr) {
    return (void*)((uintptr_t)addr -
            (intptr_t)((uintptr_t)actor->overlayEntry->vramStart - (uintptr_t)actor->overlayEntry->loadedRamAddr));
}

#endif
