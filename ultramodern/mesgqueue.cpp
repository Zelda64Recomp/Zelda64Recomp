#include <thread>
#include <atomic>

#include "ultra64.h"
#include "ultramodern.hpp"
#include "recomp.h"

extern "C" void osCreateMesgQueue(RDRAM_ARG PTR(OSMesgQueue) mq_, PTR(OSMesg) msg, s32 count) {
    OSMesgQueue *mq = TO_PTR(OSMesgQueue, mq_);
    mq->blocked_on_recv = NULLPTR;
    mq->blocked_on_send = NULLPTR;
    mq->msgCount = count;
    mq->msg = msg;
    mq->validCount = 0;
    mq->first = 0;
}

s32 MQ_GET_COUNT(OSMesgQueue *mq) {
    return mq->validCount;
}

s32 MQ_IS_EMPTY(OSMesgQueue *mq) {
    return mq->validCount == 0;
}

s32 MQ_IS_FULL(OSMesgQueue* mq) {
    return MQ_GET_COUNT(mq) >= mq->msgCount;
}

void thread_queue_insert(RDRAM_ARG PTR(OSThread)* queue, PTR(OSThread) toadd_) {
    PTR(OSThread)* cur = queue;
    OSThread* toadd = TO_PTR(OSThread, toadd_); 
    while (*cur && TO_PTR(OSThread, *cur)->priority > toadd->priority) {
        cur = &TO_PTR(OSThread, *cur)->next;
    }
    toadd->next = (*cur);
    *cur = toadd_;
}

OSThread* thread_queue_pop(RDRAM_ARG PTR(OSThread)* queue) {
    PTR(OSThread) ret = *queue;
    *queue = TO_PTR(OSThread, ret)->next;
    return TO_PTR(OSThread, ret);
}

bool thread_queue_empty(RDRAM_ARG PTR(OSThread)* queue) {
    return *queue == NULLPTR;
}

extern "C" s32 osSendMesg(RDRAM_ARG PTR(OSMesgQueue) mq_, OSMesg msg, s32 flags) {
    OSMesgQueue *mq = TO_PTR(OSMesgQueue, mq_);
    
    // Prevent accidentally blocking anything that isn't a game thread
    if (!ultramodern::is_game_thread()) {
        flags = OS_MESG_NOBLOCK;
    }

    ultramodern::disable_preemption();

    if (flags == OS_MESG_NOBLOCK) {
        // If non-blocking, fail if the queue is full
        if (MQ_IS_FULL(mq)) {
            ultramodern::enable_preemption();
            return -1;
        }
    } else {
        // Otherwise, yield this thread until the queue has room
        while (MQ_IS_FULL(mq)) {
            debug_printf("[Message Queue] Thread %d is blocked on send\n", TO_PTR(OSThread, ultramodern::this_thread())->id);
            thread_queue_insert(PASS_RDRAM &mq->blocked_on_send, ultramodern::this_thread());
            ultramodern::enable_preemption();
            ultramodern::pause_self(PASS_RDRAM1);
            ultramodern::disable_preemption();
        }
    }
    
    s32 last = (mq->first + mq->validCount) % mq->msgCount;
    TO_PTR(OSMesg, mq->msg)[last] = msg;
    mq->validCount++;
    
    OSThread* to_run = nullptr;

    if (!thread_queue_empty(PASS_RDRAM &mq->blocked_on_recv)) {
        to_run = thread_queue_pop(PASS_RDRAM &mq->blocked_on_recv);
    }
    
    ultramodern::enable_preemption();
    if (to_run) {
        debug_printf("[Message Queue] Thread %d is unblocked\n", to_run->id);
        if (ultramodern::is_game_thread()) {
            OSThread* self = TO_PTR(OSThread, ultramodern::this_thread());
            if (to_run->priority > self->priority) {
                ultramodern::swap_to_thread(PASS_RDRAM to_run);
            } else {
                ultramodern::schedule_running_thread(to_run);
            }
        } else {
            ultramodern::schedule_running_thread(to_run);
        }
    }
    return 0;
}

