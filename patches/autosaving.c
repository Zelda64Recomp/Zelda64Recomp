#include "patches.h"
#include "play_patches.h"
#include "z64save.h"
#include "overlays/gamestates/ovl_file_choose/z_file_select.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"

#define SAVE_TYPE_AUTOSAVE 2 

void Sram_SyncWriteToFlash(SramContext* sramCtx, s32 curPage, s32 numPages);

void do_autosave(SramContext* sramCtx) {
    s32 fileNum = gSaveContext.fileNum;

    gSaveContext.save.isOwlSave = SAVE_TYPE_AUTOSAVE;

    gSaveContext.save.saveInfo.checksum = 0;
    gSaveContext.save.saveInfo.checksum = Sram_CalcChecksum(&gSaveContext, offsetof(SaveContext, fileNum));

    // Copy the saved parts of the global save context into the sram saving buffer.
    Lib_MemCpy(sramCtx->saveBuf, &gSaveContext, offsetof(SaveContext, fileNum));
    // Synchronously save into the owl save slot and the backup owl save slot. 
    Sram_SyncWriteToFlash(sramCtx, gFlashOwlSaveStartPages[fileNum * 2], gFlashOwlSaveNumPages[fileNum * 2]);
    Sram_SyncWriteToFlash(sramCtx, gFlashOwlSaveStartPages[fileNum * 2 + 1], gFlashOwlSaveNumPages[fileNum * 2 + 1]);
}

// @recomp Do not clear the save if the save was an autosave.
void func_80147314(SramContext* sramCtx, s32 fileNum) {
    s32 save_type = gSaveContext.save.isOwlSave;
    gSaveContext.save.isOwlSave = false;

    // @recomp Check if this owl save was actually an autosave and don't clear it if so.
    if (save_type != SAVE_TYPE_AUTOSAVE) {
        gSaveContext.save.saveInfo.playerData.newf[0] = '\0';
        gSaveContext.save.saveInfo.playerData.newf[1] = '\0';
        gSaveContext.save.saveInfo.playerData.newf[2] = '\0';
        gSaveContext.save.saveInfo.playerData.newf[3] = '\0';
        gSaveContext.save.saveInfo.playerData.newf[4] = '\0';
        gSaveContext.save.saveInfo.playerData.newf[5] = '\0';

        gSaveContext.save.saveInfo.checksum = 0;
        gSaveContext.save.saveInfo.checksum = Sram_CalcChecksum(&gSaveContext, offsetof(SaveContext, fileNum));

        Lib_MemCpy(sramCtx->saveBuf, &gSaveContext, offsetof(SaveContext, fileNum));
        Sram_SyncWriteToFlash(sramCtx, gFlashOwlSaveStartPages[fileNum * 2], gFlashOwlSaveNumPages[fileNum * 2]);
        //! Note: should be `gFlashOwlSaveNumPages[fileNum * 2 + 1]`?
        Sram_SyncWriteToFlash(sramCtx, gFlashOwlSaveStartPages[fileNum * 2 + 1], gFlashOwlSaveNumPages[fileNum * 2]);

        gSaveContext.save.isOwlSave = true;

        gSaveContext.save.saveInfo.playerData.newf[0] = 'Z';
        gSaveContext.save.saveInfo.playerData.newf[1] = 'E';
        gSaveContext.save.saveInfo.playerData.newf[2] = 'L';
        gSaveContext.save.saveInfo.playerData.newf[3] = 'D';
        gSaveContext.save.saveInfo.playerData.newf[4] = 'A';
        gSaveContext.save.saveInfo.playerData.newf[5] = '3';
    }
}

extern u16 D_801F6AF0;
extern u8 D_801F6AF2;

// @recomp Patched to tag autosaves as owl saves so that the modified version of func_80147314 still clears them.
void Sram_EraseSave(FileSelectState* fileSelect2, SramContext* sramCtx, s32 fileNum) {
    FileSelectState* fileSelect = fileSelect2;
    s32 pad;

    if (gSaveContext.flashSaveAvailable) {
        if (fileSelect->isOwlSave[fileNum + 2]) {
            // @recomp Override the save type to a regular owl save.
            fileSelect->isOwlSave[fileNum + 2] = true;
            func_80147314(sramCtx, fileNum);
            fileSelect->isOwlSave[fileNum + 2] = false;
        }
        bzero(sramCtx->saveBuf, SAVE_BUFFER_SIZE);
        Lib_MemCpy(&gSaveContext, sramCtx->saveBuf, sizeof(Save));
    }

    gSaveContext.save.time = D_801F6AF0;
    gSaveContext.flashSaveAvailable = D_801F6AF2;
}

