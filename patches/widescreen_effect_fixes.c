#include "patches.h"
#include "graphics.h"

extern TransitionOverlay gTransitionOverlayTable[];
extern Gfx sTransWipe3DL[];

#define THIS ((TransitionWipe3*)thisx)
// @recomp patched to scale the transition based on aspect ratio
void TransitionWipe3_Draw(void* thisx, Gfx** gfxP) {
    Gfx* gfx = *gfxP;
    Mtx* modelView = &THIS->modelView[THIS->frame];
    f32 scale = 14.8f;
    Gfx* texScroll;
    
    // @recomp Modify the scale based on the aspect ratio to make sure the transition circle covers the whole screen
    scale *= (recomp_get_aspect_ratio() / (4.0f / 3.0f));

    THIS->frame ^= 1;
    gDPPipeSync(gfx++);
    texScroll = Gfx_BranchTexScroll(&gfx, THIS->scrollX, THIS->scrollY, 16, 64);
    gSPSegment(gfx++, 0x09, texScroll);
    gSPSegment(gfx++, 0x08, THIS->curTexture);
    gDPSetColor(gfx++, G_SETPRIMCOLOR, THIS->color.rgba);
    gDPSetColor(gfx++, G_SETENVCOLOR, THIS->color.rgba);
    gSPMatrix(gfx++, &THIS->projection, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    gSPPerspNormalize(gfx++, THIS->normal);
    gSPMatrix(gfx++, &THIS->lookAt, G_MTX_NOPUSH | G_MTX_MUL | G_MTX_PROJECTION);

    if (scale != 1.0f) {
        guScale(modelView, scale, scale, 1.0f);
        gSPMatrix(gfx++, modelView, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    }
    // sTransWipe3DL is an overlay symbol, so its addresses need to be offset to get the actual loaded vram address.
    // TODO remove this once the recompiler is able to handle overlay symbols automatically for patch functions.
    ptrdiff_t reloc_offset;
    TransitionOverlay* overlay_entry = &gTransitionOverlayTable[FBDEMO_WIPE3];
    reloc_offset = (uintptr_t)Lib_PhysicalToVirtual(overlay_entry->loadInfo.addr) - (uintptr_t)overlay_entry->vramStart;
    gSPDisplayList(gfx++, (Gfx*)((u8*)sTransWipe3DL + reloc_offset));
    gDPPipeSync(gfx++);
    *gfxP = gfx;
}

#undef THIS