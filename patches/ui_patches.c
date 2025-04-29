#include "patches.h"
#include "transform_ids.h"
#include "buffers.h"
#include "sys_cfb.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"

// This moves elements towards the screen edges when increased
s32 margin_reduction = 8;

extern s32 gFramerateDivisor;

// 10 times bigger than the game's normal buffers.
typedef struct {
    GfxMasterList master;
    Gfx polyXluBuffer[0x8000];
    Gfx overlayBuffer[0x4000];
    Gfx workBuffer[0x400];
    Gfx debugBuffer[0x400];
    Gfx polyOpaBuffer[0x33800];
} BiggerGfxPool;

BiggerGfxPool gBiggerGfxPools[2];

// @recomp Use the bigger gfx pools and enable RT64 extended GBI mode.
RECOMP_PATCH void Graph_SetNextGfxPool(GraphicsContext* gfxCtx) {
    GfxPool* pool = &gGfxPools[gfxCtx->gfxPoolIdx % 2];
    BiggerGfxPool* bigger_pool = &gBiggerGfxPools[gfxCtx->gfxPoolIdx % 2];

    gGfxMasterDL = &pool->master;
    gSegments[0x0E] = (uintptr_t)gGfxMasterDL;

    pool->headMagic = GFXPOOL_HEAD_MAGIC;
    pool->tailMagic = GFXPOOL_TAIL_MAGIC;

    Graph_InitTHGA(&gfxCtx->polyOpa, bigger_pool->polyOpaBuffer, sizeof(bigger_pool->polyOpaBuffer));
    Graph_InitTHGA(&gfxCtx->polyXlu, bigger_pool->polyXluBuffer, sizeof(bigger_pool->polyXluBuffer));
    Graph_InitTHGA(&gfxCtx->overlay, bigger_pool->overlayBuffer, sizeof(bigger_pool->overlayBuffer));
    Graph_InitTHGA(&gfxCtx->work, bigger_pool->workBuffer, sizeof(bigger_pool->workBuffer));
    Graph_InitTHGA(&gfxCtx->debug, bigger_pool->debugBuffer, sizeof(bigger_pool->debugBuffer));

    gfxCtx->polyOpaBuffer = bigger_pool->polyOpaBuffer;
    gfxCtx->polyXluBuffer = bigger_pool->polyXluBuffer;
    gfxCtx->overlayBuffer = bigger_pool->overlayBuffer;
    gfxCtx->workBuffer = bigger_pool->workBuffer;
    gfxCtx->debugBuffer = bigger_pool->debugBuffer;

    gfxCtx->curFrameBuffer = SysCfb_GetFramebuffer(gfxCtx->framebufferIndex % 2);
    gSegments[0x0F] = (uintptr_t)gfxCtx->curFrameBuffer;

    gfxCtx->zbuffer = SysCfb_GetZBuffer();

    gSPBranchList(&gGfxMasterDL->disps[0], bigger_pool->polyOpaBuffer);
    gSPBranchList(&gGfxMasterDL->disps[1], bigger_pool->polyXluBuffer);
    gSPBranchList(&gGfxMasterDL->disps[2], bigger_pool->overlayBuffer);
    gSPBranchList(&gGfxMasterDL->disps[3], bigger_pool->workBuffer);
    gSPEndDisplayList(&gGfxMasterDL->disps[4]);
    gSPBranchList(&gGfxMasterDL->debugDisp[0], bigger_pool->debugBuffer);

    // @recomp Enable RT64 extended GBI mode and extended rdram.
    OPEN_DISPS(gfxCtx);
    gEXEnable(POLY_OPA_DISP++);
    gEXSetRDRAMExtended(POLY_OPA_DISP++, 1);
    CLOSE_DISPS(gfxCtx);
}

void recomp_crash(const char* err) {
    recomp_printf("%s\n", err);
    // TODO open a message box instead of a hard crash
    *(volatile int*)0 = 0;
}

extern volatile OSTime gRSPGfxTimeTotal;
extern volatile OSTime gRSPGfxTimeAcc;
extern volatile OSTime gRSPAudioTimeTotal;
extern volatile OSTime gRSPAudioTimeAcc;
extern volatile OSTime gRDPTimeTotal;
extern volatile OSTime gRDPTimeAcc;
extern OSTime sGraphPrevUpdateEndTime;
extern volatile OSTime gGraphUpdatePeriod;


extern int extra_vis;

// @recomp Modified to report errors instead of skipping frames.
/**
 *  Run the game state logic, then finalize the gfx buffer
 *  and run the graphics task for this frame.
 */
RECOMP_PATCH void Graph_ExecuteAndDraw(GraphicsContext* gfxCtx, GameState* gameState) {
    u32 problem;

    gameState->unk_A3 = 0;
    Graph_SetNextGfxPool(gfxCtx);

    GameState_Update(gameState);

    OPEN_DISPS(gfxCtx);
    
    // @recomp Send the current framerate to RT64, including any extra VI interrupt periods. 
    gEXSetRefreshRate(POLY_OPA_DISP++, 60 / (gameState->framerateDivisor + extra_vis));

    // @recomp Edit billboard groups to skip interpolation if the camera also skipped.
    if (gameState->destroy == Play_Destroy) {
        PlayState* play = (PlayState*)gameState;
        edit_billboard_groups(play);
    }

    // @recomp Clear the camera skip state.
    clear_camera_skipped();

    gSPEndDisplayList(WORK_DISP++);
    gSPEndDisplayList(POLY_OPA_DISP++);
    gSPEndDisplayList(POLY_XLU_DISP++);
    gSPEndDisplayList(OVERLAY_DISP++);
    gSPEndDisplayList(DEBUG_DISP++);

    CLOSE_DISPS(gfxCtx);

    {
        Gfx* gfx = gGfxMasterDL->taskStart;

        gSPSegment(gfx++, 0x0E, gGfxMasterDL);
        gSPDisplayList(gfx++, &D_0E000000.disps[3]);
        gSPDisplayList(gfx++, &D_0E000000.disps[0]);
        gSPDisplayList(gfx++, &D_0E000000.disps[1]);
        gSPDisplayList(gfx++, &D_0E000000.disps[2]);
        gSPDisplayList(gfx++, &D_0E000000.debugDisp[0]);

        gDPPipeSync(gfx++);
        gDPFullSync(gfx++);
        gSPEndDisplayList(gfx++);
    }

    problem = false;

    // @recomp Patch all error conditions to print to console and crash the application.
    {
        GfxPool* pool = &gGfxPools[gfxCtx->gfxPoolIdx % 2];

        if (pool->headMagic != GFXPOOL_HEAD_MAGIC) {
            recomp_crash("GfxPool headMagic integrity check failed!");
        }
        if (pool->tailMagic != GFXPOOL_TAIL_MAGIC) {
            recomp_crash("GfxPool tailMagic integrity check failed!");
        }
    }

    if (THGA_IsCrash(&gfxCtx->polyOpa)) {
        recomp_crash("gfxCtx->polyOpa overflow!");
    }
    if (THGA_IsCrash(&gfxCtx->polyXlu)) {
        recomp_crash("gfxCtx->polyXlu overflow!");
    }
    if (THGA_IsCrash(&gfxCtx->overlay)) {
        recomp_crash("gfxCtx->overlay overflow!");
    }
    if (THGA_IsCrash(&gfxCtx->work)) {
        recomp_crash("gfxCtx->work overflow!");
    }
    if (THGA_IsCrash(&gfxCtx->debug)) {
        recomp_crash("gfxCtx->debug overflow!");
    }

    if (!problem) {
        // @recomp Temporarily adjust the framerate divisor to include any extra VI interrupt periods.
        u8 old_divisor = gameState->framerateDivisor;
        gameState->framerateDivisor += extra_vis;
        Graph_TaskSet00(gfxCtx, gameState);
        // @recomp Restore the old framerate divisor.
        gameState->framerateDivisor = old_divisor;
        gfxCtx->gfxPoolIdx++;
        gfxCtx->framebufferIndex++;
    }
    // @recomp Clear any extra VI interrupt periods.
    extra_vis = 0;

    {
        OSTime time = osGetTime();

        gRSPGfxTimeTotal = gRSPGfxTimeAcc;
        gRSPAudioTimeTotal = gRSPAudioTimeAcc;
        gRDPTimeTotal = gRDPTimeAcc;
        gRSPGfxTimeAcc = 0;
        gRSPAudioTimeAcc = 0;
        gRDPTimeAcc = 0;

        if (sGraphPrevUpdateEndTime != 0) {
            gGraphUpdatePeriod = time - sGraphPrevUpdateEndTime;
        }
        sGraphPrevUpdateEndTime = time;
    }
}