SaveContext prev_save_ctx;
int bcmp_recomp(void* __s1, void* __s2, int __n);

#define SAVE_COMPARE_MEMBER(ctx1, ctx2, member) \
    if ((ctx1)->member != ((ctx2)->member)) { \
        return true; \
    }

#define SAVE_COMPARE_MEMBER_ARRAY(ctx1, ctx2, member) \
    if (bcmp_recomp(&(ctx1)->member, &(ctx2)->member, sizeof((ctx1)->member))) { \
        recomp_printf(#member " differed\n"); \
        return true; \
    }

bool autosave_compare_saves(SaveContext* a, SaveContext* b) {
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.playerData.healthCapacity);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.playerData.isMagicAcquired);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.playerData.isDoubleMagicAcquired);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.playerData.doubleDefense);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.playerData.owlActivationFlags);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.inventory.items);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.inventory.upgrades);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.inventory.questItems);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.inventory.dungeonItems);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.inventory.dungeonKeys);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.inventory.strayFairies);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.permanentSceneFlags);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.dekuPlaygroundHighScores);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.scenesVisible);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.stolenItems);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.highScores);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.weekEventReg);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.regionsVisited);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.worldMapCloudVisibility);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.scarecrowSpawnSongSet);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.scarecrowSpawnSong);
    // SAVE_COMPARE_MEMBER_ARRAY(a, b, eventInf);

    return false;
}

Gfx* Gfx_DrawRect_DropShadow(Gfx* gfx, s16 rectLeft, s16 rectTop, s16 rectWidth, s16 rectHeight, u16 dsdx, u16 dtdy,
                             s16 r, s16 g, s16 b, s16 a);

int autosave_icon_counter = 0;

#define AUTOSAVE_ICON_FADE_OUT_FRAMES 5
#define AUTOSAVE_ICON_FADE_IN_FRAMES 5
#define AUTOSAVE_ICON_SHOW_FRAMES 30
#define AUTOSAVE_ICON_TOTAL_FRAMES (AUTOSAVE_ICON_FADE_IN_FRAMES + AUTOSAVE_ICON_SHOW_FRAMES + AUTOSAVE_ICON_FADE_OUT_FRAMES)

#define AUTOSAVE_ICON_WIDTH 24
#define AUTOSAVE_ICON_HEIGHT 16
#define AUTOSAVE_ICON_DRAW_WIDTH (AUTOSAVE_ICON_WIDTH * 3 / 4)
#define AUTOSAVE_ICON_DRAW_HEIGHT (AUTOSAVE_ICON_HEIGHT * 3 / 4)
#define AUTOSAVE_ICON_X (SCREEN_WIDTH - AUTOSAVE_ICON_DRAW_WIDTH - 4)
#define AUTOSAVE_ICON_Y (SCREEN_HEIGHT - AUTOSAVE_ICON_DRAW_HEIGHT - 4)

INCBIN(autosave_icon, "autosave.rgba32.bin");

Gfx* GfxEx_DrawRect_DropShadow(Gfx* gfx, s16 rectLeft, s16 rectTop, s16 rectWidth, s16 rectHeight, u16 dsdx, u16 dtdy,
                             s16 r, s16 g, s16 b, s16 a, u16 origin) {
    s16 dropShadowAlpha = a;

    if (a > 100) {
        dropShadowAlpha = 100;
    }

    gDPPipeSync(gfx++);
    gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, dropShadowAlpha);
    gEXTextureRectangle(gfx++, origin, origin, (rectLeft + 2) * 4, (rectTop + 2) * 4, (rectLeft + rectWidth + 2) * 4,
                        (rectTop + rectHeight + 2) * 4, G_TX_RENDERTILE, 0, 0, dsdx, dtdy);

    gDPPipeSync(gfx++);
    gDPSetPrimColor(gfx++, 0, 0, r, g, b, a);

    gEXTextureRectangle(gfx++, origin, origin,  rectLeft * 4, rectTop * 4, (rectLeft + rectWidth) * 4, (rectTop + rectHeight) * 4,
                        G_TX_RENDERTILE, 0, 0, dsdx, dtdy);

    return gfx;
}

bool autosave_was_ready = false;

s32 recomp_autosave_debug_enabled() {
    return 1;
}

