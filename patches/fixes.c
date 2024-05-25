#include "patches.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"
#include "overlays/actors/ovl_En_Fall/z_en_fall.h"
#include "overlays/actors/ovl_Demo_Effect/z_demo_effect.h"
#include "overlays/gamestates/ovl_daytelop/z_daytelop.h"
#include "z64shrink_window.h"

#define PAGE_BG_WIDTH (PAGE_BG_COLS * PAGE_BG_QUAD_WIDTH)
#define PAGE_BG_HEIGHT (PAGE_BG_ROWS * PAGE_BG_QUAD_HEIGHT)

#define RECOMP_PAGE_ROW_HEIGHT 14
#define RECOMP_PAGE_ROW_COUNT ((PAGE_BG_HEIGHT + RECOMP_PAGE_ROW_HEIGHT - 1) / RECOMP_PAGE_ROW_HEIGHT)

extern s16* sVtxPageQuadsX[VTX_PAGE_MAX];
extern s16* sVtxPageQuadsWidth[VTX_PAGE_MAX];
extern s16* sVtxPageQuadsY[VTX_PAGE_MAX];
extern s16* sVtxPageQuadsHeight[VTX_PAGE_MAX];

s16 sVtxPageGameOverSaveQuadsY[VTX_PAGE_SAVE_QUADS] = {
    14,  // promptPageVtx[60] QUAD_PROMPT_MESSAGE
    -2,  // promptPageVtx[64] QUAD_PROMPT_CURSOR_LEFT
    -2,  // promptPageVtx[68] QUAD_PROMPT_CURSOR_RIGHT
    -18, // promptPageVtx[72] QUAD_PROMPT_CHOICE_YES
    -18, // promptPageVtx[76] QUAD_PROMPT_CHOICE_NO
};

