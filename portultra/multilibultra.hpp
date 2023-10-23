#ifndef __MULTILIBULTRA_HPP__
#define __MULTILIBULTRA_HPP__

#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>

#include "ultra64.h"

#if defined(_WIN32)
#   include <Windows.h>
#elif defined(__ANDROID__)
#   include "android/native_window.h"
#elif defined(__linux__)
#   include "X11/Xlib.h"
#   undef None
#   undef Status
#   undef LockMask
#endif

struct UltraThreadContext {
    std::thread host_thread;
    std::atomic_bool scheduled;
    std::atomic_bool descheduled;
    std::atomic_bool initialized;
};

namespace Multilibultra {

#if defined(_WIN32)
    // Native HWND handle to the target window.
    using WindowHandle = HWND;
#elif defined(__ANDROID__)
    using WindowHandle = ANativeWindow*;
#elif defined(__linux__)
    struct WindowHandle {
        Display* display;
        Window window;
    };
#endif

// We need a place in rdram to hold the PI handles, so pick an address in extended rdram
constexpr int32_t cart_handle = 0x80800000;
constexpr int32_t flash_handle = (int32_t)(cart_handle + sizeof(OSPiHandle));
constexpr uint32_t save_size = 1024 * 1024 / 8; // Maximum save size, 1Mbit for flash

void preinit(uint8_t* rdram, uint8_t* rom, WindowHandle window_handle);
void save_init();
void init_scheduler();
void init_events(uint8_t* rdram, uint8_t* rom, WindowHandle window_handle);
void init_timers(RDRAM_ARG1);
void yield_self(RDRAM_ARG1);
void block_self(RDRAM_ARG1);
void unblock_thread(OSThread* t);
void wait_for_resumed(RDRAM_ARG1);
void swap_to_thread(RDRAM_ARG OSThread *to);
void resume_thread_impl(OSThread* t);
void schedule_running_thread(OSThread *t);
void halt_self(RDRAM_ARG1);
void stop_thread(OSThread *t);
void cleanup_thread(OSThread *t);

enum class ThreadPriority {
    Low,
    Normal,
    High,
    VeryHigh,
    Critical
};

void set_native_thread_name(const std::string& name);
void set_native_thread_priority(ThreadPriority pri);
PTR(OSThread) this_thread();
void notify_scheduler();
void reprioritize_thread(OSThread *t, OSPri pri);
void set_main_thread();
bool is_game_thread();
void submit_rsp_task(RDRAM_ARG PTR(OSTask) task);
void send_si_message();
uint32_t get_speed_multiplier();
std::chrono::system_clock::time_point get_start();
std::chrono::system_clock::duration time_since_start();

// Audio
void init_audio();
void set_audio_frequency(uint32_t freq);
void queue_audio_buffer(RDRAM_ARG PTR(s16) audio_data, uint32_t byte_count);
uint32_t get_remaining_audio_bytes();

struct audio_callbacks_t {
    using queue_samples_t = void(int16_t*, size_t);
    using get_samples_remaining_t = size_t();
    using set_frequency_t = void(uint32_t);
    queue_samples_t* queue_samples;
    get_samples_remaining_t* get_frames_remaining;
    set_frequency_t* set_frequency;
};
void set_audio_callbacks(const audio_callbacks_t* callbacks);

// Input
struct input_callbacks_t {
    using get_input_t = void(uint16_t*, float*, float*);
    get_input_t* get_input;
};
void set_input_callbacks(const input_callbacks_t* callback);

struct gfx_callbacks_t {
    using gfx_data_t = void*;
    using create_gfx_t = gfx_data_t();
    using create_window_t = WindowHandle(gfx_data_t);
    using update_gfx_t = void(gfx_data_t);
    create_gfx_t* create_gfx;
    create_window_t* create_window;
    update_gfx_t* update_gfx;
};
void set_gfx_callbacks(const gfx_callbacks_t* callbacks);

} // namespace Multilibultra

#define MIN(a, b) ((a) < (b) ? (a) : (b))

//#define debug_printf(...)
#define debug_printf(...) printf(__VA_ARGS__);

#endif
