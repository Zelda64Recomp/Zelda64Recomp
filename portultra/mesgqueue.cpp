#include <thread>
#include <atomic>

#include "ultra64.h"
#include "multilibultra.hpp"
#include "recomp.h"

#if defined(_M_X64)
static inline void spinlock_pause() {
    _mm_pause();
}
#elif defined(__x86_64__)
static inline void spinlock_pause() {
    __builtin_ia32_pause();
}
#else
#error "No spinlock_pause implementation for current architecture"
#endif

template <typename T>
class atomic_spinlock {
    static_assert(sizeof(std::atomic<T>) == sizeof(T), "atomic_spinlock must be used with a type that is the same size as its atomic counterpart");
    static_assert(std::atomic<T>::is_always_lock_free, "atomic_spinlock must be used with an always lock-free atomic type");
    std::atomic_ref<T> locked_;
public:
    atomic_spinlock(T& flag) : locked_{ flag } {}

    void lock() {
        // Loop until the lock is acquired.
        while (true) {
            // Try to acquire the lock.
            if (!locked_.exchange(true, std::memory_order_acquire)) {
                // If it was acquired then exit the loop.
                break;
            }
            // Otherwise, wait until the lock is no longer acquired.
            // Doing this instead of constantly trying to acquire the lock reduces cache coherency traffic.
            while (locked_.load(std::memory_order_relaxed)) {
                // Add a platform-specific pause instruction to reduce load unit traffic.
                spinlock_pause();
            }
        }
    }

    void unlock() {
        // Release the lock by setting it to false.
        locked_.store(false, std::memory_order_release);
    }
};

class mesg_queue_lock {
    OSMesgQueue* queue_;
    atomic_spinlock<uint8_t> spinlock_;
public:
    mesg_queue_lock(OSMesgQueue* mq) : queue_{ mq }, spinlock_{ mq->lock } {}
    void lock() { spinlock_.lock(); }
    void unlock() { spinlock_.unlock(); }
};

