#include "patches.h"
#include "sys_flashrom.h"
#include "PR/os_internal_flash.h"
#include "fault.h"

extern OSMesgQueue sFlashromMesgQueue;
s32 SysFlashrom_IsInit(void);
void Sleep_Msec(u32 ms);

// @recomp Patched to not wait a hardcoded amount of time for the save to complete.
void Sram_UpdateWriteToFlashDefault(SramContext* sramCtx) {
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
    } else if (sramCtx->status == 4) {
        // @recomp Patched to check status instead of using a hardcoded wait.
        recomp_printf("Status 4\n");
        sramCtx->status = 0;
    }
}

// @recomp Patched to not wait a hardcoded amount of time for the save to complete.
void Sram_UpdateWriteToFlashOwlSave(SramContext* sramCtx) {
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
    } else if (sramCtx->status == 4) {
        // @recomp Patched to check status instead of using a hardcoded wait.
        sramCtx->status = 0;
        bzero(sramCtx->saveBuf, SAVE_BUFFER_SIZE);
        gSaveContext.save.isOwlSave = false;
        gSaveContext.save.saveInfo.checksum = 0;
        // flash read to buffer then copy to save context
        SysFlashrom_ReadData(sramCtx->saveBuf, sramCtx->curPage, sramCtx->numPages);
        Lib_MemCpy(&gSaveContext, sramCtx->saveBuf, offsetof(SaveContext, fileNum));
    }
}

// @recomp Patched to add a pause so that other threads can execute in the meantime.
s32 SysFlashrom_ExecWrite(void* addr, u32 pageNum, u32 pageCount) {
    OSIoMesg msg;
    s32 result;
    u32 i;

    if (!SysFlashrom_IsInit()) {
        return -1;
    }
    // Ensure the page is always aligned to a sector boundary.
    if ((pageNum % FLASH_BLOCK_SIZE) != 0) {
        Fault_AddHungupAndCrash("../sys_flashrom.c", 275);
    }
    osWritebackDCache(addr, pageCount * FLASH_BLOCK_SIZE);
    for (i = 0; i < pageCount; i++) {
        // @recomp Pause shortly to allow other threads to work.
        Sleep_Msec(5);
        osFlashWriteBuffer(&msg, OS_MESG_PRI_NORMAL, (u8*)addr + i * FLASH_BLOCK_SIZE, &sFlashromMesgQueue);
        osRecvMesg(&sFlashromMesgQueue, NULL, OS_MESG_BLOCK);
        result = osFlashWriteArray(i + pageNum);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}
