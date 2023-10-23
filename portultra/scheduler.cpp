#include <thread>
#include <queue>
#include <atomic>
#include <vector>
#include <variant>
#include <algorithm>

#include "blockingconcurrentqueue.h"
#include "multilibultra.hpp"

class OSThreadComparator {
public:
    bool operator() (OSThread *a, OSThread *b) const {
        return a->priority < b->priority;
    }
};

class thread_queue_t : public std::priority_queue<OSThread*, std::vector<OSThread*>, OSThreadComparator> {
public:
    // TODO comment this
    bool remove(const OSThread* value) {
        auto it = std::find(this->c.begin(), this->c.end(), value);
    
        if (it == this->c.end()) {
            return false;
        }

        // remove element and re-heap
        this->c.erase(it);
        std::make_heap(this->c.begin(), this->c.end(), this->comp);
        
        return true;
    }

    void print() {
        std::vector<OSThread*> backup = this->c;
        debug_printf("[Scheduler] Scheduled Threads:\n");
        while (!empty()) {
            OSThread* t = top();
            pop();
            debug_printf("              %d: pri %d state %d\n", t->id, t->priority, t->state);
        }
        this->c = backup;
    }

    bool contains(OSThread* t) {
        return std::find(this->c.begin(), this->c.end(), t) != this->c.end();
    }
};

struct NotifySchedulerAction {

};

struct ScheduleThreadAction {
    OSThread* t;
};

struct StopThreadAction {
    OSThread* t;
};

struct CleanupThreadAction {
    OSThread* t;
};

struct ReprioritizeThreadAction {
    OSThread* t;
    OSPri pri;
};

struct YieldedThreadAction {
    OSThread* t;
};

struct BlockedThreadAction {
    OSThread* t;
};

struct UnblockThreadAction {
    OSThread* t;
};

using ThreadAction = std::variant<std::monostate, NotifySchedulerAction, ScheduleThreadAction, StopThreadAction, CleanupThreadAction, ReprioritizeThreadAction, YieldedThreadAction, BlockedThreadAction, UnblockThreadAction>;

static struct {
    moodycamel::BlockingConcurrentQueue<ThreadAction> action_queue{};
    OSThread* running_thread;
} scheduler_context{};

void handle_thread_queueing(thread_queue_t& running_thread_queue, const ScheduleThreadAction& action) {
    OSThread* to_schedule = action.t;
    debug_printf("[Scheduler] Scheduling thread %d\n", to_schedule->id);

    // Do not schedule the thread if it's waiting on a message queue
    if (to_schedule->state == OSThreadState::BLOCKED_STOPPED) {
        to_schedule->state = OSThreadState::BLOCKED_PAUSED;
    }
    else {
        to_schedule->state = OSThreadState::PAUSED;
        running_thread_queue.push(to_schedule);
    }
}

void handle_thread_stopping(thread_queue_t& running_thread_queue, const StopThreadAction& action) {
    OSThread* to_stop = action.t;
    debug_printf("[Scheduler] Stopping thread %d\n", to_stop->id);

    running_thread_queue.remove(to_stop);
    if (running_thread_queue.contains(to_stop)) {
        assert(false);
    }

    if (to_stop->state == OSThreadState::BLOCKED_PAUSED) {
        to_stop->state = OSThreadState::BLOCKED_STOPPED;
    }
    else {
        to_stop->state = OSThreadState::STOPPED;
    }
}

void handle_thread_cleanup(thread_queue_t& running_thread_queue, OSThread*& cur_running_thread, const CleanupThreadAction& action) {
    OSThread* to_cleanup = action.t;
    
    debug_printf("[Scheduler] Destroying thread %d\n", to_cleanup->id);
    running_thread_queue.remove(to_cleanup);
    // If the cleaned up thread was the running thread, schedule a new one to run.
    if (to_cleanup == cur_running_thread) {
        // If there's a thread queued to run, set it as the new running thread.
        if (!running_thread_queue.empty()) {
            cur_running_thread = running_thread_queue.top();
        }
        // Otherwise, set the running thread to null so the next thread that can be run gets started.
        else {
            cur_running_thread = nullptr;
        }
    }
    to_cleanup->context->host_thread.join();
    delete to_cleanup->context;
    to_cleanup->context = nullptr;
}

void handle_thread_reprioritization(thread_queue_t& running_thread_queue, const ReprioritizeThreadAction& action) {
    OSThread* to_reprioritize = action.t;
    OSPri pri = action.pri;
    
    debug_printf("[Scheduler] Reprioritizing thread %d to %d\n", to_reprioritize->id, pri);
    running_thread_queue.remove(to_reprioritize);
    to_reprioritize->priority = pri;
    running_thread_queue.push(to_reprioritize);
}

