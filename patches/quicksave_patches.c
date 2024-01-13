// Quicksaving is disabled for now

/* To reenable it, a few changes to the recompiled functions are also needed:

in Main:
L_80174E84:
    // or          $a0, $s3, $zero
    ctx->r4 = ctx->r19 | 0;
    // addiu       $a1, $sp, 0x38
    ctx->r5 = ADD32(ctx->r29, 0X38);
    // jal         0x80087ED0
    // addiu       $a2, $zero, 0x1
    ctx->r6 = ADD32(0, 0X1);
    osRecvMesg_recomp(rdram, ctx);
    +handle_quicksave_actions(rdram, ctx);

in Graph_ThreadEntry:
L_801749C8:
    +handle_quicksave_actions_main(rdram, ctx);
    +ctx->r4 = ctx->r17 | 0;
    // jal         0x80174868
    // or          $a1, $s0, $zero
    ctx->r5 = ctx->r16 | 0;
    Graph_Update(rdram, ctx);

in PadMgr_ThreadEntry:
L_801760A0:
    // or          $a1, $s2, $zero
    ctx->r5 = ctx->r18 | 0;
    // jal         0x80087ED0
    // or          $a2, $s3, $zero
    ctx->r6 = ctx->r19 | 0;
    osRecvMesg_recomp(rdram, ctx);
    +handle_quicksave_actions(rdram, ctx);

in AudioMgr_ThreadEntry:
L_80172F58:
    +handle_quicksave_actions(rdram, ctx);
    +ctx->r4 = ctx->r22 | 0;
    // or          $a1, $s2, $zero
    ctx->r5 = ctx->r18 | 0;
    // jal         0x80087ED0
    // or          $a2, $s3, $zero
    ctx->r6 = ctx->r19 | 0;
    osRecvMesg_recomp(rdram, ctx);

*/

#if 0

#include "patches.h"
#include "input.h"
#include "audiomgr.h"

// ultramodern's message queues can be statically initialized, so we don't actually need to call osCreateMesgQueue for these.
OSMesg quicksave_enter_mq_buffer[20];
OSMesgQueue quicksave_enter_mq = {
    .msgCount = ARRAY_COUNT(quicksave_enter_mq_buffer),
    .msg = quicksave_enter_mq_buffer,
    .validCount = 0,
    .first = 0
};

OSMesg quicksave_exit_mq_buffer[20];
OSMesgQueue quicksave_exit_mq = {
    .msgCount = ARRAY_COUNT(quicksave_exit_mq_buffer),
    .msg = quicksave_exit_mq_buffer,
    .validCount = 0,
    .first = 0
};

void handle_quicksave_actions() {
    recomp_handle_quicksave_actions(&quicksave_enter_mq, &quicksave_exit_mq);
}

extern OSMesgQueue sIrqMgrMsgQueue; // For Main thread
s16 main_irq_message = 0; // dummy value for sending a pointer to the main thread

#define RECOMP_IRQ_QUICKSAVE 0x2A2

void IrqMgr_ThreadEntry(IrqMgr* irqmgr) {
    u32 interrupt;
    u32 stop;

    interrupt = 0;
    stop = 0;
    while (stop == 0) {
        if (stop) {
            ;
        }

        osRecvMesg(&irqmgr->irqQueue, (OSMesg*)&interrupt, OS_MESG_BLOCK);
        switch (interrupt) {
            case 0x29A:
                IrqMgr_HandleRetrace(irqmgr);
                break;
            case 0x29D:
                IrqMgr_HandlePreNMI(irqmgr);
                break;
            case 0x29F:
                IrqMgr_HandlePRENMI450(irqmgr);
                break;
            case 0x2A0:
                IrqMgr_HandlePRENMI480(irqmgr);
                break;
            case 0x2A1:
                IrqMgr_HandlePRENMI500(irqmgr);
                break;
            case RECOMP_IRQ_QUICKSAVE:
                handle_quicksave_actions();
                break;
        }
    }
}

extern IrqMgr gIrqMgr;

