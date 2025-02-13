#include "patches.h"
#include "sys_cfb.h"
#include "buffers.h"
#include "fault.h"
#include "audiomgr.h"
#include "z64speed_meter.h"
#include "z64vimode.h"
#include "z64viscvg.h"
#include "z64vismono.h"
#include "z64viszbuf.h"
#include "input.h"

void recomp_set_current_frame_poll_id();
void PadMgr_HandleRetrace(void);
void PadMgr_LockPadData(void);
void PadMgr_UnlockPadData(void);
void PadMgr_UpdateRumble(void);
void PadMgr_UpdateConnections(void);
void PadMgr_UpdateInputs(void);
void PadMgr_InitVoice(void);
OSMesgQueue* PadMgr_AcquireSerialEventQueue(void);
void PadMgr_ReleaseSerialEventQueue(OSMesgQueue* serialEventQueue);


extern PadMgr* sPadMgrInstance;
extern s32 sPadMgrRetraceCount;
extern FaultMgr gFaultMgr;
extern s32 sVoiceInitStatus;


typedef enum {
    /* 0 */ VOICE_INIT_FAILED, // voice initialization failed
    /* 1 */ VOICE_INIT_TRY,    // try to initialize voice
    /* 2 */ VOICE_INIT_SUCCESS // voice initialized
} VoiceInitStatus;

RECOMP_PATCH void PadMgr_HandleRetrace(void) {
    // Execute rumble callback
    if (sPadMgrInstance->rumbleRetraceCallback != NULL) {
        sPadMgrInstance->rumbleRetraceCallback(sPadMgrInstance->rumbleRetraceArg);
    }

    // Try and initialize a Voice Recognition Unit if not already attempted
    if (sVoiceInitStatus != VOICE_INIT_FAILED) {
        PadMgr_InitVoice();
    }

    // Rumble Pak
    if (gFaultMgr.msgId != 0) {
        // If fault is active, no rumble
        PadMgr_RumbleStop();
    } else if (sPadMgrInstance->rumbleOffTimer > 0) {
        // If the rumble off timer is active, no rumble
        --sPadMgrInstance->rumbleOffTimer;
        PadMgr_RumbleStop();
    } else if (sPadMgrInstance->rumbleOnTimer == 0) {
        // If the rumble on timer is inactive, no rumble
        PadMgr_RumbleStop();
    } else if (!sPadMgrInstance->isResetting) {
        // If not resetting, update rumble
        PadMgr_UpdateRumble();
        --sPadMgrInstance->rumbleOnTimer;
    }
}

extern u8 sOcarinaInstrumentId;

void poll_inputs(void) {
    OSMesgQueue* serialEventQueue = PadMgr_AcquireSerialEventQueue();
    // Begin reading controller data
    osContStartReadData(serialEventQueue);

    bool needs_right_stick = recomp_get_analog_cam_enabled() || recomp_aiming_override_mode == RECOMP_AIMING_OVERRIDE_FORCE_RIGHT_STICK;
    // Suppress the right analog stick if analog camera is active unless the ocarina is in use.
    recomp_set_right_analog_suppressed(needs_right_stick && sOcarinaInstrumentId == OCARINA_INSTRUMENT_OFF);
    // Resets this flag for the next frame;
    recomp_aiming_override_mode = RECOMP_AIMING_OVERRIDE_OFF;

    // Wait for controller data
    osRecvMesg(serialEventQueue, NULL, OS_MESG_BLOCK);
    osContGetReadData(sPadMgrInstance->pads);

    // Clear all but controller 1
    bzero(&sPadMgrInstance->pads[1], sizeof(*sPadMgrInstance->pads) * (MAXCONTROLLERS - 1));

    // If in PreNMI, clear all controllers
    if (sPadMgrInstance->isResetting) {
        bzero(sPadMgrInstance->pads, sizeof(sPadMgrInstance->pads));
    }

    // Query controller statuses
    osContStartQuery(serialEventQueue);
    osRecvMesg(serialEventQueue, NULL, OS_MESG_BLOCK);
    osContGetQuery(sPadMgrInstance->padStatus);

    // Lock serial message queue
    PadMgr_ReleaseSerialEventQueue(serialEventQueue);

    // Update connections
    PadMgr_UpdateConnections();

    // Lock input data
    PadMgr_LockPadData();

    // Update input data
    PadMgr_UpdateInputs();

    // Execute input callback
    if (sPadMgrInstance->inputRetraceCallback != NULL) {
        sPadMgrInstance->inputRetraceCallback(sPadMgrInstance->inputRetraceArg);
    }

    // Unlock input data
    PadMgr_UnlockPadData();
    sPadMgrRetraceCount++;
}

