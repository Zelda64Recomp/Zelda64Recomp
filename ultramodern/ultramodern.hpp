#ifndef __ultramodern_HPP__
#define __ultramodern_HPP__

#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>

#include "ultra64.h"

#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN
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
    std::atomic_bool running;
    std::atomic_bool initialized;
};

namespace ultramodern {

#if defined(_WIN32)
    // Native HWND handle to the target window.
    struct WindowHandle {
        HWND window;
        DWORD thread_id = (DWORD)-1;
        auto operator<=>(const WindowHandle&) const = default;
    };
#elif defined(__ANDROID__)
    using WindowHandle = ANativeWindow*;
#elif defined(__linux__)
    struct WindowHandle {
        Display* display;
        Window window;
        auto operator<=>(const WindowHandle&) const = default;
    };
#endif

// We need a place in rdram to hold the PI handles, so pick an address in extended rdram
constexpr uint32_t rdram_size = 1024 * 1024 * 16; // 16MB to give extra room for anything custom
constexpr int32_t cart_handle = 0x80800000;
constexpr int32_t flash_handle = (int32_t)(cart_handle + sizeof(OSPiHandle));
constexpr uint32_t save_size = 1024 * 1024 / 8; // Maximum save size, 1Mbit for flash

void preinit(uint8_t* rdram, WindowHandle window_handle);
void save_init();
void init_scheduler();
void init_events(uint8_t* rdram, WindowHandle window_handle);
void init_timers(RDRAM_ARG1);
void set_self_paused(RDRAM_ARG1);
void yield_self(RDRAM_ARG1);
void block_self(RDRAM_ARG1);
void unblock_thread(OSThread* t);
void wait_for_resumed(RDRAM_ARG1);
void swap_to_thread(RDRAM_ARG OSThread *to);
void pause_thread_impl(OSThread *t);
void resume_thread_impl(OSThread* t);
void schedule_running_thread(OSThread *t);
void pause_self(RDRAM_ARG1);
void halt_self(RDRAM_ARG1);
void stop_thread(OSThread *t);
void cleanup_thread(OSThread *t);
uint32_t permanent_thread_count();
uint32_t temporary_thread_count();

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
void disable_preemption();
void enable_preemption();
void notify_scheduler();
void reprioritize_thread(OSThread *t, OSPri pri);
void set_main_thread();
bool is_game_thread();
void submit_rsp_task(RDRAM_ARG PTR(OSTask) task);
void send_si_message();
uint32_t get_speed_multiplier();
std::chrono::high_resolution_clock::time_point get_start();
std::chrono::high_resolution_clock::duration time_since_start();
void get_window_size(uint32_t& width, uint32_t& height);
uint32_t get_target_framerate(uint32_t original);
uint32_t get_display_refresh_rate();
void measure_input_latency();
void sleep_milliseconds(uint32_t millis);
void sleep_until(const std::chrono::high_resolution_clock::time_point& time_point);

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

// Input
struct input_callbacks_t {
    using poll_input_t = void(void);
    using get_input_t = void(uint16_t*, float*, float*);
    using set_rumble_t = void(bool);
    poll_input_t* poll_input;
    get_input_t* get_input;
    set_rumble_t* set_rumble;
};

struct gfx_callbacks_t {
    using gfx_data_t = void*;
    using create_gfx_t = gfx_data_t();
    using create_window_t = WindowHandle(gfx_data_t);
    using update_gfx_t = void(gfx_data_t);
    create_gfx_t* create_gfx;
    create_window_t* create_window;
    update_gfx_t* update_gfx;
};
bool is_game_started();
void quit();
void join_event_threads();

} // namespace ultramodern

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define debug_printf(...)
//#define debug_printf(...) printf(__VA_ARGS__);

#endif