#define RSP_DONE_MSG 667
#define RDP_DONE_MSG 668
#define ENTRY_MSG 670
#define RDP_AUDIO_CANCEL_MSG 671
#define RSP_GFX_CANCEL_MSG 672
#define RECOMP_QUICKSAVE_ACTION 673 // @recomp


void Sched_ThreadEntry(void* arg) {
    s32 msg = 0;
    SchedContext* sched = (SchedContext*)arg;

    while (true) {
        osRecvMesg(&sched->interruptQ, (OSMesg*)&msg, OS_MESG_BLOCK);

        // Check if it's a message from another thread or the OS
        switch (msg) {
            case RDP_AUDIO_CANCEL_MSG:
                Sched_HandleAudioCancel(sched);
                continue;

            case RSP_GFX_CANCEL_MSG:
                Sched_HandleGfxCancel(sched);
                continue;

            case ENTRY_MSG:
                Sched_HandleEntry(sched);
                continue;

            case RSP_DONE_MSG:
                Sched_HandleRSPDone(sched);
                continue;

            case RDP_DONE_MSG:
                Sched_HandleRDPDone(sched);
                continue;
            
            case RECOMP_QUICKSAVE_ACTION:
                handle_quicksave_actions();
                continue;
        }

        // Check if it's a message from the IrqMgr
        switch (((OSScMsg*)msg)->type) {
            case OS_SC_RETRACE_MSG:
                Sched_HandleRetrace(sched);
                continue;

            case OS_SC_PRE_NMI_MSG:
                Sched_HandleReset(sched);
                continue;

            case OS_SC_NMI_MSG:
                Sched_HandleStop(sched);
                continue;
        }
    }
}

extern PadMgr* sPadMgrInstance;
s16 padmgr_dummy_message = 0;

extern AudioMgr sAudioMgr;
s16 audiomgr_dummy_message = 0;

extern OSMesgQueue sDmaMgrMsgQueue;
#define RECOMP_DMAMGR_QUICKSAVE_MESSAGE 1

void DmaMgr_ThreadEntry(void* a0) {
    OSMesg msg;
    DmaRequest* req;

    while (1) {
        osRecvMesg(&sDmaMgrMsgQueue, &msg, OS_MESG_BLOCK);

        if (msg == NULL) {
            break;
        }

        if (msg == (OSMesg)RECOMP_DMAMGR_QUICKSAVE_MESSAGE) {
            handle_quicksave_actions();
            continue;
        }

        req = (DmaRequest*)msg;

        DmaMgr_ProcessMsg(req);
        if (req->notifyQueue) {
            osSendMesg(req->notifyQueue, req->notifyMsg, OS_MESG_NOBLOCK);
        }
    }
}

extern SchedContext gSchedContext;

void handle_quicksave_actions_main() {
    recomp_handle_quicksave_actions_main(&quicksave_enter_mq, &quicksave_exit_mq);
}

void wake_threads_for_quicksave_action() {
    // Wake up the Main thread
    osSendMesg(&sIrqMgrMsgQueue, &main_irq_message, OS_MESG_BLOCK);
    // Wake up the IrqMgr thread
    osSendMesg(&gIrqMgr.irqQueue, (OSMesg)RECOMP_IRQ_QUICKSAVE, OS_MESG_BLOCK);
    // Wake up the PadMgr thread
    osSendMesg(&sPadMgrInstance->interruptQueue, &padmgr_dummy_message, OS_MESG_BLOCK);
    // Wake up the AudioMgr thread
    osSendMesg(&sAudioMgr.interruptQueue, &audiomgr_dummy_message, OS_MESG_BLOCK);
    // Wake up the DmaMgr thread
    osSendMesg(&sDmaMgrMsgQueue, (OSMesg)RECOMP_DMAMGR_QUICKSAVE_MESSAGE, OS_MESG_BLOCK);
    // Wake up the Sched thread
    osSendMesg(&gSchedContext.interruptQ, (OSMesg)RECOMP_QUICKSAVE_ACTION, OS_MESG_BLOCK);
}

#endif