extern "C" s32 osJamMesg(RDRAM_ARG PTR(OSMesgQueue) mq_, OSMesg msg, s32 flags) {
    OSMesgQueue *mq = TO_PTR(OSMesgQueue, mq_);
    ultramodern::disable_preemption();

    if (flags == OS_MESG_NOBLOCK) {
        // If non-blocking, fail if the queue is full
        if (MQ_IS_FULL(mq)) {
            ultramodern::enable_preemption();
            return -1;
        }
    } else {
        // Otherwise, yield this thread in a loop until the queue is no longer full
        while (MQ_IS_FULL(mq)) {
            debug_printf("[Message Queue] Thread %d is blocked on jam\n", TO_PTR(OSThread, ultramodern::this_thread())->id);
            thread_queue_insert(PASS_RDRAM &mq->blocked_on_send, ultramodern::this_thread());
            ultramodern::enable_preemption();
            ultramodern::pause_self(PASS_RDRAM1);
            ultramodern::disable_preemption();
        }
    }
    
    mq->first = (mq->first + mq->msgCount - 1) % mq->msgCount;
    TO_PTR(OSMesg, mq->msg)[mq->first] = msg;
    mq->validCount++;
    
    OSThread *to_run = nullptr;

    if (!thread_queue_empty(PASS_RDRAM &mq->blocked_on_recv)) {
        to_run = thread_queue_pop(PASS_RDRAM &mq->blocked_on_recv);
    }
    
    ultramodern::enable_preemption();
    if (to_run) {
        debug_printf("[Message Queue] Thread %d is unblocked\n", to_run->id);
        OSThread *self = TO_PTR(OSThread, ultramodern::this_thread());
        if (to_run->priority > self->priority) {
            ultramodern::swap_to_thread(PASS_RDRAM to_run);
        } else {
            ultramodern::schedule_running_thread(to_run);
        }
    }
    return 0;
}

extern "C" s32 osRecvMesg(RDRAM_ARG PTR(OSMesgQueue) mq_, PTR(OSMesg) msg_, s32 flags) {
    OSMesgQueue *mq = TO_PTR(OSMesgQueue, mq_);
    OSMesg *msg = TO_PTR(OSMesg, msg_);
    ultramodern::disable_preemption();

    if (flags == OS_MESG_NOBLOCK) {
        // If non-blocking, fail if the queue is empty
        if (MQ_IS_EMPTY(mq)) {
            ultramodern::enable_preemption();
            return -1;
        }
    } else {
        // Otherwise, yield this thread in a loop until the queue is no longer full
        while (MQ_IS_EMPTY(mq)) {
            debug_printf("[Message Queue] Thread %d is blocked on receive\n", TO_PTR(OSThread, ultramodern::this_thread())->id);
            thread_queue_insert(PASS_RDRAM &mq->blocked_on_recv, ultramodern::this_thread());
            ultramodern::enable_preemption();
            ultramodern::pause_self(PASS_RDRAM1);
            ultramodern::disable_preemption();
        }
    }

    if (msg_ != NULLPTR) {
        *msg = TO_PTR(OSMesg, mq->msg)[mq->first];
    }
    
    mq->first = (mq->first + 1) % mq->msgCount;
    mq->validCount--;

    OSThread *to_run = nullptr;

    if (!thread_queue_empty(PASS_RDRAM &mq->blocked_on_send)) {
        to_run = thread_queue_pop(PASS_RDRAM &mq->blocked_on_send);
    }
    
    ultramodern::enable_preemption();
    if (to_run) {
        debug_printf("[Message Queue] Thread %d is unblocked\n", to_run->id);
        OSThread *self = TO_PTR(OSThread, ultramodern::this_thread());
        if (to_run->priority > self->priority) {
            ultramodern::swap_to_thread(PASS_RDRAM to_run);
        } else {
            ultramodern::schedule_running_thread(to_run);
        }
    }
    return 0;
}
