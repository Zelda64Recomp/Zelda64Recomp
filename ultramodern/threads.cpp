#include <cstdio>
#include <thread>
#include <cassert>
#include <string>

#include "ultra64.h"
#include "ultramodern.hpp"
#include "blockingconcurrentqueue.h"

// Native APIs only used to set thread names for easier debugging
#ifdef _WIN32
#include <Windows.h>
#endif

extern "C" void bootproc();

thread_local bool is_main_thread = false;
// Whether this thread is part of the game (i.e. the start thread or one spawned by osCreateThread)
thread_local bool is_game_thread = false;
thread_local PTR(OSThread) thread_self = NULLPTR;

void ultramodern::set_main_thread() {
    ::is_game_thread = true;
    is_main_thread = true;
}

bool ultramodern::is_game_thread() {
    return ::is_game_thread;
}

#if 0
int main(int argc, char** argv) {
    ultramodern::set_main_thread();

    bootproc();
}
#endif

#if 1
void run_thread_function(uint8_t* rdram, uint64_t addr, uint64_t sp, uint64_t arg);
#else
#define run_thread_function(func, sp, arg) func(arg)
#endif

#if defined(_WIN32)
void ultramodern::set_native_thread_name(const std::string& name) {
    std::wstring wname{name.begin(), name.end()};

    HRESULT r;
    r = SetThreadDescription(
        GetCurrentThread(),
        wname.c_str()
    );
}

void ultramodern::set_native_thread_priority(ThreadPriority pri) {
    int nPriority = THREAD_PRIORITY_NORMAL;

    // Convert ThreadPriority to Win32 priority
    switch (pri) {
        case ThreadPriority::Low:
            nPriority = THREAD_PRIORITY_BELOW_NORMAL;
            break;
        case ThreadPriority::Normal:
            nPriority = THREAD_PRIORITY_NORMAL;
            break;
        case ThreadPriority::High:
            nPriority = THREAD_PRIORITY_ABOVE_NORMAL;
            break;
        case ThreadPriority::VeryHigh:
            nPriority = THREAD_PRIORITY_HIGHEST;
            break;
        case ThreadPriority::Critical:
            nPriority = THREAD_PRIORITY_TIME_CRITICAL;
            break;
        default:
            throw std::runtime_error("Invalid thread priority!");
            break;
    }
    // SetThreadPriority(GetCurrentThread(), nPriority);
}
#elif defined(__linux__)
void ultramodern::set_native_thread_name(const std::string& name) {
    pthread_setname_np(pthread_self(), name.c_str());
}

void ultramodern::set_native_thread_priority(ThreadPriority pri) {
    // TODO linux thread priority
    printf("set_native_thread_priority unimplemented\n");
    // int nPriority = THREAD_PRIORITY_NORMAL;

    // // Convert ThreadPriority to Win32 priority
    // switch (pri) {
    //     case ThreadPriority::Low:
    //         nPriority = THREAD_PRIORITY_BELOW_NORMAL;
    //         break;
    //     case ThreadPriority::Normal:
    //         nPriority = THREAD_PRIORITY_NORMAL;
    //         break;
    //     case ThreadPriority::High:
    //         nPriority = THREAD_PRIORITY_ABOVE_NORMAL;
    //         break;
    //     case ThreadPriority::VeryHigh:
    //         nPriority = THREAD_PRIORITY_HIGHEST;
    //         break;
    //     case ThreadPriority::Critical:
    //         nPriority = THREAD_PRIORITY_TIME_CRITICAL;
    //         break;
    //     default:
    //         throw std::runtime_error("Invalid thread priority!");
    //         break;
    // }
}
#endif

std::atomic_int temporary_threads = 0;
std::atomic_int permanent_threads = 0;

static void _thread_func(RDRAM_ARG PTR(OSThread) self_, PTR(thread_func_t) entrypoint, PTR(void) arg) {
    OSThread *self = TO_PTR(OSThread, self_);
    debug_printf("[Thread] Thread created: %d\n", self->id);
    thread_self = self_;
    is_game_thread = true;

    // Set the thread name
    ultramodern::set_native_thread_name("Game Thread " + std::to_string(self->id));
    ultramodern::set_native_thread_priority(ultramodern::ThreadPriority::High);

    // TODO fix these being hardcoded (this is only used for quicksaving)
    if ((self->id == 2 && self->priority == 5) || self->id == 13) { // slowly, flashrom
        temporary_threads.fetch_add(1);
    }
    else if (self->id != 1 && self->id != 2) { // ignore idle and fault
        permanent_threads.fetch_add(1);
    }

    // Signal the initialized semaphore to indicate that this thread can be started.
    self->context->initialized.signal();

    debug_printf("[Thread] Thread waiting to be started: %d\n", self->id);

    // Wait until the thread is marked as running.
    ultramodern::wait_for_resumed(PASS_RDRAM1);

    debug_printf("[Thread] Thread started: %d\n", self->id);

    try {
        // Run the thread's function with the provided argument.
        run_thread_function(PASS_RDRAM entrypoint, self->sp, arg);
        // The thread function terminated normally, so mark this thread as destroyed and run the next thread.
        self->destroyed = true;
        ultramodern::run_next_thread(PASS_RDRAM1);
    } catch (ultramodern::thread_terminated& terminated) {
    }

    // Dispose of this thread after it completes and run the next queued thread.
    ultramodern::cleanup_thread(self);
    
    // TODO fix these being hardcoded (this is only used for quicksaving)
    if ((self->id == 2 && self->priority == 5) || self->id == 13) { // slowly, flashrom
        temporary_threads.fetch_sub(1);
    }
}