void handle_thread_yielded(thread_queue_t& running_thread_queue, const YieldedThreadAction& action) {
    OSThread* yielded = action.t;
    
    debug_printf("[Scheduler] Thread %d has yielded\n", yielded->id);
    // Remove the yielded thread from the thread queue. If it was in the queue then re-add it so that it's placed after any other threads with the same priority.
    if (running_thread_queue.remove(yielded)) {
        running_thread_queue.push(yielded);
    }
    yielded->state = OSThreadState::PAUSED;
    debug_printf("[Scheduler] Set thread %d to PAUSED\n", yielded->id);
}

void handle_thread_blocked(thread_queue_t& running_thread_queue, const BlockedThreadAction& action) {
    OSThread* blocked = action.t;

    debug_printf("[Scheduler] Thread %d has been blocked\n", blocked->id);
    // Remove the thread from the running queue.
    running_thread_queue.remove(blocked);

    // Update the thread's state accordingly. 
    if (blocked->state == OSThreadState::STOPPED) {
        blocked->state = OSThreadState::BLOCKED_STOPPED;
    }
    else if (blocked->state == OSThreadState::RUNNING) {
        blocked->state = OSThreadState::BLOCKED_PAUSED;
    }
    else {
        assert(false);
    }
    running_thread_queue.remove(blocked);
}

void handle_thread_unblocking(thread_queue_t& running_thread_queue, const UnblockThreadAction& action) {
    OSThread* unblocked = action.t;

    // Do nothing if this thread has already been unblocked.
    if (unblocked->state != OSThreadState::BLOCKED_STOPPED && unblocked->state != OSThreadState::BLOCKED_PAUSED) {
        return;
    }

    debug_printf("[Scheduler] Thread %d has been unblocked\n", unblocked->id);
    // Update the thread's state accordingly. 
    if (unblocked->state == OSThreadState::BLOCKED_STOPPED) {
        unblocked->state = OSThreadState::STOPPED;
    }
    else if (unblocked->state == OSThreadState::BLOCKED_PAUSED) {
        // The thread wasn't stopped, so put it back in the running queue now that it's been unblocked.
        unblocked->state = OSThreadState::PAUSED;
        running_thread_queue.push(unblocked);
    }
    else {
        assert(false);
    }
}

void swap_running_thread(thread_queue_t& running_thread_queue, OSThread*& cur_running_thread) {
    if (running_thread_queue.size() > 0) {
        OSThread* new_running_thread = running_thread_queue.top();
        // If the running thread has changed or the running thread is paused, run the running thread
        if (cur_running_thread != new_running_thread || (cur_running_thread && cur_running_thread->state != OSThreadState::RUNNING)) {
            if (cur_running_thread && cur_running_thread->state == OSThreadState::RUNNING) {
                debug_printf("[Scheduler] Need to wait for thread %d to pause itself\n", cur_running_thread->id);
                return;
            }
            debug_printf("[Scheduler] Switching execution to thread %d (%d)\n", new_running_thread->id, new_running_thread->priority);
            Multilibultra::resume_thread_impl(new_running_thread);
            if (cur_running_thread) {
                cur_running_thread->context->descheduled.store(true);
                cur_running_thread->context->descheduled.notify_all();
            }
            cur_running_thread = new_running_thread;
        } else if (cur_running_thread && cur_running_thread->state != OSThreadState::RUNNING) {
            Multilibultra::resume_thread_impl(cur_running_thread);
        }
    } else {
        cur_running_thread = nullptr;
    }
}

