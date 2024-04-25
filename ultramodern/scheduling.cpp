#include "ultramodern.hpp"

void ultramodern::run_next_thread(RDRAM_ARG1) {
    if (thread_queue_empty(PASS_RDRAM running_queue)) {
        throw std::runtime_error("No threads left to run!\n");
    }

    OSThread* to_run = TO_PTR(OSThread, thread_queue_pop(PASS_RDRAM running_queue));
    debug_printf("[Scheduling] Resuming execution of thread %d\n", to_run->id);
    to_run->context->running.signal();
}

void ultramodern::schedule_running_thread(RDRAM_ARG PTR(OSThread) t_) {
    debug_printf("[Scheduling] Adding thread %d to the running queue\n", TO_PTR(OSThread, t_)->id);
    thread_queue_insert(PASS_RDRAM running_queue, t_);
    TO_PTR(OSThread, t_)->state = OSThreadState::QUEUED;
}

void ultramodern::check_running_queue(RDRAM_ARG1) {
    // Check if there are any threads in the running queue.
    if (!thread_queue_empty(PASS_RDRAM running_queue)) {
        // Check if the highest priority thread in the queue is higher priority than the current thread.
        OSThread* next_thread = TO_PTR(OSThread, ultramodern::thread_queue_peek(PASS_RDRAM running_queue));
        OSThread* self = TO_PTR(OSThread, ultramodern::this_thread());
        if (next_thread->priority > self->priority) {
            ultramodern::thread_queue_pop(PASS_RDRAM running_queue);
            // Swap to the higher priority thread.
            ultramodern::swap_to_thread(PASS_RDRAM next_thread);
        }
    }
}

void ultramodern::swap_to_thread(RDRAM_ARG OSThread *to) {
    debug_printf("[Scheduling] Thread %d giving execution to thread %d\n", TO_PTR(OSThread, ultramodern::this_thread())->id, to->id);
    // Insert this thread in the running queue.
    thread_queue_insert(PASS_RDRAM running_queue, ultramodern::this_thread());
    TO_PTR(OSThread, ultramodern::this_thread())->state = OSThreadState::QUEUED;
    // Unpause the target thread and wait for this one to be unpaused.
    ultramodern::resume_thread(to);
    ultramodern::wait_for_resumed(PASS_RDRAM1);
}

void ultramodern::wait_for_resumed(RDRAM_ARG1) {
    TO_PTR(OSThread, ultramodern::this_thread())->context->running.wait();
    // If this thread was marked to be destroyed by another thre, destroy it again from its own context.
    // This will actually destroy the thread instead of just marking it to be destroyed.
    if (TO_PTR(OSThread, ultramodern::this_thread())->destroyed) {
        osDestroyThread(PASS_RDRAM NULLPTR);
    }
}

void ultramodern::resume_thread(OSThread *t) {
    if (t->state != OSThreadState::QUEUED) {
        assert(false && "Threads should only be resumed from the queued state!");
    }
    t->state = OSThreadState::RUNNING;
    t->context->running.signal();
}

extern "C" void pause_self(RDRAM_ARG1) {
    while (true) {
        // Wait until an external message arrives, then allow the next thread to run.
        ultramodern::wait_for_external_message(PASS_RDRAM1);
        ultramodern::check_running_queue(PASS_RDRAM1);
    }
}