extern s16 sPictoState;

// Modify interface drawing (rupees, key counter, etc.)
extern TexturePtr sStoryTextures[];
extern TexturePtr sStoryTLUTs[];

extern Color_RGB16 sRupeeCounterIconPrimColors[];
extern Color_RGB16 sRupeeCounterIconEnvColors[];
extern u8 gRupeeCounterIconTex[];
extern s16 sRupeeDigitsFirst[];
extern s16 sRupeeDigitsCount[];

extern u8 gSmallKeyCounterIconTex[];
extern u8 gCounterDigit0Tex[];
extern u8 gGoldSkulltulaCounterIconTex[];

extern Color_RGB16 sMinigameCountdownPrimColors[];
extern TexturePtr sMinigameCountdownTextures[];
extern s16 sMinigameCountdownTexWidths[];

extern u8 gPictoBoxFocusBorderTex[];
extern u8 gPictoBoxFocusIconTex[];
extern u8 gPictoBoxFocusTextTex[];

static Gfx sScreenFillSetupDL[] = {
    gsDPPipeSync(),
    gsSPClearGeometryMode(G_ZBUFFER | G_SHADE | G_CULL_BOTH | G_FOG | G_LIGHTING | G_TEXTURE_GEN |
                          G_TEXTURE_GEN_LINEAR | G_LOD | G_SHADING_SMOOTH),
    gsDPSetOtherMode(G_AD_DISABLE | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_BILERP | G_TT_NONE | G_TL_TILE |
                         G_TD_CLAMP | G_TP_NONE | G_CYC_1CYCLE | G_PM_1PRIMITIVE,
                     G_AC_NONE | G_ZS_PIXEL | G_RM_CLD_SURF | G_RM_CLD_SURF2),
    gsDPSetCombineMode(G_CC_PRIMITIVE, G_CC_PRIMITIVE),
    gsSPEndDisplayList(),
};

void Interface_DrawAButton(PlayState* play);
void Interface_DrawBButtonIcons(PlayState* play);
void Interface_DrawCButtonIcons(PlayState* play);
void Interface_DrawClock(PlayState* play);
void Interface_DrawItemButtons(PlayState* play);
void Interface_DrawMinigameIcons(PlayState* play);
void Interface_DrawPauseMenuEquippingIcons(PlayState* play);
void Interface_DrawPerfectLetters(PlayState* play);
void Interface_DrawTimers(PlayState* play);
void Interface_SetOrthoView(InterfaceContext* interfaceCtx);
void Interface_SetVertices(PlayState* play);
void Magic_DrawMeter(PlayState* play);