void draw_autosave_icon(PlayState* play) {
    s32 alpha = 0;
    if (autosave_icon_counter > (AUTOSAVE_ICON_SHOW_FRAMES + AUTOSAVE_ICON_FADE_OUT_FRAMES)) {
        alpha = (255 * (AUTOSAVE_ICON_FADE_IN_FRAMES - (autosave_icon_counter - AUTOSAVE_ICON_SHOW_FRAMES - AUTOSAVE_ICON_FADE_OUT_FRAMES))) / AUTOSAVE_ICON_FADE_IN_FRAMES;
    }
    else if (autosave_icon_counter > AUTOSAVE_ICON_FADE_OUT_FRAMES) {
        alpha = 255;
    }
    else if (autosave_icon_counter > 0) {
        alpha = (255 * autosave_icon_counter) / AUTOSAVE_ICON_FADE_OUT_FRAMES;
    }

    if (autosave_icon_counter > 0) {
        autosave_icon_counter--;
    }

    OPEN_DISPS(play->state.gfxCtx);

    if (alpha != 0) {
        gEXForceUpscale2D(OVERLAY_DISP++, 1);
        gDPLoadTextureBlock(OVERLAY_DISP++, autosave_icon, G_IM_FMT_RGBA, G_IM_SIZ_32b, AUTOSAVE_ICON_WIDTH, AUTOSAVE_ICON_HEIGHT, 0,
            G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

        gDPSetCombineMode(OVERLAY_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        OVERLAY_DISP = GfxEx_DrawRect_DropShadow(OVERLAY_DISP, AUTOSAVE_ICON_X - SCREEN_WIDTH, AUTOSAVE_ICON_Y, AUTOSAVE_ICON_DRAW_WIDTH, AUTOSAVE_ICON_DRAW_HEIGHT,
            (s32)(1024.0f * AUTOSAVE_ICON_WIDTH / AUTOSAVE_ICON_DRAW_WIDTH), (s32)(1024.0f * AUTOSAVE_ICON_HEIGHT / AUTOSAVE_ICON_DRAW_HEIGHT),
            255, 255, 255, alpha, G_EX_ORIGIN_RIGHT);
        gEXForceUpscale2D(OVERLAY_DISP++, 0);
    }
        
    if (recomp_autosave_debug_enabled() && autosave_was_ready) {
        gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 0, 0, 255);
        gDPSetCombineLERP(OVERLAY_DISP++, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE);
        gEXFillRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, 0, SCREEN_HEIGHT - 10, 20, SCREEN_HEIGHT);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

void show_autosave_icon() {
    autosave_icon_counter = AUTOSAVE_ICON_TOTAL_FRAMES;
}

s32 recomp_autosave_enabled() {
    return 1;
}

u32 recomp_autosave_interval() {
    return 2 * 60 * 1000;
}

#define MIN_FRAMES_SINCE_CHANGED 10
#define MIN_FRAMES_SINCE_READY 20
OSTime last_autosave_time = 0;

bool reached_final_three_hours() {
    // Logic copied with modifications from Interface_DrawClock.
    if ((CURRENT_DAY >= 4) ||
        ((CURRENT_DAY == 3) && (CURRENT_TIME >= CLOCK_TIME(3, 0)) && (CURRENT_TIME < CLOCK_TIME(6, 0)))
    ) {
        return true;
    }
    return false;
}

void autosave_post_play_update(PlayState* play) {
    static int frames_since_save_changed = 0;
    static int frames_since_autosave_ready = 0;
    if (recomp_autosave_enabled()) {
        if (autosave_compare_saves(&gSaveContext, &prev_save_ctx)) {
            frames_since_save_changed = 0;
            Lib_MemCpy(&prev_save_ctx, &gSaveContext, offsetof(SaveContext, fileNum));
        }
        else {
            frames_since_save_changed++;
        }

        OSTime time_now = osGetTime();

        // Check the following conditions:
        // * The UI is in a normal state.
        // * Time is passing.
        // * No message is on screen.
        // * The game is not paused.
        // * No cutscene is running.
        // * The game is not in cutscene mode.
        // * The clock has not reached the final 3 hours.
        if (gSaveContext.hudVisibility == HUD_VISIBILITY_ALL &&
            R_TIME_SPEED != 0 &&
            !Environment_IsTimeStopped() && 
            play->msgCtx.msgMode == MSGMODE_NONE &&
            play->pauseCtx.state == PAUSE_STATE_OFF &&
            gSaveContext.save.cutsceneIndex < 0xFFF0 &&
            !Play_InCsMode(play) &&
            !reached_final_three_hours()
        ) {
            frames_since_autosave_ready++;
            autosave_was_ready = true;
        }
        else {
            frames_since_autosave_ready = 0;
            autosave_was_ready = false;
        }

        // Check if the frame count thresholds for the save data remaining unchanged and autosaving being ready have both been met
        // and that enough time has passed since the previous autosave to create a new one.
        if (frames_since_save_changed >= MIN_FRAMES_SINCE_CHANGED &&
            frames_since_autosave_ready >= MIN_FRAMES_SINCE_READY &&
            OS_CYCLES_TO_USEC(time_now - last_autosave_time) > (1000 * recomp_autosave_interval())
        ) {
            last_autosave_time = time_now;
            do_autosave(&play->sramCtx);
            show_autosave_icon();
        }
    }
    else {
        // Update the last autosave time to the current time to prevent autosaving immediately if autosaves are turned back on. 
        last_autosave_time = osGetTime();
    }
}

void autosave_init() {
    last_autosave_time = osGetTime();
    Lib_MemCpy(&prev_save_ctx, &gSaveContext, offsetof(SaveContext, fileNum));
}

extern s32 gFlashSaveSizes[];
extern u16 D_801C6A58[];

#define CHECK_NEWF(newf)                                                                                 \
    ((newf)[0] != 'Z' || (newf)[1] != 'E' || (newf)[2] != 'L' || (newf)[3] != 'D' || (newf)[4] != 'A' || \
     (newf)[5] != '3')

// @recomp Patched to change the entrance for autosaves and initialize autosaves.
void Sram_OpenSave(FileSelectState* fileSelect, SramContext* sramCtx) {
    s32 i;
    s32 pad;
    s32 phi_t1;
    s32 pad1;
    s32 fileNum;

    if (gSaveContext.flashSaveAvailable) {
        bzero(sramCtx->saveBuf, SAVE_BUFFER_SIZE);

        if (gSaveContext.fileNum == 0xFF) {
            SysFlashrom_ReadData(sramCtx->saveBuf, gFlashSaveStartPages[0], gFlashSaveNumPages[0]);
        } else if (fileSelect->isOwlSave[gSaveContext.fileNum + 2]) {
            phi_t1 = gSaveContext.fileNum + 2;
            phi_t1 *= 2;

            if (SysFlashrom_ReadData(sramCtx->saveBuf, gFlashSaveStartPages[phi_t1], gFlashSaveNumPages[phi_t1]) != 0) {
                SysFlashrom_ReadData(sramCtx->saveBuf, gFlashSaveStartPages[phi_t1 + 1],
                                     gFlashSaveNumPages[phi_t1 + 1]);
            }
        } else {
            phi_t1 = gSaveContext.fileNum;
            phi_t1 *= 2;

            if (SysFlashrom_ReadData(sramCtx->saveBuf, gFlashSaveStartPages[phi_t1], gFlashSaveNumPages[phi_t1]) != 0) {
                SysFlashrom_ReadData(sramCtx->saveBuf, gFlashSaveStartPages[phi_t1 + 1],
                                     gFlashSaveNumPages[phi_t1 + 1]);
            }
        }

        Lib_MemCpy(&gSaveContext, sramCtx->saveBuf, gFlashSaveSizes[phi_t1]);

        if (CHECK_NEWF(gSaveContext.save.saveInfo.playerData.newf)) {
            SysFlashrom_ReadData(sramCtx->saveBuf, gFlashSaveStartPages[phi_t1 + 1], gFlashSaveNumPages[phi_t1 + 1]);
            Lib_MemCpy(&gSaveContext, sramCtx->saveBuf, gFlashSaveSizes[phi_t1]);
        }
    }

    gSaveContext.save.saveInfo.playerData.magicLevel = 0;

    if (!gSaveContext.save.isOwlSave) {
        for (i = 0; i < ARRAY_COUNT(gSaveContext.eventInf); i++) {
            gSaveContext.eventInf[i] = 0;
        }

        for (i = 0; i < ARRAY_COUNT(gSaveContext.cycleSceneFlags); i++) {
            gSaveContext.cycleSceneFlags[i].chest = gSaveContext.save.saveInfo.permanentSceneFlags[i].chest;
            gSaveContext.cycleSceneFlags[i].switch0 = gSaveContext.save.saveInfo.permanentSceneFlags[i].switch0;
            gSaveContext.cycleSceneFlags[i].switch1 = gSaveContext.save.saveInfo.permanentSceneFlags[i].switch1;
            gSaveContext.cycleSceneFlags[i].clearedRoom = gSaveContext.save.saveInfo.permanentSceneFlags[i].clearedRoom;
            gSaveContext.cycleSceneFlags[i].collectible = gSaveContext.save.saveInfo.permanentSceneFlags[i].collectible;
        }

        for (i = 0; i < TIMER_ID_MAX; i++) {
            gSaveContext.timerStates[i] = TIMER_STATE_OFF;
            gSaveContext.timerCurTimes[i] = SECONDS_TO_TIMER(0);
            gSaveContext.timerTimeLimits[i] = SECONDS_TO_TIMER(0);
            gSaveContext.timerStartOsTimes[i] = 0;
            gSaveContext.timerStopTimes[i] = SECONDS_TO_TIMER(0);
            gSaveContext.timerPausedOsTimes[i] = 0;
        }

        if (gSaveContext.save.isFirstCycle) {
            gSaveContext.save.entrance = ENTRANCE(SOUTH_CLOCK_TOWN, 0);
            gSaveContext.save.day = 0;
            gSaveContext.save.time = CLOCK_TIME(6, 0) - 1;
        } else {
            gSaveContext.save.entrance = ENTRANCE(CUTSCENE, 0);
            gSaveContext.nextCutsceneIndex = 0;
            gSaveContext.save.playerForm = PLAYER_FORM_HUMAN;
        }
    }
    // @recomp Handle autosaves.
    else if (gSaveContext.save.isOwlSave == SAVE_TYPE_AUTOSAVE) {
        gSaveContext.save.entrance = ENTRANCE(SOUTH_CLOCK_TOWN, 0);

        for (i = 0; i < ARRAY_COUNT(gSaveContext.cycleSceneFlags); i++) {
            gSaveContext.cycleSceneFlags[i].chest = gSaveContext.save.saveInfo.permanentSceneFlags[i].chest;
            gSaveContext.cycleSceneFlags[i].switch0 = gSaveContext.save.saveInfo.permanentSceneFlags[i].switch0;
            gSaveContext.cycleSceneFlags[i].switch1 = gSaveContext.save.saveInfo.permanentSceneFlags[i].switch1;
            gSaveContext.cycleSceneFlags[i].clearedRoom = gSaveContext.save.saveInfo.permanentSceneFlags[i].clearedRoom;
            gSaveContext.cycleSceneFlags[i].collectible = gSaveContext.save.saveInfo.permanentSceneFlags[i].collectible;
        }

        if (gSaveContext.save.saveInfo.scarecrowSpawnSongSet) {
            Lib_MemCpy(gScarecrowSpawnSongPtr, gSaveContext.save.saveInfo.scarecrowSpawnSong,
                       sizeof(gSaveContext.save.saveInfo.scarecrowSpawnSong));

            for (i = 0; i != ARRAY_COUNT(gSaveContext.save.saveInfo.scarecrowSpawnSong); i++) {}
        }

        fileNum = gSaveContext.fileNum;
        func_80147314(sramCtx, fileNum);
    }
    else {
        gSaveContext.save.entrance = D_801C6A58[(void)0, gSaveContext.save.owlWarpId];
        if ((gSaveContext.save.entrance == ENTRANCE(SOUTHERN_SWAMP_POISONED, 10)) &&
            CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE)) {
            gSaveContext.save.entrance = ENTRANCE(SOUTHERN_SWAMP_CLEARED, 10);
        } else if ((gSaveContext.save.entrance == ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 8)) &&
                   CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE)) {
            gSaveContext.save.entrance = ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 8);
        }

        for (i = 0; i < ARRAY_COUNT(gSaveContext.cycleSceneFlags); i++) {
            gSaveContext.cycleSceneFlags[i].chest = gSaveContext.save.saveInfo.permanentSceneFlags[i].chest;
            gSaveContext.cycleSceneFlags[i].switch0 = gSaveContext.save.saveInfo.permanentSceneFlags[i].switch0;
            gSaveContext.cycleSceneFlags[i].switch1 = gSaveContext.save.saveInfo.permanentSceneFlags[i].switch1;
            gSaveContext.cycleSceneFlags[i].clearedRoom = gSaveContext.save.saveInfo.permanentSceneFlags[i].clearedRoom;
            gSaveContext.cycleSceneFlags[i].collectible = gSaveContext.save.saveInfo.permanentSceneFlags[i].collectible;
        }

        if (gSaveContext.save.saveInfo.scarecrowSpawnSongSet) {
            Lib_MemCpy(gScarecrowSpawnSongPtr, gSaveContext.save.saveInfo.scarecrowSpawnSong,
                       sizeof(gSaveContext.save.saveInfo.scarecrowSpawnSong));

            for (i = 0; i != ARRAY_COUNT(gSaveContext.save.saveInfo.scarecrowSpawnSong); i++) {}
        }

        fileNum = gSaveContext.fileNum;
        func_80147314(sramCtx, fileNum);
    }

    // @recomp Initialize the autosave state tracking.
    autosave_init();
}
