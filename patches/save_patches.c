#include "patches.h"
#include "sys_flashrom.h"
#include "PR/os_internal_flash.h"
#include "fault.h"
#include "overlays/gamestates/ovl_file_choose/z_file_select.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"

extern OSMesgQueue sFlashromMesgQueue;
s32 SysFlashrom_IsInit(void);
void Sleep_Msec(u32 ms);
extern u16 D_801F6AF0;
extern u8 D_801F6AF2;

// @recomp Patched to wait a much shorter amount of time for the save to complete.
RECOMP_PATCH void Sram_UpdateWriteToFlashDefault(SramContext* sramCtx) {
    if (sramCtx->status == 2) {
        if (SysFlashrom_IsBusy() != 0) {          // if task running
            if (SysFlashrom_AwaitResult() == 0) { // wait for task done
                // task success
                sramCtx->status = 4;
            } else {
                // task failure
                sramCtx->status = 4;
            }
        }
    } else if (OSTIME_TO_TIMER(osGetTime() - sramCtx->startWriteOsTime) >= SECONDS_TO_TIMER_PRECISE(0, 25)) {
        // @recomp Patched to wait a much shorter amount of time.
        sramCtx->status = 0;
    }
}

// @recomp Patched to wait a much shorter amount of time for the save to complete.
RECOMP_PATCH void Sram_UpdateWriteToFlashOwlSave(SramContext* sramCtx) {
    if (sramCtx->status == 7) {
        if (SysFlashrom_IsBusy() != 0) {          // Is task running
            if (SysFlashrom_AwaitResult() == 0) { // Wait for task done
                SysFlashrom_WriteDataAsync(sramCtx->saveBuf, sramCtx->curPage + 0x80, sramCtx->numPages);
                sramCtx->status = 8;
            } else {
                SysFlashrom_WriteDataAsync(sramCtx->saveBuf, sramCtx->curPage + 0x80, sramCtx->numPages);
                sramCtx->status = 8;
            }
        }
    } else if (sramCtx->status == 8) {
        if (SysFlashrom_IsBusy() != 0) {          // Is task running
            if (SysFlashrom_AwaitResult() == 0) { // Wait for task done
                sramCtx->status = 4;
            } else {
                sramCtx->status = 4;
            }
        }
    } else if (OSTIME_TO_TIMER(osGetTime() - sramCtx->startWriteOsTime) >= SECONDS_TO_TIMER_PRECISE(0, 25)) {
        // @recomp Patched to wait a much shorter amount of time.
        sramCtx->status = 0;
        bzero(sramCtx->saveBuf, SAVE_BUFFER_SIZE);
        gSaveContext.save.isOwlSave = false;
        gSaveContext.save.saveInfo.checksum = 0;
        // flash read to buffer then copy to save context
        SysFlashrom_ReadData(sramCtx->saveBuf, sramCtx->curPage, sramCtx->numPages);
        Lib_MemCpy(&gSaveContext, sramCtx->saveBuf, offsetof(SaveContext, fileNum));
    }
}

RECOMP_DECLARE_EVENT(recomp_after_init_save(FileSelectState* fileSelect, SramContext* sramCtx));

// @recomp Patched to expose recomp_on_save_init event
RECOMP_PATCH void Sram_InitSave(FileSelectState* fileSelect2, SramContext* sramCtx) {
    s32 phi_v0;
    u16 i;
    FileSelectState* fileSelect = fileSelect2;
    s16 maskCount;

    if (gSaveContext.flashSaveAvailable) {
        Sram_InitNewSave();
        if (fileSelect->buttonIndex == 0) {
            gSaveContext.save.cutsceneIndex = 0xFFF0;
        }

        for (phi_v0 = 0; phi_v0 < ARRAY_COUNT(gSaveContext.save.saveInfo.playerData.playerName); phi_v0++) {
            gSaveContext.save.saveInfo.playerData.playerName[phi_v0] =
                fileSelect->fileNames[fileSelect->buttonIndex][phi_v0];
        }

        gSaveContext.save.saveInfo.playerData.newf[0] = 'Z';
        gSaveContext.save.saveInfo.playerData.newf[1] = 'E';
        gSaveContext.save.saveInfo.playerData.newf[2] = 'L';
        gSaveContext.save.saveInfo.playerData.newf[3] = 'D';
        gSaveContext.save.saveInfo.playerData.newf[4] = 'A';
        gSaveContext.save.saveInfo.playerData.newf[5] = '3';

        recomp_after_init_save(fileSelect, sramCtx);

        gSaveContext.save.saveInfo.checksum = Sram_CalcChecksum(&gSaveContext.save, sizeof(Save));

        Lib_MemCpy(sramCtx->saveBuf, &gSaveContext.save, sizeof(Save));
        Lib_MemCpy(&sramCtx->saveBuf[0x2000], &gSaveContext.save, sizeof(Save));

        for (i = 0; i < ARRAY_COUNT(gSaveContext.save.saveInfo.playerData.newf); i++) {
            fileSelect->newf[fileSelect->buttonIndex][i] = gSaveContext.save.saveInfo.playerData.newf[i];
        }

        fileSelect->threeDayResetCount[fileSelect->buttonIndex] =
            gSaveContext.save.saveInfo.playerData.threeDayResetCount;

        for (i = 0; i < ARRAY_COUNT(gSaveContext.save.saveInfo.playerData.playerName); i++) {
            fileSelect->fileNames[fileSelect->buttonIndex][i] = gSaveContext.save.saveInfo.playerData.playerName[i];
        }

        fileSelect->healthCapacity[fileSelect->buttonIndex] = gSaveContext.save.saveInfo.playerData.healthCapacity;
        fileSelect->health[fileSelect->buttonIndex] = gSaveContext.save.saveInfo.playerData.health;
        fileSelect->defenseHearts[fileSelect->buttonIndex] = gSaveContext.save.saveInfo.inventory.defenseHearts;
        fileSelect->questItems[fileSelect->buttonIndex] = gSaveContext.save.saveInfo.inventory.questItems;
        fileSelect->time[fileSelect->buttonIndex] = CURRENT_TIME;
        fileSelect->day[fileSelect->buttonIndex] = gSaveContext.save.day;
        fileSelect->isOwlSave[fileSelect->buttonIndex] = gSaveContext.save.isOwlSave;
        fileSelect->rupees[fileSelect->buttonIndex] = gSaveContext.save.saveInfo.playerData.rupees;
        fileSelect->walletUpgrades[fileSelect->buttonIndex] = CUR_UPG_VALUE(UPG_WALLET);

        for (i = 0, maskCount = 0; i < MASK_NUM_SLOTS; i++) {
            if (gSaveContext.save.saveInfo.inventory.items[i + ITEM_NUM_SLOTS] != ITEM_NONE) {
                maskCount++;
            }
        }

        fileSelect->maskCount[fileSelect->buttonIndex] = maskCount;
        fileSelect->heartPieceCount[fileSelect->buttonIndex] = GET_QUEST_HEART_PIECE_COUNT;
    }

    gSaveContext.save.time = D_801F6AF0;
    gSaveContext.flashSaveAvailable = D_801F6AF2;
}
