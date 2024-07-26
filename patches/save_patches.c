#include "patches.h"
#include "sys_flashrom.h"
#include "PR/os_internal_flash.h"
#include "fault.h"

extern OSMesgQueue sFlashromMesgQueue;
s32 SysFlashrom_IsInit(void);
void Sleep_Msec(u32 ms);

// @recomp Patched to not wait a hardcoded amount of time for the save to complete.
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
    } else if (sramCtx->status == 4) {
        // @recomp Patched to check status instead of using a hardcoded wait.
        sramCtx->status = 0;
    }
}

// @recomp Patched to not wait a hardcoded amount of time for the save to complete.
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