RECOMP_PATCH void Interface_Draw(PlayState* play) {
    s32 pad;
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    Player* player = GET_PLAYER(play);
    Gfx* gfx;
    s16 sp2CE;
    s16 sp2CC;
    s16 sp2CA;
    s16 sp2C8;
    PauseContext* pauseCtx = &play->pauseCtx;
    f32 minigameCountdownScale;
    s16 counterDigits[4];
    s16 magicAlpha;

    OPEN_DISPS(play->state.gfxCtx);

    gSPSegment(OVERLAY_DISP++, 0x02, interfaceCtx->parameterSegment);
    gSPSegment(OVERLAY_DISP++, 0x09, interfaceCtx->doActionSegment);
    gSPSegment(OVERLAY_DISP++, 0x08, interfaceCtx->iconItemSegment);
    gSPSegment(OVERLAY_DISP++, 0x0B, interfaceCtx->mapSegment);

    if (pauseCtx->debugEditor == DEBUG_EDITOR_NONE) {
        Interface_SetVertices(play);
        // @recomp Adjust any scissors to cover the whole screen
        gEXSetScissorAlign(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT, 0, -margin_reduction, -SCREEN_WIDTH, margin_reduction, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        Interface_SetOrthoView(interfaceCtx);

        // Draw Grandma's Story
        if (interfaceCtx->storyDmaStatus == STORY_DMA_DONE) {
            gSPSegment(OVERLAY_DISP++, 0x07, interfaceCtx->storySegment);
            Gfx_SetupDL39_Opa(play->state.gfxCtx);

            gDPSetTextureFilter(POLY_OPA_DISP++, G_TF_POINT);
            gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, pauseCtx->alpha);

            // Load in Grandma's Story
            gSPLoadUcodeL(OVERLAY_DISP++, gspS2DEX2_fifo);
            gfx = OVERLAY_DISP;
            Prerender_DrawBackground2D(&gfx, sStoryTextures[interfaceCtx->storyType],
                                       sStoryTLUTs[interfaceCtx->storyType], SCREEN_WIDTH, SCREEN_HEIGHT, 2, 1, 0x8000,
                                       0x100, 0.0f, 0.0f, 1.0f, 1.0f, 0);
            OVERLAY_DISP = gfx;
            gSPLoadUcode(OVERLAY_DISP++, SysUcode_GetUCode(), SysUcode_GetUCodeData());

            gDPPipeSync(OVERLAY_DISP++);

            // Fill the screen with a black rectangle
            gDPSetRenderMode(OVERLAY_DISP++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
            gDPSetCombineMode(OVERLAY_DISP++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
            gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 0, 0, R_STORY_FILL_SCREEN_ALPHA);
            gDPFillRectangle(OVERLAY_DISP++, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        }

        // @recomp Left align and shift left/up for life meter
        gEXSetViewportAlign(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, -margin_reduction * 4, -margin_reduction * 4);
        gEXSetRectAlign(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, -margin_reduction * 4, -margin_reduction * 4, -margin_reduction * 4, -margin_reduction * 4);
        Interface_SetOrthoView(interfaceCtx);

        LifeMeter_Draw(play);

        Gfx_SetupDL39_Overlay(play->state.gfxCtx);

        // @recomp Left align and shift left/down for key count, skulltula count, and rupee count
        gEXSetViewportAlign(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, -margin_reduction * 4, margin_reduction * 4);
        gEXSetRectAlign(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, -margin_reduction * 4, margin_reduction * 4, -margin_reduction * 4, margin_reduction * 4);

        // Draw Rupee Icon
        gDPSetPrimColor(OVERLAY_DISP++, 0, 0, sRupeeCounterIconPrimColors[CUR_UPG_VALUE(UPG_WALLET)].r,
                        sRupeeCounterIconPrimColors[CUR_UPG_VALUE(UPG_WALLET)].g,
                        sRupeeCounterIconPrimColors[CUR_UPG_VALUE(UPG_WALLET)].b, interfaceCtx->magicAlpha);
        gDPSetEnvColor(OVERLAY_DISP++, sRupeeCounterIconEnvColors[CUR_UPG_VALUE(UPG_WALLET)].r,
                       sRupeeCounterIconEnvColors[CUR_UPG_VALUE(UPG_WALLET)].g,
                       sRupeeCounterIconEnvColors[CUR_UPG_VALUE(UPG_WALLET)].b, 255);
        OVERLAY_DISP =
            Gfx_DrawTexRectIA8(OVERLAY_DISP, gRupeeCounterIconTex, 16, 16, 26, 206, 16, 16, 1 << 10, 1 << 10);

        switch (play->sceneId) {
            case SCENE_INISIE_N:
            case SCENE_INISIE_R:
            case SCENE_MITURIN:
            case SCENE_HAKUGIN:
            case SCENE_SEA:
                if (DUNGEON_KEY_COUNT(gSaveContext.mapIndex) >= 0) {
                    // Small Key Icon
                    gDPPipeSync(OVERLAY_DISP++);
                    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 200, 230, 255, interfaceCtx->magicAlpha);
                    gDPSetEnvColor(OVERLAY_DISP++, 0, 0, 20, 255);
                    OVERLAY_DISP = Gfx_DrawTexRectIA8(OVERLAY_DISP, gSmallKeyCounterIconTex, 16, 16, 26, 190, 16, 16,
                                                      1 << 10, 1 << 10);

                    // Small Key Counter
                    gDPPipeSync(OVERLAY_DISP++);
                    gDPSetCombineLERP(OVERLAY_DISP++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE,
                                      TEXEL0, 0, PRIMITIVE, 0);

                    counterDigits[2] = 0;
                    counterDigits[3] = DUNGEON_KEY_COUNT(gSaveContext.mapIndex);

                    while (counterDigits[3] >= 10) {
                        counterDigits[2]++;
                        counterDigits[3] -= 10;
                    }

                    sp2CA = 42;

                    if (counterDigits[2] != 0) {
                        gDPPipeSync(OVERLAY_DISP++);
                        gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 0, 0, interfaceCtx->magicAlpha);

                        OVERLAY_DISP =
                            Gfx_DrawTexRectI8(OVERLAY_DISP, (u8*)gCounterDigit0Tex + (8 * 16 * counterDigits[2]), 8, 16,
                                              43, 191, 8, 16, 1 << 10, 1 << 10);

                        gDPPipeSync(OVERLAY_DISP++);
                        gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, interfaceCtx->magicAlpha);
                        gSPTextureRectangle(OVERLAY_DISP++, 168, 760, 200, 824, G_TX_RENDERTILE, 0, 0, 1 << 10,
                                            1 << 10);

                        sp2CA += 8;
                    }

                    gDPPipeSync(OVERLAY_DISP++);
                    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 0, 0, interfaceCtx->magicAlpha);

                    OVERLAY_DISP = Gfx_DrawTexRectI8(OVERLAY_DISP, (u8*)gCounterDigit0Tex + (8 * 16 * counterDigits[3]),
                                                     8, 16, sp2CA + 1, 191, 8, 16, 1 << 10, 1 << 10);

                    gDPPipeSync(OVERLAY_DISP++);
                    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, interfaceCtx->magicAlpha);
                    gSPTextureRectangle(OVERLAY_DISP++, sp2CA * 4, 760, (sp2CA * 4) + 0x20, 824, G_TX_RENDERTILE, 0, 0,
                                        1 << 10, 1 << 10);
                }
                break;

            case SCENE_KINSTA1:
            case SCENE_KINDAN2:
                // Gold Skulltula Icon
                gDPPipeSync(OVERLAY_DISP++);
                gDPSetCombineMode(OVERLAY_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
                gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, interfaceCtx->magicAlpha);
                gDPSetEnvColor(OVERLAY_DISP++, 0, 0, 0, 255);
                gDPLoadTextureBlock(OVERLAY_DISP++, gGoldSkulltulaCounterIconTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 24, 24,
                                    0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK,
                                    G_TX_NOLOD, G_TX_NOLOD);
                gSPTextureRectangle(OVERLAY_DISP++, 80, 748, 176, 820, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);

                // Gold Skulluta Counter
                gDPPipeSync(OVERLAY_DISP++);
                gDPSetCombineLERP(OVERLAY_DISP++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE,
                                  TEXEL0, 0, PRIMITIVE, 0);

                counterDigits[2] = 0;
                counterDigits[3] = Inventory_GetSkullTokenCount(play->sceneId);

                while (counterDigits[3] >= 10) {
                    counterDigits[2]++;
                    counterDigits[3] -= 10;
                }

                sp2CA = 42;

                if (counterDigits[2] != 0) {
                    gDPPipeSync(OVERLAY_DISP++);
                    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 0, 0, interfaceCtx->magicAlpha);

                    OVERLAY_DISP = Gfx_DrawTexRectI8(OVERLAY_DISP, (u8*)gCounterDigit0Tex + (8 * 16 * counterDigits[2]),
                                                     8, 16, 43, 191, 8, 16, 1 << 10, 1 << 10);

                    gDPPipeSync(OVERLAY_DISP++);
                    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, interfaceCtx->magicAlpha);
                    gSPTextureRectangle(OVERLAY_DISP++, 168, 760, 200, 824, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);

                    sp2CA += 8;
                }

                gDPPipeSync(OVERLAY_DISP++);
                gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 0, 0, interfaceCtx->magicAlpha);

                OVERLAY_DISP = Gfx_DrawTexRectI8(OVERLAY_DISP, (u8*)gCounterDigit0Tex + (8 * 16 * counterDigits[3]), 8,
                                                 16, sp2CA + 1, 191, 8, 16, 1 << 10, 1 << 10);

                gDPPipeSync(OVERLAY_DISP++);
                gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, interfaceCtx->magicAlpha);
                gSPTextureRectangle(OVERLAY_DISP++, sp2CA * 4, 760, (sp2CA * 4) + 0x20, 824, G_TX_RENDERTILE, 0, 0,
                                    1 << 10, 1 << 10);
                break;

            default:
                break;
        }

        // Rupee Counter
        gDPPipeSync(OVERLAY_DISP++);
        gDPSetCombineLERP(OVERLAY_DISP++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0,
                          PRIMITIVE, 0);

        counterDigits[0] = counterDigits[1] = 0;
        counterDigits[2] = gSaveContext.save.saveInfo.playerData.rupees;

        if ((counterDigits[2] > 9999) || (counterDigits[2] < 0)) {
            counterDigits[2] &= 0xDDD;
        }

        while (counterDigits[2] >= 100) {
            counterDigits[0]++;
            counterDigits[2] -= 100;
        }

        while (counterDigits[2] >= 10) {
            counterDigits[1]++;
            counterDigits[2] -= 10;
        }

        sp2CC = sRupeeDigitsFirst[CUR_UPG_VALUE(UPG_WALLET)];
        sp2C8 = sRupeeDigitsCount[CUR_UPG_VALUE(UPG_WALLET)];

        magicAlpha = interfaceCtx->magicAlpha;
        if (magicAlpha > 180) {
            magicAlpha = 180;
        }

        for (sp2CE = 0, sp2CA = 42; sp2CE < sp2C8; sp2CE++, sp2CC++, sp2CA += 8) {
            gDPPipeSync(OVERLAY_DISP++);
            gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 0, 0, magicAlpha);

            OVERLAY_DISP = Gfx_DrawTexRectI8(OVERLAY_DISP, (u8*)gCounterDigit0Tex + (8 * 16 * counterDigits[sp2CC]), 8,
                                             16, sp2CA + 1, 207, 8, 16, 1 << 10, 1 << 10);

            gDPPipeSync(OVERLAY_DISP++);

            if (gSaveContext.save.saveInfo.playerData.rupees == CUR_CAPACITY(UPG_WALLET)) {
                gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 120, 255, 0, interfaceCtx->magicAlpha);
            } else if (gSaveContext.save.saveInfo.playerData.rupees != 0) {
                gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, interfaceCtx->magicAlpha);
            } else {
                gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 100, 100, 100, interfaceCtx->magicAlpha);
            }

            gSPTextureRectangle(OVERLAY_DISP++, sp2CA * 4, 824, (sp2CA * 4) + 0x20, 888, G_TX_RENDERTILE, 0, 0, 1 << 10,
                                1 << 10);
        }

        // @recomp Left align and shift left/up for magic meter
        gEXSetViewportAlign(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, -margin_reduction * 4, -margin_reduction * 4);
        gEXSetRectAlign(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, -margin_reduction * 4, -margin_reduction * 4, -margin_reduction * 4, -margin_reduction * 4);

        Magic_DrawMeter(play);
        
        // @recomp Draw the autosave icon if the game is unpaused.
        if (pauseCtx->state != PAUSE_STATE_MAIN) {
            draw_autosave_icon(play);
        }

        // @recomp Right align and shift right/down for minimap
        gEXSetRectAlign(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT,
            -(SCREEN_WIDTH - margin_reduction) * 4, margin_reduction * 4,
            -(SCREEN_WIDTH - margin_reduction) * 4, margin_reduction * 4);
        gEXSetViewportAlign(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, -(SCREEN_WIDTH - margin_reduction) * 4, margin_reduction * 4);
        Interface_SetOrthoView(interfaceCtx);

        Minimap_Draw(play);

        // @recomp Reset viewport alignment for drawing the target reticle
        gEXSetRectAlign(OVERLAY_DISP++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, 0, 0, 0);
        gEXSetViewportAlign(OVERLAY_DISP++, G_EX_ORIGIN_NONE, 0, 0);
        Interface_SetOrthoView(interfaceCtx);

        if ((R_PAUSE_BG_PRERENDER_STATE != 2) && (R_PAUSE_BG_PRERENDER_STATE != 3)) {
            Target_Draw(&play->actorCtx.targetCtx, play);
        }

        // @recomp Right align and shift right/up for equipped items and buttons
        gEXSetRectAlign(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT,
            -(SCREEN_WIDTH - margin_reduction) * 4, -margin_reduction * 4,
            -(SCREEN_WIDTH - margin_reduction) * 4, -margin_reduction * 4);
        gEXSetViewportAlign(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, -(SCREEN_WIDTH - margin_reduction) * 4, -margin_reduction * 4);
        gEXSetScissorAlign(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, -(SCREEN_WIDTH - margin_reduction), -margin_reduction, -(SCREEN_WIDTH - margin_reduction), -margin_reduction, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        Interface_SetOrthoView(interfaceCtx);

        Gfx_SetupDL39_Overlay(play->state.gfxCtx);

        Interface_DrawItemButtons(play);

        if (player->transformation == GET_PLAYER_FORM) {
            Interface_DrawBButtonIcons(play);
        }
        Interface_DrawCButtonIcons(play);

        Interface_DrawAButton(play);

        // @recomp Adjust any scissors to cover the whole screen
        gEXSetScissorAlign(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT, 0, -margin_reduction, -SCREEN_WIDTH, margin_reduction, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

        // @recomp Move the item being equipped from the center of the screen to the right edge as the timer counts down
        if ((pauseCtx->state == PAUSE_STATE_MAIN) && ((pauseCtx->mainState == PAUSE_MAIN_STATE_EQUIP_ITEM) ||
                                                    (pauseCtx->mainState == PAUSE_MAIN_STATE_EQUIP_MASK))) {
            extern s16 sEquipAnimTimer;
            extern s16 sMaskEquipAnimTimer;
            extern s16 sEquipState;
            extern s16 sMaskEquipState;
            s16 equip_timer =      sEquipAnimTimer;
            s16 mask_equip_timer = sMaskEquipAnimTimer;
            s16 equip_state =      sEquipState;
            s16 mask_equip_state = sMaskEquipState;

            s16 timer = MIN(equip_timer, mask_equip_timer);
            s32 max_timer = 10;

            // Prevent the timer from being used to calculate the origin when an arrow effect is taking place.
            if (equip_timer < 10 && equip_state != EQUIP_STATE_MOVE_TO_C_BTN) {
                timer = 10;
            }

            // Adjust the max timer value if a magic arrow is being equipped.
            if ((pauseCtx->equipTargetItem == ITEM_BOW_FIRE) ||
                (pauseCtx->equipTargetItem == ITEM_BOW_ICE) ||
                (pauseCtx->equipTargetItem == ITEM_BOW_LIGHT)) {
                max_timer = 6;
            }

            s32 origin = (G_EX_ORIGIN_CENTER - G_EX_ORIGIN_RIGHT) * (timer - 1) / (max_timer - 1) + G_EX_ORIGIN_RIGHT;
            s32 offset = (SCREEN_WIDTH / 2) * (timer - 1) / (max_timer - 1) - SCREEN_WIDTH;
            gEXSetRectAlign(OVERLAY_DISP++, origin, origin, offset * 4, 0, offset * 4, 0);
            gEXSetViewportAlign(OVERLAY_DISP++, origin, offset * 4, 0);
        }
        Interface_DrawPauseMenuEquippingIcons(play);

        // Draw either the minigame countdown or the three-day clock
        if ((play->pauseCtx.state == PAUSE_STATE_OFF) && (play->pauseCtx.debugEditor == DEBUG_EDITOR_NONE)) {
            if ((interfaceCtx->minigameState != MINIGAME_STATE_NONE) &&
                (interfaceCtx->minigameState < MINIGAME_STATE_NO_COUNTDOWN_SETUP)) {
                // Minigame Countdown
                if (((u32)interfaceCtx->minigameState % 2) == 0) {
                    // @recomp Restore normal alignment and reset shift for minigame countdown
                    gEXSetRectAlign(OVERLAY_DISP++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, 0, 0, 0);
                    gEXSetViewportAlign(OVERLAY_DISP++, G_EX_ORIGIN_NONE, 0, 0);
                    Interface_SetOrthoView(interfaceCtx);

                    sp2CE = (interfaceCtx->minigameState >> 1) - 1;
                    minigameCountdownScale = interfaceCtx->minigameCountdownScale / 100.0f;

                    if (sp2CE == 3) {
                        interfaceCtx->actionVtx[40 + 0].v.ob[0] = interfaceCtx->actionVtx[40 + 2].v.ob[0] = -20;
                        interfaceCtx->actionVtx[40 + 1].v.ob[0] = interfaceCtx->actionVtx[40 + 3].v.ob[0] =
                            interfaceCtx->actionVtx[40 + 0].v.ob[0] + 40;
                        interfaceCtx->actionVtx[40 + 1].v.tc[0] = interfaceCtx->actionVtx[40 + 3].v.tc[0] = 40 << 5;
                    }

                    interfaceCtx->actionVtx[40 + 2].v.tc[1] = interfaceCtx->actionVtx[40 + 3].v.tc[1] = 32 << 5;

                    Gfx_SetupDL42_Overlay(play->state.gfxCtx);

                    gDPSetCombineMode(OVERLAY_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
                    gDPSetAlphaCompare(OVERLAY_DISP++, G_AC_THRESHOLD);
                    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, sMinigameCountdownPrimColors[sp2CE].r,
                                    sMinigameCountdownPrimColors[sp2CE].g, sMinigameCountdownPrimColors[sp2CE].b,
                                    interfaceCtx->minigameCountdownAlpha);

                    Matrix_Translate(0.0f, -40.0f, 0.0f, MTXMODE_NEW);
                    Matrix_Scale(minigameCountdownScale, minigameCountdownScale, 0.0f, MTXMODE_APPLY);

                    gSPMatrix(OVERLAY_DISP++, Matrix_NewMtx(play->state.gfxCtx),
                              G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                    gSPVertex(OVERLAY_DISP++, &interfaceCtx->actionVtx[40], 4, 0);

                    OVERLAY_DISP = Gfx_DrawTexQuadIA8(OVERLAY_DISP, sMinigameCountdownTextures[sp2CE],
                                                      sMinigameCountdownTexWidths[sp2CE], 32, 0);
                }
            } else {
                // @recomp Use normal alignment and shift down for clock
                gEXSetRectAlign(OVERLAY_DISP++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, margin_reduction * 4, 0, margin_reduction * 4);
                gEXSetViewportAlign(OVERLAY_DISP++, G_EX_ORIGIN_NONE, 0, margin_reduction * 4);
                Interface_SetOrthoView(interfaceCtx);

                Interface_DrawClock(play);
            }
        }
        
        // @recomp Restore normal alignment and reset shift for minigame "Perfect" text
        gEXSetRectAlign(OVERLAY_DISP++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, 0, 0, 0);
        gEXSetViewportAlign(OVERLAY_DISP++, G_EX_ORIGIN_NONE, 0, 0);
        Interface_SetOrthoView(interfaceCtx);

        // Draw the letters of minigame perfect
        if (interfaceCtx->perfectLettersOn) {
            Interface_DrawPerfectLetters(play);
        }

        // @recomp Align left and shift up for minigame icons (carrot icons use an extended gbi texrect to ignore this state).
        gEXSetRectAlign(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, 0, -margin_reduction * 4, 0, -margin_reduction * 4);
        gEXSetViewportAlign(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, 0, -margin_reduction * 4);
        Interface_SetOrthoView(interfaceCtx);

        Interface_DrawMinigameIcons(play);

        // @recomp If the moon crash timer is running, center align timers and shift down.
        bool moon_crash_timer_running = gSaveContext.timerStates[TIMER_ID_MOON_CRASH] != TIMER_STATE_OFF;
        if (moon_crash_timer_running) {
            // @recomp Use normal alignment and shift down for clock
            gEXSetRectAlign(OVERLAY_DISP++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, margin_reduction * 4, 0, margin_reduction * 4);
            gEXSetViewportAlign(OVERLAY_DISP++, G_EX_ORIGIN_NONE, 0, margin_reduction * 4);
            Interface_SetOrthoView(interfaceCtx);
        }

        Interface_DrawTimers(play);
        
        // @recomp Restore normal alignment and shift down for minigame countdown or clock
        gEXSetRectAlign(OVERLAY_DISP++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, 0, 0, 0);
        gEXSetViewportAlign(OVERLAY_DISP++, G_EX_ORIGIN_NONE, 0, 0);
        gEXSetScissorAlign(OVERLAY_DISP++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, 0, 0, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        Interface_SetOrthoView(interfaceCtx);
    }

    // Draw pictograph focus icons
    if (sPictoState == PICTO_BOX_STATE_LENS) {

        Gfx_SetupDL39_Overlay(play->state.gfxCtx);

        gDPSetAlphaCompare(OVERLAY_DISP++, G_AC_THRESHOLD);
        gDPSetRenderMode(OVERLAY_DISP++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPSetCombineMode(OVERLAY_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 155, 255);
        gDPLoadTextureBlock_4b(OVERLAY_DISP++, gPictoBoxFocusBorderTex, G_IM_FMT_IA, 16, 16, 0, G_TX_MIRROR | G_TX_WRAP,
                               G_TX_MIRROR | G_TX_WRAP, 4, 4, G_TX_NOLOD, G_TX_NOLOD);

        gSPTextureRectangle(OVERLAY_DISP++, R_PICTO_FOCUS_BORDER_TOPLEFT_X << 2, R_PICTO_FOCUS_BORDER_TOPLEFT_Y << 2,
                            (R_PICTO_FOCUS_BORDER_TOPLEFT_X << 2) + (16 << 2),
                            (R_PICTO_FOCUS_BORDER_TOPLEFT_Y << 2) + (16 << 2), G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
        gSPTextureRectangle(OVERLAY_DISP++, R_PICTO_FOCUS_BORDER_TOPRIGHT_X << 2, R_PICTO_FOCUS_BORDER_TOPRIGHT_Y << 2,
                            (R_PICTO_FOCUS_BORDER_TOPRIGHT_X << 2) + (16 << 2),
                            (R_PICTO_FOCUS_BORDER_TOPRIGHT_Y << 2) + (16 << 2), G_TX_RENDERTILE, 512, 0, 1 << 10,
                            1 << 10);
        gSPTextureRectangle(
            OVERLAY_DISP++, R_PICTO_FOCUS_BORDER_BOTTOMLEFT_X << 2, R_PICTO_FOCUS_BORDER_BOTTOMLEFT_Y << 2,
            (R_PICTO_FOCUS_BORDER_BOTTOMLEFT_X << 2) + (16 << 2), (R_PICTO_FOCUS_BORDER_BOTTOMLEFT_Y << 2) + (16 << 2),
            G_TX_RENDERTILE, 0, 512, 1 << 10, 1 << 10);
        gSPTextureRectangle(
            OVERLAY_DISP++, R_PICTO_FOCUS_BORDER_BOTTOMRIGHT_X << 2, R_PICTO_FOCUS_BORDER_BOTTOMRIGHT_Y << 2,
            (R_PICTO_FOCUS_BORDER_BOTTOMRIGHT_X << 2) + (16 << 2),
            (R_PICTO_FOCUS_BORDER_BOTTOMRIGHT_Y << 2) + (16 << 2), G_TX_RENDERTILE, 512, 512, 1 << 10, 1 << 10);

        gDPLoadTextureBlock_4b(OVERLAY_DISP++, gPictoBoxFocusIconTex, G_IM_FMT_I, 32, 16, 0, G_TX_NOMIRROR | G_TX_WRAP,
                               G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

        gSPTextureRectangle(OVERLAY_DISP++, R_PICTO_FOCUS_ICON_X << 2, R_PICTO_FOCUS_ICON_Y << 2,
                            (R_PICTO_FOCUS_ICON_X << 2) + 0x80, (R_PICTO_FOCUS_ICON_Y << 2) + (16 << 2),
                            G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);

        gDPLoadTextureBlock_4b(OVERLAY_DISP++, gPictoBoxFocusTextTex, G_IM_FMT_I, 32, 8, 0, G_TX_NOMIRROR | G_TX_WRAP,
                               G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

        gSPTextureRectangle(OVERLAY_DISP++, R_PICTO_FOCUS_TEXT_X << 2, R_PICTO_FOCUS_TEXT_Y << 2,
                            (R_PICTO_FOCUS_TEXT_X << 2) + 0x80, (R_PICTO_FOCUS_TEXT_Y << 2) + 0x20, G_TX_RENDERTILE, 0,
                            0, 1 << 10, 1 << 10);
    }

    // Draw pictograph photo
    if (sPictoState >= PICTO_BOX_STATE_SETUP_PHOTO) {
        if (!(play->actorCtx.flags & ACTORCTX_FLAG_PICTO_BOX_ON)) {
            Play_CompressI8ToI5((play->pictoPhotoI8 != NULL) ? play->pictoPhotoI8 : gWorkBuffer,
                                (u8*)gSaveContext.pictoPhotoI5, PICTO_PHOTO_WIDTH * PICTO_PHOTO_HEIGHT);

            interfaceCtx->unk_222 = interfaceCtx->unk_224 = 0;

            sPictoState = PICTO_BOX_STATE_OFF;
            gSaveContext.hudVisibility = HUD_VISIBILITY_IDLE;
            Interface_SetHudVisibility(HUD_VISIBILITY_ALL);
        } else {
            s16 pictoRectTop;
            s16 pictoRectLeft;

            if (sPictoState == PICTO_BOX_STATE_SETUP_PHOTO) {
                sPictoState = PICTO_BOX_STATE_PHOTO;
                Message_StartTextbox(play, 0xF8, NULL);
                Interface_SetHudVisibility(HUD_VISIBILITY_NONE);
                player->stateFlags1 |= PLAYER_STATE1_200;
            }

            gDPPipeSync(OVERLAY_DISP++);
            gDPSetRenderMode(OVERLAY_DISP++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
            gDPSetCombineMode(OVERLAY_DISP++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
            gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 200, 200, 200, 250);
            gDPFillRectangle(OVERLAY_DISP++, 70, 22, 251, 151);

            Gfx_SetupDL39_Overlay(play->state.gfxCtx);

            gDPSetRenderMode(OVERLAY_DISP++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
            gDPSetCombineMode(OVERLAY_DISP++, G_CC_MODULATEI_PRIM, G_CC_MODULATEI_PRIM);
            gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 250, 160, 160, 255);

            // Picture is offset up by 33 pixels to give room for the message box at the bottom
            pictoRectTop = PICTO_PHOTO_TOPLEFT_Y - 33;
            for (sp2CC = 0; sp2CC < (PICTO_PHOTO_HEIGHT / 8); sp2CC++, pictoRectTop += 8) {
                pictoRectLeft = PICTO_PHOTO_TOPLEFT_X;
                gDPLoadTextureBlock(OVERLAY_DISP++,
                                    (u8*)((play->pictoPhotoI8 != NULL) ? play->pictoPhotoI8 : gWorkBuffer) +
                                        (0x500 * sp2CC),
                                    G_IM_FMT_I, G_IM_SIZ_8b, PICTO_PHOTO_WIDTH, 8, 0, G_TX_NOMIRROR | G_TX_WRAP,
                                    G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

                gSPTextureRectangle(OVERLAY_DISP++, pictoRectLeft << 2, pictoRectTop << 2,
                                    (pictoRectLeft + PICTO_PHOTO_WIDTH) << 2, (pictoRectTop << 2) + (8 << 2),
                                    G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
            }
        }
    }

    // Draw over the entire screen (used in gameover)
    if (interfaceCtx->screenFillAlpha != 0) {
        gDPPipeSync(OVERLAY_DISP++);
        gSPDisplayList(OVERLAY_DISP++, sScreenFillSetupDL);
        gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 0, 0, interfaceCtx->screenFillAlpha);
        gSPDisplayList(OVERLAY_DISP++, D_0E000000.fillRect);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

extern u64 gCarrotIconTex[];
extern u64 gBeaverRingIconTex[];
extern u64 gSwordTrainingLogIconTex[];
extern u64 gFishermanMinigameTorchIconTex[];
extern u64 gArcheryScoreIconTex[];
extern u16 sMinigameScoreDigits[];

// @recomp Patched to draw the carrot icons with an extended gbi texrect so they don't inherit the current origin.
RECOMP_PATCH void Interface_DrawMinigameIcons(PlayState* play) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    s16 i;
    s16 numDigitsDrawn;
    s16 rectX;
    s16 rectY;
    s16 width;
    s16 height;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL39_Overlay(play->state.gfxCtx);

    if ((play->pauseCtx.state == PAUSE_STATE_OFF) && (play->pauseCtx.debugEditor == DEBUG_EDITOR_NONE)) {
        // Carrots rendering if the action corresponds to riding a horse
        if (interfaceCtx->unk_212 == DO_ACTION_FASTER) {
            // Load Carrot Icon
            gDPLoadTextureBlock(OVERLAY_DISP++, gCarrotIconTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 16, 16, 0,
                                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK,
                                G_TX_NOLOD, G_TX_NOLOD);

            rectX = 110;
            rectY = (interfaceCtx->minigameState != MINIGAME_STATE_NONE) ? 200 : 56;

            // Draw 6 carrots
            for (i = 1; i < 7; i++, rectX += 16) {
                // Carrot Color (based on availability)
                if ((interfaceCtx->numHorseBoosts == 0) || (interfaceCtx->numHorseBoosts < i)) {
                    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 150, 255, interfaceCtx->aAlpha);
                } else {
                    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, interfaceCtx->aAlpha);
                }

                // @recomp Draw the carrot icons with an extended gbi texrect, using none as the origin to override the current alignment.
                gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, rectX << 2, rectY << 2, (rectX + 16) << 2, (rectY + 16) << 2,
                                    G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
            }
        }

        if (gSaveContext.minigameStatus == MINIGAME_STATUS_ACTIVE) {
            gDPPipeSync(OVERLAY_DISP++);
            gDPSetCombineLERP(OVERLAY_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0,
                              PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);
            width = 24;
            height = 16;
            rectX = 20;
            if (gSaveContext.save.saveInfo.playerData.healthCapacity > 0xA0) {
                rectY = 75; // two rows of hearts
            } else {
                rectY = 67; // one row of hearts
            }

            if (gSaveContext.save.entrance == ENTRANCE(WATERFALL_RAPIDS, 1)) {
                gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, interfaceCtx->bAlpha);
                gDPSetEnvColor(OVERLAY_DISP++, 0, 0, 0, 255);
                gDPLoadTextureBlock(OVERLAY_DISP++, gBeaverRingIconTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 24, 16, 0,
                                    G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK,
                                    G_TX_NOLOD, G_TX_NOLOD);
            } else if (play->sceneId == SCENE_DOUJOU) {
                gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 140, 50, interfaceCtx->bAlpha);
                gDPSetEnvColor(OVERLAY_DISP++, 0, 0, 0, 255);
                gDPLoadTextureBlock(OVERLAY_DISP++, gSwordTrainingLogIconTex, G_IM_FMT_IA, G_IM_SIZ_8b, 24, 16, 0,
                                    G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK,
                                    G_TX_NOLOD, G_TX_NOLOD);
            } else if (play->sceneId == SCENE_30GYOSON) {
                width = 16;
                height = 30;
                rectX = 24;
                gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 100, 75, interfaceCtx->bAlpha);
                gDPSetEnvColor(OVERLAY_DISP++, 55, 55, 0, 255);
                gDPLoadTextureBlock(OVERLAY_DISP++, gFishermanMinigameTorchIconTex, G_IM_FMT_IA, G_IM_SIZ_8b, 16, 30, 0,
                                    G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK,
                                    G_TX_NOLOD, G_TX_NOLOD);
            } else {
                gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, interfaceCtx->bAlpha);
                gDPSetEnvColor(OVERLAY_DISP++, 0, 0, 0, 255);
                gDPLoadTextureBlock(OVERLAY_DISP++, gArcheryScoreIconTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 24, 16, 0,
                                    G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK,
                                    G_TX_NOLOD, G_TX_NOLOD);
            }

            gSPTextureRectangle(OVERLAY_DISP++, (rectX << 2), (rectY << 2), ((rectX + width) << 2),
                                ((rectY + height) << 2), G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
            gDPPipeSync(OVERLAY_DISP++);
            gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, interfaceCtx->bAlpha);
            gDPSetCombineLERP(OVERLAY_DISP++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0,
                              0, PRIMITIVE, 0);

            if (play->sceneId == SCENE_30GYOSON) {
                rectX += 20;
                if (gSaveContext.save.saveInfo.playerData.healthCapacity > 0xA0) {
                    rectY = 87; // two rows of hearts
                } else {
                    rectY = 79; // one row of hearts
                }
            } else {
                rectX += 26;
            }

            for (i = 0, numDigitsDrawn = 0; i < 4; i++) {
                if ((sMinigameScoreDigits[i] != 0) || (numDigitsDrawn != 0) || (i >= 3)) {
                    OVERLAY_DISP =
                        Gfx_DrawTexRectI8(OVERLAY_DISP, ((u8*)gCounterDigit0Tex + (8 * 16 * sMinigameScoreDigits[i])),
                                          8, 0x10, rectX, rectY - 2, 9, 0xFA, 0x370, 0x370);
                    rectX += 9;
                    numDigitsDrawn++;
                }
            }

            gDPPipeSync(OVERLAY_DISP++);
            gDPSetCombineMode(OVERLAY_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx);
}


extern s16 sTextboxWidth;
extern s16 sTextboxHeight;
extern s16 sTextboxTexWidth;
extern s16 sTextboxTexHeight;
extern u64 gOcarinaTrebleClefTex[];

// @recomp Patch textboxes to use ortho tris with a matrix so they can be interpolated.
RECOMP_PATCH void Message_DrawTextBox(PlayState* play, Gfx** gfxP) {
    MessageContext* msgCtx = &play->msgCtx;
    Gfx* gfx = *gfxP;

    gDPPipeSync(gfx++);

    if (((u32)msgCtx->textBoxType == TEXTBOX_TYPE_0) || (msgCtx->textBoxType == TEXTBOX_TYPE_2) ||
        (msgCtx->textBoxType == TEXTBOX_TYPE_9) || (msgCtx->textBoxType == TEXTBOX_TYPE_A)) {
        gDPSetRenderMode(gfx++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
    } else if (msgCtx->textBoxType == TEXTBOX_TYPE_3) {
        gDPSetAlphaCompare(gfx++, G_AC_THRESHOLD);
        gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    }

    gDPSetPrimColor(gfx++, 0, 0, msgCtx->textboxColorRed, msgCtx->textboxColorGreen, msgCtx->textboxColorBlue,
                    msgCtx->textboxColorAlphaCurrent);

    if (((u32)msgCtx->textBoxType == TEXTBOX_TYPE_0) || (msgCtx->textBoxType == TEXTBOX_TYPE_2) ||
        (msgCtx->textBoxType == TEXTBOX_TYPE_6) || (msgCtx->textBoxType == TEXTBOX_TYPE_8) ||
        (msgCtx->textBoxType == TEXTBOX_TYPE_9) || (msgCtx->textBoxType == TEXTBOX_TYPE_A)) {
        gDPLoadTextureBlock_4b(gfx++, msgCtx->textboxSegment, G_IM_FMT_I, 128, 64, 0, G_TX_MIRROR | G_TX_WRAP,
                               G_TX_NOMIRROR | G_TX_WRAP, 7, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    } else {
        gDPPipeSync(gfx++);

        if (msgCtx->textBoxType == TEXTBOX_TYPE_3) {
            gDPSetEnvColor(gfx++, 0, 0, 0, 255);
        } else if (msgCtx->textBoxType == TEXTBOX_TYPE_D) {
            gDPSetEnvColor(gfx++, 20, 0, 10, 255);
        } else {
            gDPSetEnvColor(gfx++, 50, 20, 0, 255);
        }
        gDPLoadTextureBlock_4b(gfx++, msgCtx->textboxSegment, G_IM_FMT_IA, 128, 64, 0, G_TX_MIRROR | G_TX_WRAP,
                               G_TX_MIRROR | G_TX_WRAP, 7, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    }

    // @recomp Push the old RDP/RSP params.
    gEXPushProjectionMatrix(gfx++);
    gEXPushGeometryMode(gfx++);
    gEXPushOtherMode(gfx++);
    gEXPushViewport(gfx++);
    gEXMatrixGroupSimple(gfx++, TEXTBOX_TRANSFORM_PROJECTION_ID, G_EX_PUSH, G_MTX_PROJECTION,
        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);
    gEXMatrixGroupSimple(gfx++, TEXTBOX_TRANSFORM_ID, G_EX_PUSH, G_MTX_MODELVIEW,
        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);

    // @recomp Set up the RSP params.
    gSPLoadGeometryMode(gfx++, 0);
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    Vp* textbox_viewport = GRAPH_ALLOC(play->state.gfxCtx, sizeof(Vp));

    textbox_viewport->vp.vscale[0] = (gCfbWidth / 2) << 2;
    textbox_viewport->vp.vscale[1] = (gCfbHeight / 2) << 2;
    textbox_viewport->vp.vscale[2] = G_MAXZ;
    textbox_viewport->vp.vscale[3] = 0;
    
    textbox_viewport->vp.vtrans[0] = (gCfbWidth / 2) << 2;
    textbox_viewport->vp.vtrans[1] = (gCfbHeight / 2) << 2;
    textbox_viewport->vp.vtrans[2] = 0;
    textbox_viewport->vp.vtrans[3] = 0;

    gSPViewport(gfx++, textbox_viewport);

    if (msgCtx->textBoxType == TEXTBOX_TYPE_A) {
        gSPTextureRectangle(gfx++, msgCtx->textboxX << 2, (msgCtx->textboxY + 22) << 2,
                            (msgCtx->textboxX + msgCtx->unk12008) << 2, (msgCtx->textboxY + 54) << 2, G_TX_RENDERTILE,
                            0, 6, msgCtx->unk1200C << 1, 2 << 10);
    } else {
        const s32 base_textbox_width = 256;
        const s32 base_textbox_height = 64;

        // @recomp Calculate a scale based on the the target size derivatives.
        f32 textbox_scale_x = sTextboxWidth / (f32)base_textbox_width;
        f32 textbox_scale_y = sTextboxHeight / (f32)base_textbox_height;

        // @recomp Calculate the textbox center.
        f32 textbox_center_x = msgCtx->textboxX + sTextboxWidth / 2.0f;
        f32 textbox_center_y = msgCtx->textboxY + sTextboxHeight / 2.0f;

        // @recomp Allocate and build the matrices.
        Mtx* textbox_model_matrix = GRAPH_ALLOC(play->state.gfxCtx, sizeof(Mtx));
        Mtx* textbox_proj_matrix  = GRAPH_ALLOC(play->state.gfxCtx, sizeof(Mtx));
        guOrtho(textbox_proj_matrix, 0, gCfbWidth, gCfbHeight, 0, -1.0f, 1.0f, 1.0f);
        Mtx_SetTranslateScaleMtx(textbox_model_matrix, textbox_scale_x, textbox_scale_y, 1.0f, textbox_center_x, textbox_center_y, 0.0f);

        // @recomp Static vertex list for the textboxes.
        static Vtx textbox_verts[4] = {
            {{{-base_textbox_width / 2, -base_textbox_height / 2, 0}, 0, {                 0 << 5,                    0 << 5}, {0, 0, 0, 0xFF}}},
            {{{ base_textbox_width / 2, -base_textbox_height / 2, 0}, 0, {base_textbox_width << 5,                    0 << 5}, {0, 0, 0, 0xFF}}},
            {{{-base_textbox_width / 2,  base_textbox_height / 2, 0}, 0, {                 0 << 5,  base_textbox_height << 5}, {0, 0, 0, 0xFF}}},
            {{{ base_textbox_width / 2,  base_textbox_height / 2, 0}, 0, {base_textbox_width << 5,  base_textbox_height << 5}, {0, 0, 0, 0xFF}}},
        };

        // @recomp Loads the matrices, then the verts, and then draw the textbox.
        gSPMatrix(gfx++, textbox_model_matrix, G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPMatrix(gfx++, textbox_proj_matrix, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
        gSPVertex(gfx++, textbox_verts, 4, 0);

        // @recomp Use point filtering as this texture doesn't work well with bilerp. Also enable perspective correction for drawing tris.
        gDPSetTextureFilter(gfx++, G_TF_POINT);
        gDPSetTexturePersp(gfx++, G_TP_PERSP);
        gSP2Triangles(gfx++, 0, 1, 3, 0x0, 0, 3, 2, 0x0);

        // @recomp Restore bilerp filtering, disable perspective correction, and pop the model matrix
        gDPSetTextureFilter(gfx++, G_TF_BILERP);
        gDPSetTexturePersp(gfx++, G_TP_NONE);
        gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
    }

    // Draw treble clef
    if (msgCtx->textBoxType == TEXTBOX_TYPE_3) {
        gDPPipeSync(gfx++);
        gDPSetCombineLERP(gfx++, 1, 0, PRIMITIVE, 0, TEXEL0, 0, PRIMITIVE, 0, 1, 0, PRIMITIVE, 0, TEXEL0, 0, PRIMITIVE,
                          0);
        gDPSetPrimColor(gfx++, 0, 0, 255, 100, 0, 255);
        gDPLoadTextureBlock_4b(gfx++, gOcarinaTrebleClefTex, G_IM_FMT_I, 16, 32, 0, G_TX_MIRROR | G_TX_WRAP,
                               G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        gSPTextureRectangle(gfx++, 78 << 2, 166 << 2, 94 << 2, 198 << 2, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
    }

    // @recomp Restore the old RDP/RSP params.
    gEXPopProjectionMatrix(gfx++);
    gEXPopGeometryMode(gfx++);
    gEXPopOtherMode(gfx++);
    gEXPopViewport(gfx++);
    gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
    gEXPopMatrixGroup(gfx++, G_MTX_MODELVIEW);
    gEXPopMatrixGroup(gfx++, G_MTX_PROJECTION);
    gSPPerspNormalize(gfx++, play->view.perspNorm);

    *gfxP = gfx++;
}

void View_SetScissor(Gfx** gfx, s32 ulx, s32 uly, s32 lrx, s32 lry);

// @recomp Patched to not actually letterbox the scissor. The letterbox effect will be achieved by drawing an overlay on top instead, which
// will get interpolated unlike a scissor.
RECOMP_PATCH void View_ApplyLetterbox(View* view) {
    s32 letterboxY;
    s32 letterboxX;
    s32 pad1;
    s32 ulx;
    s32 uly;
    s32 lrx;
    s32 lry;

    OPEN_DISPS(view->gfxCtx);

    // @recomp Disable letterboxing for the scissor.
    letterboxY = 0; // ShrinkWindow_Letterbox_GetSize();

    letterboxX = -1; // The following is optimized to varX = 0 but affects codegen

    if (letterboxX < 0) {
        letterboxX = 0;
    }
    if (letterboxX > (SCREEN_WIDTH / 2)) {
        letterboxX = SCREEN_WIDTH / 2;
    }

    if (letterboxY < 0) {
        letterboxY = 0;
    } else if (letterboxY > (SCREEN_HEIGHT / 2)) {
        letterboxY = SCREEN_HEIGHT / 2;
    }

    ulx = view->viewport.leftX + letterboxX;
    uly = view->viewport.topY + letterboxY;
    lrx = view->viewport.rightX - letterboxX;
    lry = view->viewport.bottomY - letterboxY;

    gDPPipeSync(POLY_OPA_DISP++);
    {
        s32 pad2;
        Gfx* polyOpa;

        polyOpa = POLY_OPA_DISP;
        View_SetScissor(&polyOpa, ulx, uly, lrx, lry);
        POLY_OPA_DISP = polyOpa;
    }

    gDPPipeSync(POLY_XLU_DISP++);
    {
        Gfx* polyXlu;
        s32 pad3;

        polyXlu = POLY_XLU_DISP;
        View_SetScissor(&polyXlu, ulx, uly, lrx, lry);
        POLY_XLU_DISP = polyXlu;
    }

    CLOSE_DISPS(view->gfxCtx);
}

typedef struct {
    /* 0x0 */ s8 letterboxTarget;
    /* 0x1 */ s8 letterboxSize;
    /* 0x2 */ s8 pillarboxTarget;
    /* 0x3 */ s8 pillarboxSize;
} ShrinkWindow; // size = 0x4

extern ShrinkWindow* sShrinkWindowPtr;

// @recomp Replace the rects used to letterbox with ortho tris so they can be interpolated.
RECOMP_PATCH void ShrinkWindow_Draw(GraphicsContext* gfxCtx) {
    Gfx* gfx;
    s8 letterboxSize = sShrinkWindowPtr->letterboxSize;
    s8 pillarboxSize = sShrinkWindowPtr->pillarboxSize;

    // @recomp Always draw the letterbox tris so that they the matrices can still be interpolated on the first frame they appear.
    // Avoid drawing it at a specific point in the pausing process.
    if (R_PAUSE_BG_PRERENDER_STATE != PAUSE_BG_PRERENDER_PROCESS) { //letterboxSize > 0) {
        OPEN_DISPS(gfxCtx);

        gfx = OVERLAY_DISP;

        // @recomp Push the old RDP/RSP params.
        gEXPushProjectionMatrix(gfx++);
        gEXPushGeometryMode(gfx++);
        gEXPushOtherMode(gfx++);
        gEXPushCombineMode(gfx++);

        // @recomp Set up the new RSP/RDP params.
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        gDPSetRenderMode(gfx++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
        gDPSetCombineLERP(gfx++, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1);
        gSPLoadGeometryMode(gfx++, 0);

        // @recomp Set up the letterbox matrix groups.
        gEXMatrixGroupSimple(gfx++, TEXTBOX_TRANSFORM_PROJECTION_ID, G_EX_PUSH, G_MTX_PROJECTION,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);
        gEXMatrixGroupSimple(gfx++, TEXTBOX_TRANSFORM_ID, G_EX_PUSH, G_MTX_MODELVIEW,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);

        // @recomp Allocate and build the matrices.
        Mtx* ortho_matrix = GRAPH_ALLOC(gfxCtx, sizeof(Mtx));
        Mtx* letterbox_matrix_top = GRAPH_ALLOC(gfxCtx, sizeof(Mtx));
        Mtx* letterbox_matrix_bottom = GRAPH_ALLOC(gfxCtx, sizeof(Mtx));

        guOrtho(ortho_matrix, -SCREEN_WIDTH / 2, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, -SCREEN_HEIGHT / 2, -1.0f, 1.0f, 1.0f);
        Mtx_SetTranslateScaleMtx(letterbox_matrix_top,    1.0f,  1.0f, 1.0f, 0.0f, -SCREEN_HEIGHT / 2 + letterboxSize, 0.0f);
        Mtx_SetTranslateScaleMtx(letterbox_matrix_bottom, 1.0f, -1.0f, 1.0f, 0.0f,  SCREEN_HEIGHT / 2 - letterboxSize, 0.0f);
        gSPMatrix(gfx++, ortho_matrix, G_MTX_PROJECTION | G_MTX_NOPUSH | G_MTX_LOAD);

        // @recomp Static variable holding the letterbox vertices. The origin is at the edge to make calculating the translation matrix easy.
        static Vtx letterbox_verts[4] = {
            {{{-32000, -120, 0}, 0, {0, 0}, {0, 0, 0, 0xFF}}},
            {{{ 32000, -120, 0}, 0, {0, 0}, {0, 0, 0, 0xFF}}},
            {{{-32000,    0, 0}, 0, {0, 0}, {0, 0, 0, 0xFF}}},
            {{{ 32000,    0, 0}, 0, {0, 0}, {0, 0, 0, 0xFF}}},
        };

        // @recomp Draw the top letterbox element.
        gSPMatrix(gfx++, letterbox_matrix_top, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_LOAD);
        gSPVertex(gfx++, letterbox_verts, 4, 0);
        gSP2Triangles(gfx++, 0, 1, 3, 0x0, 0, 3, 2, 0x0);
        
        // @recomp Draw the bottom letterbox element.
        gSPMatrix(gfx++, letterbox_matrix_bottom, G_MTX_MODELVIEW | G_MTX_NOPUSH | G_MTX_LOAD);
        gSPVertex(gfx++, letterbox_verts, 4, 0);
        gSP2Triangles(gfx++, 0, 3, 1, 0x0, 0, 2, 3, 0x0);

        // @recomp Restore the old RDP/RSP params.
        gEXPopProjectionMatrix(gfx++);
        gEXPopGeometryMode(gfx++);
        gEXPopOtherMode(gfx++);
        gEXPopCombineMode(gfx++);
        gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
        gEXPopMatrixGroup(gfx++, G_MTX_MODELVIEW);
        gEXPopMatrixGroup(gfx++, G_MTX_PROJECTION);

        OVERLAY_DISP = gfx++;

        CLOSE_DISPS(gfxCtx);
    }

    if (pillarboxSize > 0) {
        OPEN_DISPS(gfxCtx);

        gfx = OVERLAY_DISP;

        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_FILL);
        gDPSetRenderMode(gfx++, G_RM_NOOP, G_RM_NOOP2);
        gDPSetFillColor(gfx++, (GPACK_RGBA5551(0, 0, 0, 1) << 16) | GPACK_RGBA5551(0, 0, 0, 1));

        gDPFillRectangle(gfx++, 0, 0, pillarboxSize - 1, gScreenHeight - 1);
        gDPFillRectangle(gfx++, gScreenWidth - pillarboxSize, 0, gScreenWidth - 1, gScreenHeight - 1);

        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, 0);

        gDPFillRectangle(gfx++, pillarboxSize, 0, pillarboxSize + 2, gScreenHeight);
        gDPFillRectangle(gfx++, gScreenWidth - pillarboxSize - 2, 0, gScreenWidth - pillarboxSize, gScreenHeight);

        gDPPipeSync(gfx++);
        OVERLAY_DISP = gfx++;

        CLOSE_DISPS(gfxCtx);
    }
}

extern u64 gSceneTitleCardGradientTex[];

// @recomp Patch the scene title card (the one with purple background when entering a new scene) 
// to not glitch out on the right edge, which is hidden by overscan on N64.
RECOMP_PATCH void Message_DrawSceneTitleCard(PlayState* play, Gfx** gfxP) {
    MessageContext* msgCtx = &play->msgCtx;
    Gfx* gfx;

    gfx = *gfxP;
    Gfx_SetupDL39_Ptr(&gfx);

    gDPSetCombineLERP(gfx++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0, PRIMITIVE,
                      ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);
    gDPSetTextureFilter(gfx++, G_TF_BILERP);
    gDPSetAlphaDither(gfx++, G_AD_NOTPATTERN);
    gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, msgCtx->textboxColorAlphaCurrent);
    gDPSetEnvColor(gfx++, 140, 40, 160, 255);
    // @recomp Use clamp as the addressing mode to prevent the texture from wrapping on the left edge, which can be visible
    // if HD textures are in use.
    gDPLoadTextureBlock(gfx++, gSceneTitleCardGradientTex, G_IM_FMT_I, G_IM_SIZ_8b, 64, 1, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                        G_TX_NOMIRROR | G_TX_CLAMP, 6, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    // @recomp Decrease dsdx from the original 204 to 200 in order to hide the glitching on the right edge.
    gSPTextureRectangle(gfx++, 0, XREG(77) << 2, 320 << 2, (XREG(77) + XREG(76)) << 2, G_TX_RENDERTILE, 0, 0, 200,
                        1 << 10);
    gDPPipeSync(gfx++);
    gDPSetCombineLERP(gfx++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0);
    gDPSetEnvColor(gfx++, 0, 0, 0, 255);

    if ((msgCtx->currentTextId < 0x1BB2) || (msgCtx->currentTextId >= 0x1BB7)) {
        msgCtx->unk11FF8 = XREG(75);
    }
    msgCtx->unk11FFA = XREG(74);
    Message_DrawTextNES(play, &gfx, 0);
    *gfxP = gfx++;
}