uint32_t ultramodern::permanent_thread_count() {
    return permanent_threads.load();
}

uint32_t ultramodern::temporary_thread_count() {
    return temporary_threads.load();
}

extern "C" void osStartThread(RDRAM_ARG PTR(OSThread) t_) {
    OSThread* t = TO_PTR(OSThread, t_);
    debug_printf("[os] Start Thread %d\n", t->id);

    // Wait until the thread is initialized to indicate that it's ready to be started.
    t->context->initialized.wait();

    debug_printf("[os] Thread %d is ready to be started\n", t->id);

    // If this is a game thread, insert the new thread into the running queue and then check the running queue.
    if (thread_self) {
        ultramodern::schedule_running_thread(PASS_RDRAM t_);
        ultramodern::check_running_queue(PASS_RDRAM1);
    }
    // Otherwise, immediately start the thread and terminate this one.
    else {
        t->state = OSThreadState::QUEUED;
        ultramodern::resume_thread(t);
        //throw ultramodern::thread_terminated{};
    }
}

extern "C" void osCreateThread(RDRAM_ARG PTR(OSThread) t_, OSId id, PTR(thread_func_t) entrypoint, PTR(void) arg, PTR(void) sp, OSPri pri) {
    debug_printf("[os] Create Thread %d\n", id);
    OSThread *t = TO_PTR(OSThread, t_);
    
    t->next = NULLPTR;
    t->queue = NULLPTR;
    t->priority = pri;
    t->id = id;
    t->state = OSThreadState::STOPPED;
    t->sp = sp - 0x10; // Set up the first stack frame
    t->destroyed = false;

    // Spawn a new thread, which will immediately pause itself and wait until it's been started.
    t->context = new UltraThreadContext{};
    t->context->host_thread = std::thread{_thread_func, PASS_RDRAM t_, entrypoint, arg};
}

extern "C" void osStopThread(RDRAM_ARG PTR(OSThread) t_) {
    assert(false);
}

extern "C" void osDestroyThread(RDRAM_ARG PTR(OSThread) t_) {
    if (t_ == NULLPTR) {
        t_ = thread_self;
    }
    OSThread* t = TO_PTR(OSThread, t_);
    // Check if the thread is destroying itself (arg is null or thread_self)
    if (t_ == thread_self) {
        // Check if the thread was destroyed by another thread. If it wasn't, then this thread destroyed itself and a new thread
        // needs to be run.
        if (!t->destroyed) {
            t->destroyed = true;
            ultramodern::run_next_thread(PASS_RDRAM1);
        }
        throw ultramodern::thread_terminated{};
    }
    // Otherwise if the thread isn't stopped, remove it from its currrent queue., 
    if (t->state != OSThreadState::STOPPED) {
        ultramodern::thread_queue_remove(PASS_RDRAM t->queue, t_);
    }
    // Check if the thread has already been destroyed to prevent destroying it again.
    if (!t->destroyed) {
        // Mark the target thread as destroyed and resume it. When it starts it'll check this and terminate itself instead of resuming.
        t->destroyed = true;
        t->context->running.signal();
    }
}

extern "C" void osSetThreadPri(RDRAM_ARG PTR(OSThread) t_, OSPri pri) {
    if (t_ == NULLPTR) {
        t_ = thread_self;
    }
    OSThread* t = TO_PTR(OSThread, t_);

    if (t->priority != pri) {
        t->priority = pri;

        if (t_ != ultramodern::this_thread() && t->state != OSThreadState::STOPPED) {
            ultramodern::thread_queue_remove(PASS_RDRAM t->queue, t_);
            ultramodern::thread_queue_insert(PASS_RDRAM t->queue, t_);
        }

        ultramodern::check_running_queue(PASS_RDRAM1);
    }
}

extern "C" OSPri osGetThreadPri(RDRAM_ARG PTR(OSThread) t) {
    if (t == NULLPTR) {
        t = thread_self;
    }
    return TO_PTR(OSThread, t)->priority;
}

extern "C" OSId osGetThreadId(RDRAM_ARG PTR(OSThread) t) {
    if (t == NULLPTR) {
        t = thread_self;
    }
    return TO_PTR(OSThread, t)->id;
}

PTR(OSThread) ultramodern::this_thread() {
    return thread_self;
}

static std::thread thread_cleaner_thread;
static moodycamel::BlockingConcurrentQueue<OSThread*> deleted_threads{};
extern std::atomic_bool exited;

void thread_cleaner_func() {
    using namespace std::chrono_literals;
    while (!exited) {
        OSThread* to_delete;
        if (deleted_threads.wait_dequeue_timed(to_delete, 10ms)) {
            printf("Deleting thread %d\n", to_delete->id);
            UltraThreadContext* cur_context = to_delete->context;
            to_delete->context = nullptr;

            cur_context->host_thread.join();
            delete cur_context;
        }
    }
}

void ultramodern::init_thread_cleanup() {
    thread_cleaner_thread = std::thread{thread_cleaner_func};
}

void ultramodern::cleanup_thread(OSThread *t) {
    deleted_threads.enqueue(t);
}

void ultramodern::join_thread_cleaner_thread() {
    thread_cleaner_thread.join();
}
