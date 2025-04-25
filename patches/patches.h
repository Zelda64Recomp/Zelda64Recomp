#ifndef __PATCHES_H__
#define __PATCHES_H__

#define RECOMP_EXPORT __attribute__((section(".recomp_export")))
#define RECOMP_PATCH __attribute__((section(".recomp_patch")))
#define RECOMP_FORCE_PATCH __attribute__((section(".recomp_force_patch")))
#define RECOMP_DECLARE_EVENT(func) \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"") \
    __attribute__((noinline, weak, used, section(".recomp_event"))) void func {} \
    _Pragma("GCC diagnostic pop")

// TODO fix renaming symbols in patch recompilation
#define osCreateMesgQueue osCreateMesgQueue_recomp
#define osRecvMesg osRecvMesg_recomp
#define osSendMesg osSendMesg_recomp
#define osViGetCurrentFramebuffer osViGetCurrentFramebuffer_recomp
#define osFlashWriteArray osFlashWriteArray_recomp
#define osFlashWriteBuffer osFlashWriteBuffer_recomp
#define osWritebackDCache osWritebackDCache_recomp
#define osInvalICache osInvalICache_recomp
#define osGetTime osGetTime_recomp

#define osContStartReadData osContStartReadData_recomp
#define osContGetReadData osContGetReadData_recomp
#define osContStartQuery osContStartQuery_recomp
#define osContGetQuery osContGetQuery_recomp

#define sinf __sinf_recomp
#define cosf __cosf_recomp
#define bzero bzero_recomp
#define gRandFloat sRandFloat
#include "global.h"
#include "rt64_extended_gbi.h"

#ifndef gEXFillRectangle
#define gEXFillRectangle(cmd, lorigin, rorigin, ulx, uly, lrx, lry) \
    G_EX_COMMAND2(cmd, \
        PARAM(RT64_EXTENDED_OPCODE, 8, 24) | PARAM(G_EX_FILLRECT_V1, 24, 0), \
        PARAM(lorigin, 12, 0) | PARAM(rorigin, 12, 12), \
        \
        PARAM((ulx) * 4, 16, 16) | PARAM((uly) * 4, 16, 0), \
        PARAM((lrx) * 4, 16, 16) | PARAM((lry) * 4, 16, 0) \
    )
#endif

#define gEXMatrixGroupNoInterpolation(cmd, push, proj, edit) \
    gEXMatrixGroup(cmd, G_EX_ID_IGNORE, G_EX_INTERPOLATE_SIMPLE, push, proj, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_ORDER_LINEAR, edit)

#define gEXMatrixGroupInterpolateOnlyTiles(cmd, push, proj, edit) \
    gEXMatrixGroup(cmd, G_EX_ID_IGNORE, G_EX_INTERPOLATE_SIMPLE, push, proj, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, edit)

#define gEXMatrixGroupDecomposedNormal(cmd, id, push, proj, edit) \
    gEXMatrixGroupDecomposed(cmd, id, push, proj, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, edit)

#define gEXMatrixGroupDecomposedSkipRot(cmd, id, push, proj, edit) \
    gEXMatrixGroupDecomposed(cmd, id, push, proj, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, edit)

#define gEXMatrixGroupDecomposedSkipPosRot(cmd, id, push, proj, edit) \
    gEXMatrixGroupDecomposed(cmd, id, push, proj, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, edit)

#define gEXMatrixGroupDecomposedSkipAll(cmd, id, push, proj, edit) \
    gEXMatrixGroupDecomposed(cmd, id, push, proj, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, edit)

#define gEXMatrixGroupDecomposedVerts(cmd, id, push, proj, edit) \
    gEXMatrixGroupDecomposed(cmd, id, push, proj, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, edit)

#define gEXMatrixGroupDecomposedVertsOrderAuto(cmd, id, push, proj, edit) \
    gEXMatrixGroupDecomposed(cmd, id, push, proj, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_AUTO, edit)


int recomp_printf(const char* fmt, ...);
float recomp_powf(float, float);

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

void View_ApplyInterpolate(View* view, s32 mask, bool reset_interpolation_state);

void set_camera_skipped(bool skipped);
void clear_camera_skipped();
void edit_billboard_groups(PlayState* play);
bool camera_was_skipped();
void room_load_hook(PlayState* play, Room* room);
void draw_autosave_icon(PlayState* play);

void recomp_crash(const char* err);

#endif