extern "C" void osCreateMesgQueue(RDRAM_ARG PTR(OSMesgQueue) mq_, PTR(OSMesg) msg, s32 count) {
    OSMesgQueue *mq = TO_PTR(OSMesgQueue, mq_);
    mq->blocked_on_recv = NULLPTR;
    mq->blocked_on_send = NULLPTR;
    mq->msgCount = count;
    mq->msg = msg;
    mq->validCount = 0;
    mq->first = 0;
    mq->lock = false;
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

std::mutex test_mutex{};

// Attempts to put a message into a queue.
// If the queue is not full, returns true and pops a thread from the blocked on receive list. 
// If the queue is full and this is a blocking send, places the current thread into the blocked on send list
// for the message queue, marks the current thread as being blocked on a queue and returns false.
bool mesg_queue_try_insert(RDRAM_ARG OSMesgQueue* mq, OSMesg msg, OSThread*& to_run, bool jam, bool blocking) {
    //mesg_queue_lock lock{ mq };
    std::lock_guard guard{ test_mutex };

    // If the queue is full, insert this thread into the blocked on send queue and return false.
    if (MQ_IS_FULL(mq)) {
        if (blocking) {
            thread_queue_insert(PASS_RDRAM &mq->blocked_on_send, Multilibultra::this_thread());
            // TODO is it safe to use the schedule queue here while in the message queue lock?
            Multilibultra::block_self(PASS_RDRAM1);
        }
        to_run = nullptr;
        return false;
    }

    // The queue wasn't full, so place the message into it.
    if (jam) {
        // Insert this message at the start of the queue.
        mq->first = (mq->first + mq->msgCount - 1) % mq->msgCount;
        TO_PTR(OSMesg, mq->msg)[mq->first] = msg;
        mq->validCount++;
    }
    else {
        // Insert this message at the end of the queue.
        s32 last = (mq->first + mq->validCount) % mq->msgCount;
        TO_PTR(OSMesg, mq->msg)[last] = msg;
        mq->validCount++;
    }

    // Pop a thread from the blocked on recv queue to wake afterwards.
    if (!thread_queue_empty(PASS_RDRAM &mq->blocked_on_recv)) {
        to_run = thread_queue_pop(PASS_RDRAM &mq->blocked_on_recv);
    }

    return true;
}

// Attempts to remove a message from a queue.
// If the queue is not empty, returns true and pops a thread from the blocked on send list. 
// If the queue is empty and this is a blocking receive, places the current thread into the blocked on receive list
// for the message queue, marks the current thread as being blocked on a queue and returns false.
bool mesg_queue_try_remove(RDRAM_ARG OSMesgQueue* mq, PTR(OSMesg) msg_out, OSThread*& to_run, bool blocking) {
    //mesg_queue_lock lock{ mq };
    std::lock_guard guard{ test_mutex };

    // If the queue is full, insert this thread into the blocked on receive queue and return false.
    if (MQ_IS_EMPTY(mq)) {
        if (blocking) {
            thread_queue_insert(PASS_RDRAM &mq->blocked_on_recv, Multilibultra::this_thread());
            // TODO is it safe to use the schedule queue here while in the message queue lock?
            Multilibultra::block_self(PASS_RDRAM1);
        }
        to_run = nullptr;
        return false;
    }

    // The queue wasn't empty, so remove the first message from it.
    if (msg_out != NULLPTR) {
        *TO_PTR(OSMesg, msg_out) = TO_PTR(OSMesg, mq->msg)[mq->first];
    }
    mq->first = (mq->first + 1) % mq->msgCount;
    mq->validCount--;

    // Pop a thread from the blocked on send queue to wake afterwards.
    if (!thread_queue_empty(PASS_RDRAM &mq->blocked_on_send)) {
        to_run = thread_queue_pop(PASS_RDRAM &mq->blocked_on_send);
    }

    return true;
}

enum class MesgQueueActionType {
    Send,
    Jam,
    Receive
};

s32 mesg_queue_action(RDRAM_ARG PTR(OSMesgQueue) mq_, OSMesg msg, PTR(OSMesg) msg_out, s32 flags, MesgQueueActionType action) {
    OSMesgQueue* mq = TO_PTR(OSMesgQueue, mq_);
    OSThread* this_thread = TO_PTR(OSThread, Multilibultra::this_thread());
    bool is_blocking = flags != OS_MESG_NOBLOCK;

    // Prevent accidentally blocking anything that isn't a game thread
    if (!Multilibultra::is_game_thread()) {
        is_blocking = false;
    }

    OSThread* to_run = nullptr;

    // Repeatedly attempt to send the message until it's successful.
    while (true) {
        // Try to insert/remove the message into the queue depending on the action.
        bool success = false;
        switch (action) {
            case MesgQueueActionType::Send:
                success = mesg_queue_try_insert(PASS_RDRAM mq, msg, to_run, false, is_blocking);
                break;
            case MesgQueueActionType::Jam:
                success = mesg_queue_try_insert(PASS_RDRAM mq, msg, to_run, true, is_blocking);
                break;
            case MesgQueueActionType::Receive:
                success = mesg_queue_try_remove(PASS_RDRAM mq, msg_out, to_run, is_blocking);
                break;
        }

        // If successful, don't block.
        if (success) {
            //goto after;
            break;
        }

        // Otherwise if the action was unsuccessful but wasn't blocking, return -1 to indicate a failure.
        if (!is_blocking) {
            return -1;
        }

        // The action failed, so pause this thread until unblocked by the queue.
        debug_printf("[Message Queue] Thread %d is blocked on %s\n", this_thread->id, action == MesgQueueActionType::Receive ? "receive" : "send");

        // Wait for it this thread be resumed.
        Multilibultra::wait_for_resumed(PASS_RDRAM1);
    }
    //after:

    // If any thread was blocked on receiving from this queue, wake it.    
    if (to_run) {
        debug_printf("[Message Queue] Thread %d is unblocked\n", to_run->id);
        Multilibultra::unblock_thread(to_run);

        // If the unblocked thread is higher priority than this one, pause this thread so it can take over.
        if (Multilibultra::is_game_thread() && to_run->priority > this_thread->priority) {
            Multilibultra::yield_self(PASS_RDRAM1);
            Multilibultra::wait_for_resumed(PASS_RDRAM1);
        }
    }

    return 0;
}

extern "C" s32 osSendMesg(RDRAM_ARG PTR(OSMesgQueue) mq_, OSMesg msg, s32 flags) {
    return mesg_queue_action(PASS_RDRAM mq_, msg, NULLPTR, flags, MesgQueueActionType::Send);
}

extern "C" s32 osJamMesg(RDRAM_ARG PTR(OSMesgQueue) mq_, OSMesg msg, s32 flags) {
    return mesg_queue_action(PASS_RDRAM mq_, msg, NULLPTR, flags, MesgQueueActionType::Jam);
}

extern "C" s32 osRecvMesg(RDRAM_ARG PTR(OSMesgQueue) mq_, PTR(OSMesg) msg_out_, s32 flags) {
    return mesg_queue_action(PASS_RDRAM mq_, NULLPTR, msg_out_, flags, MesgQueueActionType::Receive);
}
