#ifndef __PATCHES_H__
#define __PATCHES_H__

// TODO fix renaming symbols in patch recompilation
#define osCreateMesgQueue osCreateMesgQueue_recomp
#define osRecvMesg osRecvMesg_recomp
#define osSendMesg osSendMesg_recomp
#define sinf __sinf_recomp
#define cosf __cosf_recomp
#define gRandFloat sRandFloat
#include "global.h"
#include "rt64_extended_gbi.h"

#define gEXMatrixGroupInterpolateOnlyTiles(cmd, push, proj) \
    gEXMatrixGroup(cmd, G_EX_ID_IGNORE, G_EX_INTERPOLATE_SIMPLE, push, proj, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR)


int recomp_printf(const char* fmt, ...);
float recomp_powf(float, float);

static inline void* actor_relocate(Actor* actor, void* addr) {
    return (void*)((uintptr_t)addr -
            (intptr_t)((uintptr_t)actor->overlayEntry->vramStart - (uintptr_t)actor->overlayEntry->loadedRamAddr));
}

typedef enum {
    /* 0 */ PICTO_BOX_STATE_OFF,         // Not using the pictograph
    /* 1 */ PICTO_BOX_STATE_LENS,        // Looking through the lens of the pictograph
    /* 2 */ PICTO_BOX_STATE_SETUP_PHOTO, // Looking at the photo currently taken
    /* 3 */ PICTO_BOX_STATE_PHOTO
} PictoBoxState;


#define INCBIN(identifier, filename)          \
    asm(".pushsection .rodata\n"              \
        "\t.local " #identifier "\n"          \
        "\t.type " #identifier ", @object\n"  \
        "\t.balign 8\n"                       \
        #identifier ":\n"                     \
        "\t.incbin \"" filename "\"\n\n"      \
                                              \
        "\t.balign 8\n"                       \
        "\t.popsection\n");                   \
    extern u8 identifier[]

void draw_dpad(PlayState* play);
void draw_dpad_icons(PlayState* play);

void View_ApplyInterpolate(View* view, s32 mask, bool reset_interpolation_state);

#endif