// @recomp patched to draw as strips with bilerp compensation instead of tiles.
s16 KaleidoScope_SetPageVertices(PlayState* play, Vtx* vtx, s16 vtxPage, s16 numQuads) {
    PauseContext* pauseCtx = &play->pauseCtx;
    GameOverContext* gameOverCtx = &play->gameOverCtx;
    s16* quadsX;
    s16* quadsWidth;
    s16* quadsY;
    s16* quadsHeight;
    s32 cur_y;
    u32 row;

    cur_y = PAGE_BG_HEIGHT / 2;

    // 2 verts per row plus 2 extra verts at the start and the end.
    for (row = 0; row < RECOMP_PAGE_ROW_COUNT + 2; row++) {
        s32 next_y = MAX(cur_y - RECOMP_PAGE_ROW_HEIGHT, -PAGE_BG_HEIGHT / 2);

        vtx[4 * row + 0].v.ob[0] = -PAGE_BG_WIDTH / 2;
        vtx[4 * row + 1].v.ob[0] =  PAGE_BG_WIDTH / 2;
        vtx[4 * row + 2].v.ob[0] = -PAGE_BG_WIDTH / 2;
        vtx[4 * row + 3].v.ob[0] =  PAGE_BG_WIDTH / 2;
        
        vtx[4 * row + 0].v.ob[1] = cur_y + pauseCtx->offsetY;
        vtx[4 * row + 1].v.ob[1] = cur_y + pauseCtx->offsetY;
        vtx[4 * row + 2].v.ob[1] = next_y + pauseCtx->offsetY;
        vtx[4 * row + 3].v.ob[1] = next_y + pauseCtx->offsetY;

        vtx[4 * row + 0].v.ob[2] = vtx[4 * row + 1].v.ob[2] = vtx[4 * row + 2].v.ob[2] = vtx[4 * row + 3].v.ob[2] = 0;

        vtx[4 * row + 0].v.flag = vtx[4 * row + 1].v.flag = vtx[4 * row + 2].v.flag = vtx[4 * row + 3].v.flag = 0;

        #define PIXEL_OFFSET ((1 << 4))

        vtx[4 * row + 0].v.tc[0] = PIXEL_OFFSET;
        vtx[4 * row + 0].v.tc[1] = (1 << 5) + PIXEL_OFFSET;
        vtx[4 * row + 1].v.tc[0] = PAGE_BG_WIDTH * (1 << 5) + PIXEL_OFFSET;
        vtx[4 * row + 1].v.tc[1] = (1 << 5) + PIXEL_OFFSET;
        vtx[4 * row + 2].v.tc[0] = PIXEL_OFFSET;
        vtx[4 * row + 2].v.tc[1] = (cur_y - next_y + 1) * (1 << 5) + PIXEL_OFFSET;
        vtx[4 * row + 3].v.tc[0] = PAGE_BG_WIDTH * (1 << 5) + PIXEL_OFFSET;
        vtx[4 * row + 3].v.tc[1] = (cur_y - next_y + 1) * (1 << 5) + PIXEL_OFFSET;

        vtx[4 * row + 0].v.cn[0] = vtx[4 * row + 1].v.cn[0] = vtx[4 * row + 2].v.cn[0] = vtx[4 * row + 3].v.cn[0] = 0;
        vtx[4 * row + 0].v.cn[1] = vtx[4 * row + 1].v.cn[1] = vtx[4 * row + 2].v.cn[1] = vtx[4 * row + 3].v.cn[1] = 0;
        vtx[4 * row + 0].v.cn[2] = vtx[4 * row + 1].v.cn[2] = vtx[4 * row + 2].v.cn[2] = vtx[4 * row + 3].v.cn[2] = 0;
        vtx[4 * row + 0].v.cn[3] = vtx[4 * row + 1].v.cn[3] = vtx[4 * row + 2].v.cn[3] = vtx[4 * row + 3].v.cn[3] = pauseCtx->alpha;

        cur_y = next_y;
    }
    
    // These are overlay symbols, so their addresses need to be offset to get their actual loaded vram address.
    // TODO remove this once the recompiler is able to handle overlay symbols automatically for patch functions.
    s16** sVtxPageQuadsXRelocated =      (s16**)KaleidoManager_GetRamAddr(sVtxPageQuadsX);
    s16** sVtxPageQuadsWidthRelocated =  (s16**)KaleidoManager_GetRamAddr(sVtxPageQuadsWidth);
    s16** sVtxPageQuadsYRelocated =      (s16**)KaleidoManager_GetRamAddr(sVtxPageQuadsY);
    s16** sVtxPageQuadsHeightRelocated = (s16**)KaleidoManager_GetRamAddr(sVtxPageQuadsHeight);
    
    s16 k = 60;

    if (numQuads != 0) {
        quadsX = sVtxPageQuadsXRelocated[vtxPage];
        quadsWidth = sVtxPageQuadsWidthRelocated[vtxPage];
        quadsY = sVtxPageQuadsYRelocated[vtxPage];
        quadsHeight = sVtxPageQuadsHeightRelocated[vtxPage];
        s16 i;

        for (i = 0; i < numQuads; i++, k += 4) {
            vtx[k + 2].v.ob[0] = vtx[k + 0].v.ob[0] = quadsX[i];

            vtx[k + 1].v.ob[0] = vtx[k + 3].v.ob[0] = vtx[k + 0].v.ob[0] + quadsWidth[i];

            if (!IS_PAUSE_STATE_GAMEOVER) {
                vtx[k + 0].v.ob[1] = vtx[k + 1].v.ob[1] = quadsY[i] + pauseCtx->offsetY;
            } else if (gameOverCtx->state == GAMEOVER_INACTIVE) {
                vtx[k + 0].v.ob[1] = vtx[k + 1].v.ob[1] = quadsY[i] + pauseCtx->offsetY;
            } else {
                vtx[k + 0].v.ob[1] = vtx[k + 1].v.ob[1] = sVtxPageGameOverSaveQuadsY[i] + pauseCtx->offsetY;
            }

            vtx[k + 2].v.ob[1] = vtx[k + 3].v.ob[1] = vtx[k + 0].v.ob[1] - quadsHeight[i];

            vtx[k + 0].v.ob[2] = vtx[k + 1].v.ob[2] = vtx[k + 2].v.ob[2] = vtx[k + 3].v.ob[2] = 0;

            vtx[k + 0].v.flag = vtx[k + 1].v.flag = vtx[k + 2].v.flag = vtx[k + 3].v.flag = 0;

            vtx[k + 0].v.tc[0] = vtx[k + 0].v.tc[1] = vtx[k + 1].v.tc[1] = vtx[k + 2].v.tc[0] = 0;
            vtx[k + 1].v.tc[0] = vtx[k + 3].v.tc[0] = quadsWidth[i] << 5;
            vtx[k + 2].v.tc[1] = vtx[k + 3].v.tc[1] = quadsHeight[i] << 5;

            vtx[k + 0].v.cn[0] = vtx[k + 2].v.cn[0] = vtx[k + 0].v.cn[1] = vtx[k + 2].v.cn[1] = vtx[k + 0].v.cn[2] =
                vtx[k + 2].v.cn[2] = 255;

            vtx[k + 1].v.cn[0] = vtx[k + 3].v.cn[0] = vtx[k + 1].v.cn[1] = vtx[k + 3].v.cn[1] = vtx[k + 1].v.cn[2] =
                vtx[k + 3].v.cn[2] = 255;

            vtx[k + 0].v.cn[3] = vtx[k + 2].v.cn[3] = vtx[k + 1].v.cn[3] = vtx[k + 3].v.cn[3] = pauseCtx->alpha;
        }
    }
    return k;
}

