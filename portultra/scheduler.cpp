#include <thread>
#include <queue>
#include <atomic>
#include <vector>
#include <variant>

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

        if (it == this->c.begin()) {
            // deque the top element
            this->pop();
        } else {
            // remove element and re-heap
            this->c.erase(it);
            std::make_heap(this->c.begin(), this->c.end(), this->comp);
        }
        
        return true;
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

using ThreadAction = std::variant<NotifySchedulerAction, ScheduleThreadAction, StopThreadAction, CleanupThreadAction, ReprioritizeThreadAction>;

static struct {
    moodycamel::BlockingConcurrentQueue<ThreadAction> action_queue{};
    OSThread* running_thread;

    bool can_preempt;
    std::mutex premption_mutex;
} scheduler_context{};

void handle_thread_queueing(thread_queue_t& running_thread_queue, const ScheduleThreadAction& action) {
    OSThread* to_schedule = action.t;
    debug_printf("[Scheduler] Scheduling thread %d\n", to_schedule->id);
    running_thread_queue.push(to_schedule);
}

void handle_thread_stopping(thread_queue_t& running_thread_queue, const StopThreadAction& action) {
    OSThread* to_stop = action.t;
    debug_printf("[Scheduler] Stopping thread %d\n", to_stop->id);
    running_thread_queue.remove(to_stop);
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

void swap_running_thread(thread_queue_t& running_thread_queue, OSThread*& cur_running_thread) {
    if (running_thread_queue.size() > 0) {
        OSThread* new_running_thread = running_thread_queue.top();
        if (cur_running_thread != new_running_thread) {
            if (cur_running_thread && cur_running_thread->state == OSThreadState::RUNNING) {
                debug_printf("[Scheduler] Need to wait for thread %d to pause itself\n", cur_running_thread->id);
                return;
            } else {
                debug_printf("[Scheduler] Switching execution to thread %d (%d)\n", new_running_thread->id, new_running_thread->priority);
            }
            Multilibultra::resume_thread_impl(new_running_thread);
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
        ThreadAction action;
        OSThread* old_running_thread = cur_running_thread;
        scheduler_context.action_queue.wait_dequeue(action);

        std::lock_guard lock{scheduler_context.premption_mutex};

        // Determine the action type and act on it
        if (const auto* cleanup_action = std::get_if<NotifySchedulerAction>(&action)) {
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
    scheduler_context.can_preempt = true;
    std::thread scheduler_thread{scheduler_func};
    scheduler_thread.detach();
}

void schedule_running_thread(OSThread *t) {
    debug_printf("[Scheduler] Queuing Thread %d to be scheduled\n", t->id);
    scheduler_context.action_queue.enqueue(ScheduleThreadAction{t});
}

void swap_to_thread(RDRAM_ARG OSThread *to) {
    OSThread *self = TO_PTR(OSThread, Multilibultra::this_thread());
    debug_printf("[Scheduler] Scheduling swap from thread %d to %d\n", self->id, to->id);
    
    Multilibultra::set_self_paused(PASS_RDRAM1);
    scheduler_context.action_queue.enqueue(ScheduleThreadAction{to});
    Multilibultra::wait_for_resumed(PASS_RDRAM1);
}

void reprioritize_thread(OSThread *t, OSPri pri) {
    debug_printf("[Scheduler] Adjusting Thread %d priority to %d\n", t->id, pri);

    scheduler_context.action_queue.enqueue(ReprioritizeThreadAction{t, pri});
}

void pause_self(RDRAM_ARG1) {
    OSThread *self = TO_PTR(OSThread, Multilibultra::this_thread());
    debug_printf("[Scheduler] Thread %d pausing itself\n", self->id);

    Multilibultra::set_self_paused(PASS_RDRAM1);
    scheduler_context.action_queue.enqueue(StopThreadAction{self});
    Multilibultra::wait_for_resumed(PASS_RDRAM1);
}

void cleanup_thread(OSThread *t) {
    scheduler_context.action_queue.enqueue(CleanupThreadAction{t});
}

void disable_preemption() {
    scheduler_context.premption_mutex.lock();
    if (Multilibultra::is_game_thread()) {
        scheduler_context.can_preempt = false;
    }
}

void enable_preemption() {
    if (Multilibultra::is_game_thread()) {
        scheduler_context.can_preempt = true;
    }
#pragma warning(push)
#pragma warning( disable : 26110)
    scheduler_context.premption_mutex.unlock();
#pragma warning( pop ) 
}

// lock's constructor is called first, so can_preempt is set after locking
preemption_guard::preemption_guard() : lock{scheduler_context.premption_mutex} {
    scheduler_context.can_preempt = false;
}

// lock's destructor is called last, so can_preempt is set before unlocking
preemption_guard::~preemption_guard() {
    scheduler_context.can_preempt = true;
}

void notify_scheduler() {
    scheduler_context.action_queue.enqueue(NotifySchedulerAction{});
}

}

extern "C" void pause_self(uint8_t* rdram) {
    Multilibultra::pause_self(rdram);
}

