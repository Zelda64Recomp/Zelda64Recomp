#include "patches.h"
/*
// Infinite magic
s32 Magic_Consume(PlayState* play, s16 magicToConsume, s16 type) {
   InterfaceContext* interfaceCtx = &play->interfaceCtx;

   magicToConsume = 0;

   // // Magic is not acquired yet
   // if (!gSaveContext.save.saveInfo.playerData.isMagicAcquired) {
   //     return false;
   // }

   // Not enough magic available to consume
   if ((gSaveContext.save.saveInfo.playerData.magic - magicToConsume) < 0) {
       if (gSaveContext.magicCapacity != 0) {
           Audio_PlaySfx(NA_SE_SY_ERROR);
       }
       return false;
   }

   switch (type) {
       case MAGIC_CONSUME_NOW:
       case MAGIC_CONSUME_NOW_ALT:
           // Drain magic immediately e.g. Deku Bubble
           if ((gSaveContext.magicState == MAGIC_STATE_IDLE) ||
               (gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS)) {
               if (gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS) {
                   play->actorCtx.lensActive = false;
               }
               if (CHECK_WEEKEVENTREG(WEEKEVENTREG_DRANK_CHATEAU_ROMANI)) {
                   magicToConsume = 0;
               }
               gSaveContext.magicToConsume = magicToConsume;
               gSaveContext.magicState = MAGIC_STATE_CONSUME_SETUP;
               return true;
           } else {
               Audio_PlaySfx(NA_SE_SY_ERROR);
               return false;
           }

       case MAGIC_CONSUME_WAIT_NO_PREVIEW:
           // Sets consume target but waits to consume.
           // No yellow magic to preview target consumption.
           if ((gSaveContext.magicState == MAGIC_STATE_IDLE) ||
               (gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS)) {
               if (gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS) {
                   play->actorCtx.lensActive = false;
               }
               if (CHECK_WEEKEVENTREG(WEEKEVENTREG_DRANK_CHATEAU_ROMANI)) {
                   magicToConsume = 0;
               }
               gSaveContext.magicToConsume = magicToConsume;
               gSaveContext.magicState = MAGIC_STATE_METER_FLASH_3;
               return true;
           } else {
               Audio_PlaySfx(NA_SE_SY_ERROR);
               return false;
           }

       case MAGIC_CONSUME_LENS:
           if (gSaveContext.magicState == MAGIC_STATE_IDLE) {
               if (gSaveContext.save.saveInfo.playerData.magic != 0) {
                   interfaceCtx->magicConsumptionTimer = 80;
                   gSaveContext.magicState = MAGIC_STATE_CONSUME_LENS;
                   return true;
               } else {
                   return false;
               }
           } else if (gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS) {
               return true;
           } else {
               return false;
           }

       case MAGIC_CONSUME_WAIT_PREVIEW:
           // Sets consume target but waits to consume.
           // Preview consumption with a yellow bar. e.g. Spin Attack
           if ((gSaveContext.magicState == MAGIC_STATE_IDLE) ||
               (gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS)) {
               if (gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS) {
                   play->actorCtx.lensActive = false;
               }
               gSaveContext.magicToConsume = magicToConsume;
               gSaveContext.magicState = MAGIC_STATE_METER_FLASH_2;
               return true;
           } else {
               Audio_PlaySfx(NA_SE_SY_ERROR);
               return false;
           }

       case MAGIC_CONSUME_GORON_ZORA:
           // Goron spiked rolling or Zora electric barrier
           if (gSaveContext.save.saveInfo.playerData.magic != 0) {
               interfaceCtx->magicConsumptionTimer = 10;
               gSaveContext.magicState = MAGIC_STATE_CONSUME_GORON_ZORA_SETUP;
               return true;
           } else {
               return false;
           }

       case MAGIC_CONSUME_GIANTS_MASK:
           // Wearing Giant's Mask
           if (gSaveContext.magicState == MAGIC_STATE_IDLE) {
               if (gSaveContext.save.saveInfo.playerData.magic != 0) {
                   interfaceCtx->magicConsumptionTimer = R_MAGIC_CONSUME_TIMER_GIANTS_MASK;
                   gSaveContext.magicState = MAGIC_STATE_CONSUME_GIANTS_MASK;
                   return true;
               } else {
                   return false;
               }
           }
           if (gSaveContext.magicState == MAGIC_STATE_CONSUME_GIANTS_MASK) {
               return true;
           } else {
               return false;
           }

       case MAGIC_CONSUME_DEITY_BEAM:
           // Consumes magic immediately
           if ((gSaveContext.magicState == MAGIC_STATE_IDLE) ||
               (gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS)) {
               if (gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS) {
                   play->actorCtx.lensActive = false;
               }
               if (CHECK_WEEKEVENTREG(WEEKEVENTREG_DRANK_CHATEAU_ROMANI)) {
                   magicToConsume = 0;
               }
               gSaveContext.save.saveInfo.playerData.magic -= magicToConsume;
               return true;
           } else {
               Audio_PlaySfx(NA_SE_SY_ERROR);
               return false;
           }
   }

   return false;
}
*/