// There's one extra row and column of padding on each side, so the size is +2 in each dimension.
typedef u8 bg_image_t[(2 + PAGE_BG_WIDTH) * (2 + PAGE_BG_HEIGHT)];

#define BG_IMAGE_COUNT 4
TexturePtr* bg_pointers[BG_IMAGE_COUNT];
bg_image_t bg_images[BG_IMAGE_COUNT] __attribute__((aligned(8)));

void assemble_image(TexturePtr* textures, bg_image_t* image_out) {
    u8* pixels_out_start = *image_out;
    // Skip a row, it'll be filled in later.
    u8* pixels_out = pixels_out_start + PAGE_BG_WIDTH + 2;
    for (u32 row = 0; row < PAGE_BG_ROWS; row++) {
        u8* texture0 = Lib_SegmentedToVirtual(textures[row + 0]);
        u8* texture1 = Lib_SegmentedToVirtual(textures[row + 5]);
        u8* texture2 = Lib_SegmentedToVirtual(textures[row + 10]);
        for (u32 tile_row = 0; tile_row < PAGE_BG_QUAD_HEIGHT; tile_row++) {
            // Write the first column, setting alpha to 0.
            *pixels_out = (*texture0) & 0xF0;
            pixels_out++;
            // Copy a row from each of the tiles into the output texture.
            Lib_MemCpy(pixels_out, texture0, PAGE_BG_QUAD_WIDTH * sizeof(u8));
            pixels_out += PAGE_BG_QUAD_WIDTH;
            texture0 += PAGE_BG_QUAD_WIDTH;
            Lib_MemCpy(pixels_out, texture1, PAGE_BG_QUAD_WIDTH * sizeof(u8));
            pixels_out += PAGE_BG_QUAD_WIDTH;
            texture1 += PAGE_BG_QUAD_WIDTH;
            Lib_MemCpy(pixels_out, texture2, PAGE_BG_QUAD_WIDTH * sizeof(u8));
            pixels_out += PAGE_BG_QUAD_WIDTH;
            texture2 += PAGE_BG_QUAD_WIDTH;
            // Write the last column, setting alpha to 0.
            *pixels_out = (*(texture2 - 1)) & 0xF0;
            pixels_out++;
        }
    }
    // Fill in the padding rows with duplicates of the first and last row but with zero alpha.
    for (u32 col = 0; col < PAGE_BG_WIDTH + 2; col++) {
        pixels_out_start[col] = pixels_out_start[col + PAGE_BG_WIDTH + 2] & 0xF0;
        pixels_out[col] = pixels_out[col - PAGE_BG_WIDTH - 2] & 0xF0;
    }
}

static bool assembled_kaleido_images = false;

extern TexturePtr sMaskPageBgTextures[];
extern TexturePtr sItemPageBgTextures[];
extern TexturePtr sMapPageBgTextures[];
extern TexturePtr sQuestPageBgTextures[];

extern void (*sKaleidoScopeUpdateFunc)(PlayState* play);
extern void (*sKaleidoScopeDrawFunc)(PlayState* play);

extern void KaleidoScope_Update(PlayState* play);
extern void KaleidoScope_Draw(PlayState* play);

void KaleidoUpdateWrapper(PlayState* play) {
    KaleidoScope_Update(play);
}

