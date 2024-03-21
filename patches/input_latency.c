#include "patches.h"
#include "sys_cfb.h"
#include "buffers.h"
#include "fault.h"

void recomp_set_current_frame_poll_id();
void PadMgr_HandleRetrace(void);
void PadMgr_LockPadData(void);
void PadMgr_UnlockPadData(void);

void PadMgr_ThreadEntry() {
    // @recomp Controller polling was moved to the main thread, so there's nothing to do here.
}

// @recomp Patched to do the actual input polling.
void PadMgr_GetInput(Input* inputs, s32 gameRequest) {
    // @recomp Do an actual poll if gameRequest is true.
    if (gameRequest) {
        PadMgr_HandleRetrace();
        // @recomp Tag the current frame's input polling id for latency tracking.
        recomp_set_current_frame_poll_id();
    }
    PadMgr_LockPadData();
    PadMgr_GetInputNoLock(inputs, gameRequest);
    PadMgr_UnlockPadData();
}

// @recomp Just call PadMgr_GetInput.
void PadMgr_GetInput2(Input* inputs, s32 gameRequest) {
    PadMgr_GetInput(inputs, gameRequest);
}

extern CfbInfo sGraphCfbInfos[3];

// @recomp Immediately sends the graphics task instead of queueing it in the scheduler.
void Graph_TaskSet00(GraphicsContext* gfxCtx, GameState* gameState) {
    static s32 retryCount = 10;
    static s32 cfbIdx = 0;
    OSTask_t* task = &gfxCtx->task.list.t;
    OSScTask* scTask = &gfxCtx->task;
    OSTimer timer;
    OSMesg msg;
    CfbInfo* cfb;

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
    
    // @recomp Manually wait the required number of VI periods after submitting the task
    // so that the next frame doesn't need to wait before submitting its task.
    static IrqMgrClient irq_client = {0};
    static OSMesgQueue vi_queue = {0};
    static OSMesg vi_buf[8] = {0};
    static bool created = false;

    // Create the message queue and install the VI irq manager
    if (!created) {
        created = true;
        osCreateMesgQueue(&vi_queue, vi_buf, ARRAY_COUNT(vi_buf));
        extern IrqMgr gIrqMgr;
        IrqMgr_AddClient(&gIrqMgr, &irq_client, &vi_queue);
    }

    for (int i = 0; i < cfb->updateRate; i++) {
        osRecvMesg(&vi_queue, NULL, OS_MESG_BLOCK);
    }
}