// @recomp Patched to do the actual input polling.
RECOMP_PATCH void PadMgr_GetInput(Input* inputs, s32 gameRequest) {
    // @recomp Do an actual poll if gameRequest is true.
    if (gameRequest) {
        poll_inputs();
        // @recomp Tag the current frame's input polling id for latency tracking.
        recomp_set_current_frame_poll_id();
    }
    PadMgr_LockPadData();
    PadMgr_GetInputNoLock(inputs, gameRequest);
    PadMgr_UnlockPadData();
}

// @recomp Just call PadMgr_GetInput.
RECOMP_PATCH void PadMgr_GetInput2(Input* inputs, s32 gameRequest) {
    PadMgr_GetInput(inputs, gameRequest);
}

extern CfbInfo sGraphCfbInfos[3];
u32 recomp_time_us();
void recomp_measure_latency();
void* osViGetCurrentFramebuffer_recomp();

OSMesgQueue *rdp_queue_ptr = NULL;

// @recomp Immediately sends the graphics task instead of queueing it in the scheduler.
RECOMP_PATCH void Graph_TaskSet00(GraphicsContext* gfxCtx, GameState* gameState) {
    static s32 retryCount = 10;
    static s32 cfbIdx = 0;
    OSTask_t* task = &gfxCtx->task.list.t;
    OSScTask* scTask = &gfxCtx->task;
    OSTimer timer;
    OSMesg msg;
    CfbInfo* cfb;
    
    // @recomp Additional static members for extra scheduling purposes.
    static IrqMgrClient irq_client = {0};
    static OSMesgQueue vi_queue = {0};
    static OSMesg vi_buf[8] = {0};
    static bool created = false;
    if (!created) {
        created = true;
        osCreateMesgQueue(&vi_queue, vi_buf, ARRAY_COUNT(vi_buf));
        extern IrqMgr gIrqMgr;
        IrqMgr_AddClient(&gIrqMgr, &irq_client, &vi_queue);
    }

    // @recomp Disable the wait here so that it can be moved after task submission for minimizing latency.
// retry:
//     osSetTimer(&timer, OS_USEC_TO_CYCLES(3 * 1000 * 1000), 0, &gfxCtx->queue, (OSMesg)666);
//     osRecvMesg(&gfxCtx->queue, &msg, OS_MESG_BLOCK);
//     osStopTimer(&timer);

//     if (msg == (OSMesg)666) {
//         osSyncPrintf("GRAPH SP TIMEOUT\n");
//         if (retryCount >= 0) {
//             retryCount--;
//             Sched_SendGfxCancelMsg(&gSchedContext);
//             goto retry;
//         } else {
//             // graph.c: No more! die!
//             osSyncPrintf("graph.c:もうダメ！死ぬ！\n");
//             Fault_AddHungupAndCrashImpl("RCP is HUNG UP!!", "Oh! MY GOD!!");
//         }
//     }

    gfxCtx->masterList = gGfxMasterDL;
    if (gfxCtx->callback != NULL) {
        gfxCtx->callback(gfxCtx, gfxCtx->callbackArg);
    }

    task->type = M_GFXTASK;
    task->flags = OS_SC_DRAM_DLIST;
    task->ucodeBoot = SysUcode_GetUCodeBoot();
    task->ucodeBootSize = SysUcode_GetUCodeBootSize();
    task->ucode = SysUcode_GetUCode();
    task->ucodeData = SysUcode_GetUCodeData();
    task->ucodeSize = SP_UCODE_SIZE;
    task->ucodeDataSize = SP_UCODE_DATA_SIZE;
    task->dramStack = (u64*)gGfxSPTaskStack;
    task->dramStackSize = sizeof(gGfxSPTaskStack);
    task->outputBuff = gGfxSPTaskOutputBufferPtr;
    task->outputBuffSize = gGfxSPTaskOutputBufferEnd;
    task->dataPtr = (u64*)gGfxMasterDL;
    task->dataSize = 0;
    task->yieldDataPtr = (u64*)gGfxSPTaskYieldBuffer;
    task->yieldDataSize = sizeof(gGfxSPTaskYieldBuffer);

    scTask->next = NULL;
    scTask->flags = OS_SC_RCP_MASK | OS_SC_SWAPBUFFER | OS_SC_LAST_TASK;

    if (SREG(33) & 1) {
        SREG(33) &= ~1;
        scTask->flags &= ~OS_SC_SWAPBUFFER;
        gfxCtx->framebufferIndex--;
    }

    scTask->msgQ = &gfxCtx->queue;
    scTask->msg = NULL;

    { s32 pad; }

    cfb = &sGraphCfbInfos[cfbIdx];
    cfbIdx = (cfbIdx + 1) % ARRAY_COUNT(sGraphCfbInfos);

    cfb->fb1 = gfxCtx->curFrameBuffer;
    cfb->swapBuffer = gfxCtx->curFrameBuffer;

    if (gfxCtx->updateViMode) {
        gfxCtx->updateViMode = false;
        cfb->viMode = gfxCtx->viMode;
        cfb->features = gfxCtx->viConfigFeatures;
        cfb->xScale = gfxCtx->xScale;
        cfb->yScale = gfxCtx->yScale;
    } else {
        cfb->viMode = NULL;
    }
    cfb->unk_10 = 0;
    cfb->updateRate = gameState->framerateDivisor;

    scTask->framebuffer = cfb;

    while (gfxCtx->queue.validCount != 0) {
        osRecvMesg(&gfxCtx->queue, NULL, OS_MESG_NOBLOCK);
    }

    gfxCtx->schedMsgQ = &gSchedContext.cmdQ;
    osSendMesg(&gSchedContext.cmdQ, scTask, OS_MESG_BLOCK);
    Sched_SendEntryMsg(&gSchedContext);
    
    // @recomp Immediately wait on the task to complete to minimize latency for the next one.
    osRecvMesg(&gfxCtx->queue, &msg, OS_MESG_BLOCK);

    // @recomp Wait on the VI framebuffer to change if this task has a framebuffer swap.
    if (scTask->flags & OS_SC_SWAPBUFFER) {
        int viCounter = 0;
        while (osViGetCurrentFramebuffer() != cfb->fb1) {
            osRecvMesg(&vi_queue, NULL, OS_MESG_BLOCK);
            viCounter++;
        }
        
        // If we didn't wait the full number of VIs needed between frames then wait one extra VI afterwards.
        if (viCounter < gameState->framerateDivisor) {
            osRecvMesg(&vi_queue, NULL, OS_MESG_BLOCK);
        }
    }
    
    // @recomp Flush any extra messages from the VI queue.
    while (osRecvMesg(&vi_queue, NULL, OS_MESG_NOBLOCK) == 0) {
        ;
    }
}

extern SpeedMeter sGameSpeedMeter;
extern VisCvg sGameVisCvg;
extern VisZbuf sGameVisZbuf;
extern VisMono sGameVisMono;
extern ViMode sGameViMode;

RECOMP_PATCH void GameState_Destroy(GameState* gameState) {
    AudioMgr_StopAllSfxExceptSystem();
    Audio_Update();

    // @recomp The wait for the gfx task was moved to directly after submission, so it's not needed here.
    // osRecvMesg(&gameState->gfxCtx->queue, NULL, OS_MESG_BLOCK);

    if (gameState->destroy != NULL) {
        gameState->destroy(gameState);
    }

    Rumble_Destroy();
    SpeedMeter_Destroy(&sGameSpeedMeter);
    VisCvg_Destroy(&sGameVisCvg);
    VisZbuf_Destroy(&sGameVisZbuf);
    VisMono_Destroy(&sGameVisMono);
    ViMode_Destroy(&sGameViMode);
    THA_Destroy(&gameState->tha);
    GameAlloc_Cleanup(&gameState->alloc);
}