void KaleidoDrawWrapper(PlayState* play) {
    // @recomp Update the background image pointers to reflect the overlay's load address.
    bg_pointers[0] = KaleidoManager_GetRamAddr(sMaskPageBgTextures);
    bg_pointers[1] = KaleidoManager_GetRamAddr(sItemPageBgTextures);
    bg_pointers[2] = KaleidoManager_GetRamAddr(sMapPageBgTextures);
    bg_pointers[3] = KaleidoManager_GetRamAddr(sQuestPageBgTextures);

    KaleidoScope_Draw(play);

    // @recomp Check if this is the first time kaleido has been drawn. If so, assemble the background textures
    // into the full seamless image.
    if (!assembled_kaleido_images) {
        assembled_kaleido_images = true;
        // Record the old value for segments 0x08 and 0x0D, then update them with the correct values so that segmented addresses
        // can be converted in assemble_image.
        uintptr_t old_segment_08 = gSegments[0x08];
        uintptr_t old_segment_0D = gSegments[0x0D];
        gSegments[0x08] = OS_K0_TO_PHYSICAL(play->pauseCtx.iconItemSegment);
        gSegments[0x0D] = OS_K0_TO_PHYSICAL(play->pauseCtx.iconItemLangSegment);
        assemble_image(KaleidoManager_GetRamAddr(sMaskPageBgTextures), &bg_images[0]);
        assemble_image(KaleidoManager_GetRamAddr(sItemPageBgTextures), &bg_images[1]);
        assemble_image(KaleidoManager_GetRamAddr(sMapPageBgTextures), &bg_images[2]);
        assemble_image(KaleidoManager_GetRamAddr(sQuestPageBgTextures), &bg_images[3]);
        gSegments[0x08] = old_segment_08;
        gSegments[0x0D] = old_segment_0D;
    }
}

void KaleidoScopeCall_Init(PlayState* play) {
    // @recomp Set the update and draw func pointers to the wrappers instead of the actual functions.
    sKaleidoScopeUpdateFunc = KaleidoUpdateWrapper;
    sKaleidoScopeDrawFunc = KaleidoDrawWrapper;
    KaleidoSetup_Init(play);
}

// @recomp patched to fix bilerp seams.
Gfx* KaleidoScope_DrawPageSections(Gfx* gfx, Vtx* vertices, TexturePtr* textures) {
    s32 i;
    s32 j;

    bg_image_t* cur_image = NULL;

    // Check if this texture set has been assembled into a full image.
    u32 image_index;
    for (image_index = 0; image_index < BG_IMAGE_COUNT; image_index++) {
        if (bg_pointers[image_index] == textures) {
            cur_image = &bg_images[image_index];
            break;
        }
    }

    if (cur_image == NULL) {
        // No image was found.
        return gfx;
    }

    // Draw the rows.
    for (u32 bg_row = 0; bg_row < RECOMP_PAGE_ROW_COUNT; bg_row++) {
        gDPLoadTextureTile(gfx++, *cur_image,
            G_IM_FMT_IA, G_IM_SIZ_8b, // fmt, siz
            PAGE_BG_WIDTH + 2, PAGE_BG_HEIGHT + 2, // width, height
            0, (bg_row + 0) * RECOMP_PAGE_ROW_HEIGHT, // uls, ult
            PAGE_BG_WIDTH + 2, (bg_row + 1) * RECOMP_PAGE_ROW_HEIGHT + 2, // lrs, lrt
            0, // pal
            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP,
            G_TX_NOMASK, G_TX_NOMASK,
            G_TX_NOLOD, G_TX_NOLOD);
        gDPSetTileSize(gfx++, G_TX_RENDERTILE,
                0 << G_TEXTURE_IMAGE_FRAC,
                0 << G_TEXTURE_IMAGE_FRAC,
                (PAGE_BG_WIDTH + 2) <<G_TEXTURE_IMAGE_FRAC,
                (RECOMP_PAGE_ROW_HEIGHT + 2) << G_TEXTURE_IMAGE_FRAC);
        gSPVertex(gfx++, vertices + 4 * bg_row, 4, 0);
        gSP2Triangles(gfx++, 0, 3, 1, 0x0, 3, 0, 2, 0x0);
    }

    return gfx;
}

typedef void (*CutsceneHandler)(PlayState* play, CutsceneContext* csCtx);
extern CutsceneHandler sScriptedCutsceneHandlers[];
void Cutscene_SetupScripted(PlayState* play, CutsceneContext* csCtx);

int extra_vis = 0;