void scheduler_func() {
    thread_queue_t running_thread_queue{};
    OSThread* cur_running_thread = nullptr;

    Multilibultra::set_native_thread_name("Scheduler Thread");
    Multilibultra::set_native_thread_priority(Multilibultra::ThreadPriority::VeryHigh);

    while (true) {
        using namespace std::chrono_literals;
        ThreadAction action{};
        OSThread* old_running_thread = cur_running_thread;
        //scheduler_context.action_queue.wait_dequeue_timed(action, 1ms);
        scheduler_context.action_queue.wait_dequeue(action);

        if (std::get_if<std::monostate>(&action) == nullptr) {
            // Determine the action type and act on it
            if (const auto* notify_action = std::get_if<NotifySchedulerAction>(&action)) {
                // Nothing to do
            }
            else if (const auto* stop_action = std::get_if<StopThreadAction>(&action)) {
                handle_thread_stopping(running_thread_queue, *stop_action);
            }
            else if (const auto* cleanup_action = std::get_if<CleanupThreadAction>(&action)) {
                handle_thread_cleanup(running_thread_queue, cur_running_thread, *cleanup_action);
            }
            else if (const auto* schedule_action = std::get_if<ScheduleThreadAction>(&action)) {
                handle_thread_queueing(running_thread_queue, *schedule_action);
            }
            else if (const auto* reprioritize_action = std::get_if<ReprioritizeThreadAction>(&action)) {
                handle_thread_reprioritization(running_thread_queue, *reprioritize_action);
            }
            else if (const auto* yielded_action = std::get_if<YieldedThreadAction>(&action)) {
                handle_thread_yielded(running_thread_queue, *yielded_action);
            }
            else if (const auto* blocked_action = std::get_if<BlockedThreadAction>(&action)) {
                handle_thread_blocked(running_thread_queue, *blocked_action);
            }
            else if (const auto* unblock_action = std::get_if<UnblockThreadAction>(&action)) {
                handle_thread_unblocking(running_thread_queue, *unblock_action);
            }
        }

        running_thread_queue.print();

        // Determine which thread to run, stopping the current running thread if necessary
        swap_running_thread(running_thread_queue, cur_running_thread);

        std::this_thread::yield();
        if (old_running_thread != cur_running_thread && old_running_thread && cur_running_thread) {
            debug_printf("[Scheduler] Swapped from Thread %d (%d) to Thread %d (%d)\n",
                old_running_thread->id, old_running_thread->priority, cur_running_thread->id, cur_running_thread->priority);
        }
    }
}

extern "C" void do_yield() {
    std::this_thread::yield();
}

namespace Multilibultra {

void init_scheduler() {
    std::thread scheduler_thread{scheduler_func};
    scheduler_thread.detach();
}

void schedule_running_thread(OSThread *t) {
    debug_printf("[Thread] Queuing Thread %d to be scheduled\n", t->id);
    scheduler_context.action_queue.enqueue(ScheduleThreadAction{t});
}

void swap_to_thread(RDRAM_ARG OSThread *to) {
    OSThread *self = TO_PTR(OSThread, Multilibultra::this_thread());
    debug_printf("[Thread] Scheduling swap from thread %d to %d\n", self->id, to->id);
    
    // Tell the scheduler that the swapped-to thread is ready to run and that this thread is yielding.
    schedule_running_thread(to);
    yield_self(PASS_RDRAM1);

    // Wait for the scheduler to resume this thread.
    wait_for_resumed(PASS_RDRAM1);
}

void reprioritize_thread(OSThread *t, OSPri pri) {
    debug_printf("[Thread] Adjusting Thread %d priority to %d\n", t->id, pri);

    scheduler_context.action_queue.enqueue(ReprioritizeThreadAction{t, pri});
}

void stop_thread(OSThread *t) {
    debug_printf("[Thread] Queueing stopping of thread %d\n", t->id);

    scheduler_context.action_queue.enqueue(StopThreadAction{t});
}

void Multilibultra::yield_self(RDRAM_ARG1) {
    OSThread* self = TO_PTR(OSThread, Multilibultra::this_thread());
    debug_printf("[Thread] Thread %d yielding itself\n", self->id);

    scheduler_context.action_queue.enqueue(YieldedThreadAction{ self });
}

void Multilibultra::block_self(RDRAM_ARG1) {
    OSThread* self = TO_PTR(OSThread, Multilibultra::this_thread());
    debug_printf("[Thread] Thread %d has been blocked\n", self->id);

    scheduler_context.action_queue.enqueue(BlockedThreadAction{ self });
    
}

void Multilibultra::unblock_thread(OSThread *t) {
    debug_printf("[Thread] Unblocking thread %d\n", t->id);

    scheduler_context.action_queue.enqueue(UnblockThreadAction{ t });
}

void halt_self(RDRAM_ARG1) {
    OSThread* self = TO_PTR(OSThread, Multilibultra::this_thread());
    debug_printf("[Thread] Thread %d pausing itself\n", self->id);

    stop_thread(self);
    yield_self(PASS_RDRAM1);
    wait_for_resumed(PASS_RDRAM1);
}

void cleanup_thread(OSThread *t) {
    scheduler_context.action_queue.enqueue(CleanupThreadAction{t});
}

void notify_scheduler() {
    scheduler_context.action_queue.enqueue(NotifySchedulerAction{});
}

void resume_thread_impl(OSThread* t) {
    if (t->state == OSThreadState::PREEMPTED) {
        // Nothing to do here
    }
    t->state = OSThreadState::RUNNING;
    debug_printf("[Scheduler] Set thread %d to RUNNING\n", t->id);
    t->context->scheduled.store(true);
    t->context->scheduled.notify_all();
}

}

extern "C" void pause_self(uint8_t* rdram) {
    Multilibultra::halt_self(rdram);
}