// @recomp Patch the giants cutscene to make certain frames take longer to mimic performance on console.
// This prevents the music from desyncing from the cutscene as it was designed around the console's frame times.
void Cutscene_UpdateScripted(PlayState* play, CutsceneContext* csCtx) {
    if ((gSaveContext.cutsceneTrigger != 0) && (play->transitionTrigger == TRANS_TRIGGER_START)) {
        gSaveContext.cutsceneTrigger = 0;
    }

    if ((gSaveContext.cutsceneTrigger != 0) && (csCtx->state == CS_STATE_IDLE)) {
        gSaveContext.save.cutsceneIndex = 0xFFFD;
        gSaveContext.cutsceneTrigger = 1;
    }

    if (gSaveContext.save.cutsceneIndex >= 0xFFF0) {
        Cutscene_SetupScripted(play, csCtx);
        sScriptedCutsceneHandlers[csCtx->state](play, csCtx);
    }

    // @recomp Insert extra VI interrupt delays on certain frames of the giants cutscene to match console
    // framerates. This prevents the music from desyncing with the cutscene.
    if (play->sceneId == SCENE_00KEIKOKU && gSaveContext.sceneLayer == 1) {
        s32 curFrame = play->csCtx.curFrame;
        s32 scriptIndex = play->csCtx.scriptIndex;
        // These regions of lag were determined by measuring framerate on console, though they pretty clearly
        // correspond to specific camera angles and effects active during the cutscene.
        if (
            (scriptIndex == 0 && curFrame >= 1123 && curFrame <= 1381) ||
            (scriptIndex != 0 && curFrame >= 4 && curFrame <= 85) ||
            (scriptIndex != 0 && curFrame >= 431 && curFrame <= 490)
        ) {
            extra_vis = 1;
        }
    }
}

// @recomp Fix a texture scroll using an incorrect tile size, which resulted in the scroll jumping during the animation.
s32 DemoEffect_OverrideLimbDrawTimewarp(PlayState* play, SkelCurve* skelCurve, s32 limbIndex, Actor* thisx) {
    s32 pad;
    DemoEffect* this = (DemoEffect*)thisx;
    u32 frames = play->gameplayFrames;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, 170, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, this->envXluColor[0], this->envXluColor[1], this->envXluColor[2], 255);

    // @recomp Fix the tile size to be 64x64 for both tiles to match the actual texture size.
    gSPSegment(POLY_XLU_DISP++, 0x08,
            Gfx_TwoTexScroll(play->state.gfxCtx,
                0, (frames * 6) % 256, 255 - ((frames * 16) % 256), 0x40, 0x40,
                1, (frames * 4) % 256, 255 - ((frames * 12) % 256), 0x40, 0x40));

    CLOSE_DISPS(play->state.gfxCtx);

    if (limbIndex == 0) {
        s16* transform = skelCurve->jointTable[0];

        transform[2] = transform[0] = 1024;
        transform[1] = 1024;
    }

    return true;
}

void* gamestate_relocate(void* addr, GameStateId id) {
    GameStateOverlay* ovl = &gGameStateOverlayTable[id];
    if ((uintptr_t)addr >= 0x80800000) {
        return (void*)((uintptr_t)addr -
                (intptr_t)((uintptr_t)ovl->vramStart - (uintptr_t)ovl->loadedRamAddr));
    }
    else {
        recomp_printf("Not an overlay address!: 0x%08X 0x%08X 0x%08X\n", (u32)addr, (u32)ovl->vramStart, (u32)ovl->loadedRamAddr);
        return addr;
    }
}

void DayTelop_Main(GameState* thisx);
void DayTelop_Destroy(GameState* thisx);
void DayTelop_Noop(DayTelopState* this);
void DayTelop_LoadGraphics(DayTelopState* this);

// @recomp Increase the length of the "Dawn of the X Day" screen to account for faster loading.
void DayTelop_Init(GameState* thisx) {
    DayTelopState* this = (DayTelopState*)thisx;

    GameState_SetFramerateDivisor(&this->state, 1);
    Matrix_Init(&this->state);
    ShrinkWindow_Destroy();
    View_Init(&this->view, this->state.gfxCtx);
    // @recomp Manual relocation, TODO remove when automated.
    this->state.main = (GameStateFunc)gamestate_relocate(DayTelop_Main, GAMESTATE_DAYTELOP);
    this->state.destroy = (GameStateFunc)gamestate_relocate(DayTelop_Destroy, GAMESTATE_DAYTELOP);
    // @recomp Add 120 extra frames (2 seconds with a frame divisor of 1) to account for faster loading.
    this->transitionCountdown = 260;
    this->fadeInState = DAYTELOP_HOURSTEXT_OFF;

    if (gSaveContext.save.day < 9) {
        if (gSaveContext.save.day == 0) {
            Sram_ClearFlagsAtDawnOfTheFirstDay();
        }
        Sram_IncrementDay();
    }

    DayTelop_Noop(this);
    DayTelop_LoadGraphics(this);
    Audio_PlaySfx(NA_SE_OC_TELOP_IMPACT);
}
