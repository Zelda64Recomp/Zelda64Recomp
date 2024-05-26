#include <thread>
#include <atomic>
#include <chrono>
#include <cinttypes>
#include <variant>
#include <unordered_map>
#include <utility>
#include <mutex>
#include <queue>
#include <cstring>

#include "blockingconcurrentqueue.h"

#include "ultra64.h"
#include "ultramodern.hpp"
#include "config.hpp"
#include "rt64_layer.h"
#include "recomp.h"
#include "recomp_game.h"
#include "recomp_ui.h"
#include "recomp_input.h"
#include "rsp.h"

struct SpTaskAction {
    OSTask task;
};

struct SwapBuffersAction {
    uint32_t origin;
};

struct UpdateConfigAction {
};

struct LoadShaderCacheAction {
    std::span<const char> data;
};

using Action = std::variant<SpTaskAction, SwapBuffersAction, UpdateConfigAction, LoadShaderCacheAction>;

static struct {
    struct {
        std::thread thread;
        PTR(OSMesgQueue) mq = NULLPTR;
        PTR(void) current_buffer = NULLPTR;
        PTR(void) next_buffer = NULLPTR;
        OSMesg msg = (OSMesg)0;
        int retrace_count = 1;
    } vi;
    struct {
        std::thread gfx_thread;
        std::thread task_thread;
        PTR(OSMesgQueue) mq = NULLPTR;
        OSMesg msg = (OSMesg)0;
    } sp;
    struct {
        PTR(OSMesgQueue) mq = NULLPTR;
        OSMesg msg = (OSMesg)0;
    } dp;
    struct {
        PTR(OSMesgQueue) mq = NULLPTR;
        OSMesg msg = (OSMesg)0;
    } ai;
    struct {
        PTR(OSMesgQueue) mq = NULLPTR;
        OSMesg msg = (OSMesg)0;
    } si;
    // The same message queue may be used for multiple events, so share a mutex for all of them
    std::mutex message_mutex;
    uint8_t* rdram;
    moodycamel::BlockingConcurrentQueue<Action> action_queue{};
    moodycamel::BlockingConcurrentQueue<OSTask*> sp_task_queue{};
    moodycamel::ConcurrentQueue<OSThread*> deleted_threads{};
} events_context{};

extern "C" void osSetEventMesg(RDRAM_ARG OSEvent event_id, PTR(OSMesgQueue) mq_, OSMesg msg) {
    OSMesgQueue* mq = TO_PTR(OSMesgQueue, mq_);
    std::lock_guard lock{ events_context.message_mutex };

    switch (event_id) {
        case OS_EVENT_SP:
            events_context.sp.msg = msg;
            events_context.sp.mq = mq_;
            break;
        case OS_EVENT_DP:
            events_context.dp.msg = msg;
            events_context.dp.mq = mq_;
            break;
        case OS_EVENT_AI:
            events_context.ai.msg = msg;
            events_context.ai.mq = mq_;
            break;
        case OS_EVENT_SI:
            events_context.si.msg = msg;
            events_context.si.mq = mq_;
    }
}

extern "C" void osViSetEvent(RDRAM_ARG PTR(OSMesgQueue) mq_, OSMesg msg, u32 retrace_count) {
    std::lock_guard lock{ events_context.message_mutex };
    events_context.vi.mq = mq_;
    events_context.vi.msg = msg;
    events_context.vi.retrace_count = retrace_count;
}

uint64_t total_vis = 0;


extern std::atomic_bool exited;

void set_dummy_vi();

void vi_thread_func() {
    ultramodern::set_native_thread_name("VI Thread");
    // This thread should be prioritized over every other thread in the application, as it's what allows
    // the game to generate new audio and gfx lists.
    ultramodern::set_native_thread_priority(ultramodern::ThreadPriority::Critical);
    using namespace std::chrono_literals;
    
    int remaining_retraces = events_context.vi.retrace_count;

    while (!exited) {
        // Determine the next VI time (more accurate than adding 16ms each VI interrupt)
        auto next = ultramodern::get_start() + (total_vis * 1000000us) / (60 * ultramodern::get_speed_multiplier());
        //if (next > std::chrono::high_resolution_clock::now()) {
        //    printf("Sleeping for %" PRIu64 " us to get from %" PRIu64 " us to %" PRIu64 " us \n",
        //        (next - std::chrono::high_resolution_clock::now()) / 1us,
        //        (std::chrono::high_resolution_clock::now() - events_context.start) / 1us,
        //        (next - events_context.start) / 1us);
        //} else {
        //    printf("No need to sleep\n");
        //}
        // Detect if there's more than a second to wait and wait a fixed amount instead for the next VI if so, as that usually means the system clock went back in time.
        if (std::chrono::floor<std::chrono::seconds>(next - std::chrono::high_resolution_clock::now()) > 1s) {
            // printf("Skipping the next VI wait\n");
            next = std::chrono::high_resolution_clock::now();
        }
        ultramodern::sleep_until(next);
        // Calculate how many VIs have passed
        uint64_t new_total_vis = (ultramodern::time_since_start() * (60 * ultramodern::get_speed_multiplier()) / 1000ms) + 1;
        if (new_total_vis > total_vis + 1) {
            //printf("Skipped % " PRId64 " frames in VI interupt thread!\n", new_total_vis - total_vis - 1);
        }
        total_vis = new_total_vis;

        remaining_retraces--;

        {
            std::lock_guard lock{ events_context.message_mutex };
            uint8_t* rdram = events_context.rdram;
            if (remaining_retraces == 0) {
                remaining_retraces = events_context.vi.retrace_count;

                if (ultramodern::is_game_started()) {
                    if (events_context.vi.mq != NULLPTR) {
                        if (osSendMesg(PASS_RDRAM events_context.vi.mq, events_context.vi.msg, OS_MESG_NOBLOCK) == -1) {
                            //printf("Game skipped a VI frame!\n");
                        }
                    }
                }
                else {
                    set_dummy_vi();
                    static bool swap = false;
                    uint32_t vi_origin = 0x400 + 0x280; // Skip initial RDRAM contents and add the usual origin offset
                    // Offset by one FB every other frame so RT64 continues drawing
                    if (swap) {
                        vi_origin += 0x25800;
                    }
                    osViSwapBuffer(rdram, vi_origin);
                    swap = !swap;
                }
            }
            if (events_context.ai.mq != NULLPTR) {
                if (osSendMesg(PASS_RDRAM events_context.ai.mq, events_context.ai.msg, OS_MESG_NOBLOCK) == -1) {
                    //printf("Game skipped a AI frame!\n");
                }
            }
        }
                
        // TODO move recomp code out of ultramodern.
        recomp::update_rumble();
    }
}

void sp_complete() {
    uint8_t* rdram = events_context.rdram;
    std::lock_guard lock{ events_context.message_mutex };
    osSendMesg(PASS_RDRAM events_context.sp.mq, events_context.sp.msg, OS_MESG_NOBLOCK);
}

void dp_complete() {
    uint8_t* rdram = events_context.rdram;
    std::lock_guard lock{ events_context.message_mutex };
    osSendMesg(PASS_RDRAM events_context.dp.mq, events_context.dp.msg, OS_MESG_NOBLOCK);
}

uint8_t dmem[0x1000];
uint16_t rspReciprocals[512];
uint16_t rspInverseSquareRoots[512];

using RspUcodeFunc = RspExitReason(uint8_t* rdram);
extern RspUcodeFunc njpgdspMain;
extern RspUcodeFunc aspMain;

// From Ares emulator. For license details, see rsp_vu.h
void rsp_constants_init() {
    rspReciprocals[0] = u16(~0);
    for (u16 index = 1; index < 512; index++) {
        u64 a = index + 512;
        u64 b = (u64(1) << 34) / a;
        rspReciprocals[index] = u16((b + 1) >> 8);
    }

    for (u16 index = 0; index < 512; index++) {
        u64 a = (index + 512) >> ((index % 2 == 1) ? 1 : 0);
        u64 b = 1 << 17;
        //find the largest b where b < 1.0 / sqrt(a)
        while (a * (b + 1) * (b + 1) < (u64(1) << 44)) b++;
        rspInverseSquareRoots[index] = u16(b >> 1);
    }
}

// Runs a recompiled RSP microcode
void run_rsp_microcode(uint8_t* rdram, const OSTask* task, RspUcodeFunc* ucode_func) {
    // Load the OSTask into DMEM
    memcpy(&dmem[0xFC0], task, sizeof(OSTask));
    // Load the ucode data into DMEM
    dma_rdram_to_dmem(rdram, 0x0000, task->t.ucode_data, 0xF80 - 1);
    // Run the ucode
    RspExitReason exit_reason = ucode_func(rdram);
    // Ensure that the ucode exited correctly
    assert(exit_reason == RspExitReason::Broke);
}


void task_thread_func(uint8_t* rdram, moodycamel::LightweightSemaphore* thread_ready) {
    ultramodern::set_native_thread_name("SP Task Thread");
    ultramodern::set_native_thread_priority(ultramodern::ThreadPriority::Normal);

    // Notify the caller thread that this thread is ready.
    thread_ready->signal();

    while (true) {
        // Wait until an RSP task has been sent
        OSTask* task;
        events_context.sp_task_queue.wait_dequeue(task);

        if (task == nullptr) {
            return;
        }

        // Run the correct function based on the task type
        if (task->t.type == M_AUDTASK) {
            run_rsp_microcode(rdram, task, aspMain);
        }
        else if (task->t.type == M_NJPEGTASK) {
            run_rsp_microcode(rdram, task, njpgdspMain);
        }
        else {
            fprintf(stderr, "Unknown task type: %" PRIu32 "\n", task->t.type);
            assert(false);
            std::quick_exit(EXIT_FAILURE);
        }

        // Tell the game that the RSP has completed
        sp_complete();
    }
}

static std::atomic<ultramodern::GraphicsConfig> cur_config{};

void ultramodern::set_graphics_config(const ultramodern::GraphicsConfig& config) {
    cur_config = config;
    events_context.action_queue.enqueue(UpdateConfigAction{});
}

ultramodern::GraphicsConfig ultramodern::get_graphics_config() {
    return cur_config;
}

std::atomic_uint32_t display_refresh_rate = 60;
std::atomic<float> resolution_scale = 1.0f;

uint32_t ultramodern::get_target_framerate(uint32_t original) {
    ultramodern::GraphicsConfig graphics_config = ultramodern::get_graphics_config();

    switch (graphics_config.rr_option) {
        case RT64::UserConfiguration::RefreshRate::Original:
        default:
            return original;
        case RT64::UserConfiguration::RefreshRate::Manual:
            return graphics_config.rr_manual_value;
        case RT64::UserConfiguration::RefreshRate::Display:
            return display_refresh_rate.load();
    }
}

uint32_t ultramodern::get_display_refresh_rate() {
    return display_refresh_rate.load();
}

float ultramodern::get_resolution_scale() {
    return resolution_scale.load();
}

void ultramodern::load_shader_cache(std::span<const char> cache_data) {
    events_context.action_queue.enqueue(LoadShaderCacheAction{cache_data});
}

std::atomic<ultramodern::RT64SetupResult> rt64_setup_result = ultramodern::RT64SetupResult::Success;

void gfx_thread_func(uint8_t* rdram, moodycamel::LightweightSemaphore* thread_ready, ultramodern::WindowHandle window_handle) {
    bool enabled_instant_present = false;
    using namespace std::chrono_literals;

    ultramodern::set_native_thread_name("Gfx Thread");
    ultramodern::set_native_thread_priority(ultramodern::ThreadPriority::Normal);

    ultramodern::GraphicsConfig old_config = ultramodern::get_graphics_config();

    ultramodern::RT64Context rt64{rdram, window_handle, cur_config.load().developer_mode};

    if (!rt64.valid()) {
        // TODO move recomp code out of ultramodern.
        rt64_setup_result.store(rt64.get_setup_result());
        // Notify the caller thread that this thread is ready.
        thread_ready->signal();
        return;
    }

    // TODO move recomp code out of ultramodern.
    recomp::update_supported_options();
    
    rsp_constants_init();

    // Notify the caller thread that this thread is ready.
    thread_ready->signal();

    while (!exited) {
        // Try to pull an action from the queue
        Action action;
        if (events_context.action_queue.wait_dequeue_timed(action, 1ms)) {
            // Determine the action type and act on it
            if (const auto* task_action = std::get_if<SpTaskAction>(&action)) {
                // Turn on instant present if the game has been started and it hasn't been turned on yet.
                if (ultramodern::is_game_started() && !enabled_instant_present) {
                    rt64.enable_instant_present();
                    enabled_instant_present = true;
                }
                // Tell the game that the RSP completed instantly. This will allow it to queue other task types, but it won't
                // start another graphics task until the RDP is also complete. Games usually preserve the RSP inputs until the RDP
                // is finished as well, so sending this early shouldn't be an issue in most cases.
                // If this causes issues then the logic can be replaced with responding to yield requests.
                sp_complete();
                ultramodern::measure_input_latency();

                auto rt64_start = std::chrono::high_resolution_clock::now();
                rt64.send_dl(&task_action->task);
                auto rt64_end = std::chrono::high_resolution_clock::now();
                dp_complete();
                // printf("RT64 ProcessDList time: %d us\n", static_cast<u32>(std::chrono::duration_cast<std::chrono::microseconds>(rt64_end - rt64_start).count()));
            }
            else if (const auto* swap_action = std::get_if<SwapBuffersAction>(&action)) {
                events_context.vi.current_buffer = events_context.vi.next_buffer;
                rt64.update_screen(swap_action->origin);
                display_refresh_rate = rt64.get_display_framerate();
                resolution_scale = rt64.get_resolution_scale();
            }
            else if (const auto* config_action = std::get_if<UpdateConfigAction>(&action)) {
                ultramodern::GraphicsConfig new_config = cur_config;
                if (old_config != new_config) {
                    rt64.update_config(old_config, new_config);
                    old_config = new_config;
                }
            }
            else if (const auto* load_shader_cache_action = std::get_if<LoadShaderCacheAction>(&action)) {
                rt64.load_shader_cache(load_shader_cache_action->data);
            }
        }
    }
    // TODO move recomp code out of ultramodern.
    recomp::destroy_ui();
    rt64.shutdown();
}

extern unsigned int VI_STATUS_REG;
extern unsigned int VI_ORIGIN_REG;
extern unsigned int VI_WIDTH_REG;
extern unsigned int VI_INTR_REG;
extern unsigned int VI_V_CURRENT_LINE_REG;
extern unsigned int VI_TIMING_REG;
extern unsigned int VI_V_SYNC_REG;
extern unsigned int VI_H_SYNC_REG;
extern unsigned int VI_LEAP_REG;
extern unsigned int VI_H_START_REG;
extern unsigned int VI_V_START_REG;
extern unsigned int VI_V_BURST_REG;
extern unsigned int VI_X_SCALE_REG;
extern unsigned int VI_Y_SCALE_REG;

uint32_t hstart = 0;
uint32_t vi_origin_offset = 320 * sizeof(uint16_t);
bool vi_black = false;

void set_dummy_vi() {
    VI_STATUS_REG = 0x311E;
    VI_WIDTH_REG = 0x140;
    VI_V_SYNC_REG = 0x20D;
    VI_H_SYNC_REG = 0xC15;
    VI_LEAP_REG = 0x0C150C15;
    hstart = 0x006C02EC;
    VI_X_SCALE_REG = 0x200;
    VI_V_CURRENT_LINE_REG = 0x0;
    vi_origin_offset = 0x280;
    VI_Y_SCALE_REG = 0x400;
    VI_V_START_REG = 0x2501FF;
    VI_V_BURST_REG = 0xE0204;
    VI_INTR_REG = 0x2;
}

extern "C" void osViSwapBuffer(RDRAM_ARG PTR(void) frameBufPtr) {
    if (vi_black) {
        VI_H_START_REG = 0;
    } else {
        VI_H_START_REG = hstart;
    }
    events_context.vi.next_buffer = frameBufPtr;
    events_context.action_queue.enqueue(SwapBuffersAction{ osVirtualToPhysical(frameBufPtr) + vi_origin_offset });
}

extern "C" void osViSetMode(RDRAM_ARG PTR(OSViMode) mode_) {
    OSViMode* mode = TO_PTR(OSViMode, mode_);
    VI_STATUS_REG = mode->comRegs.ctrl;
    VI_WIDTH_REG = mode->comRegs.width;
    // burst
    VI_V_SYNC_REG = mode->comRegs.vSync;
    VI_H_SYNC_REG = mode->comRegs.hSync;
    VI_LEAP_REG = mode->comRegs.leap;
    hstart = mode->comRegs.hStart;
    VI_X_SCALE_REG = mode->comRegs.xScale;
    VI_V_CURRENT_LINE_REG = mode->comRegs.vCurrent;

    // TODO swap these every VI to account for fields changing
    vi_origin_offset = mode->fldRegs[0].origin;
    VI_Y_SCALE_REG = mode->fldRegs[0].yScale;
    VI_V_START_REG = mode->fldRegs[0].vStart;
    VI_V_BURST_REG = mode->fldRegs[0].vBurst;
    VI_INTR_REG = mode->fldRegs[0].vIntr;
}

#define VI_CTRL_TYPE_16             0x00002
#define VI_CTRL_TYPE_32             0x00003
#define VI_CTRL_GAMMA_DITHER_ON     0x00004
#define VI_CTRL_GAMMA_ON            0x00008
#define VI_CTRL_DIVOT_ON            0x00010
#define VI_CTRL_SERRATE_ON          0x00040
#define VI_CTRL_ANTIALIAS_MASK      0x00300
#define VI_CTRL_ANTIALIAS_MODE_1    0x00100
#define VI_CTRL_ANTIALIAS_MODE_2    0x00200
#define VI_CTRL_ANTIALIAS_MODE_3    0x00300
#define VI_CTRL_PIXEL_ADV_MASK      0x01000
#define VI_CTRL_PIXEL_ADV_1         0x01000
#define VI_CTRL_PIXEL_ADV_2         0x02000
#define VI_CTRL_PIXEL_ADV_3         0x03000
#define VI_CTRL_DITHER_FILTER_ON    0x10000

#define	OS_VI_GAMMA_ON          0x0001
#define	OS_VI_GAMMA_OFF         0x0002
#define	OS_VI_GAMMA_DITHER_ON   0x0004
#define	OS_VI_GAMMA_DITHER_OFF  0x0008
#define	OS_VI_DIVOT_ON          0x0010
#define	OS_VI_DIVOT_OFF         0x0020
#define	OS_VI_DITHER_FILTER_ON  0x0040
#define	OS_VI_DITHER_FILTER_OFF 0x0080

extern "C" void osViSetSpecialFeatures(uint32_t func) {
    if ((func & OS_VI_GAMMA_ON) != 0) {
        VI_STATUS_REG |= VI_CTRL_GAMMA_ON;
    }

    if ((func & OS_VI_GAMMA_OFF) != 0) {
        VI_STATUS_REG &= ~VI_CTRL_GAMMA_ON;
    }

    if ((func & OS_VI_GAMMA_DITHER_ON) != 0) {
        VI_STATUS_REG |= VI_CTRL_GAMMA_DITHER_ON;
    }

    if ((func & OS_VI_GAMMA_DITHER_OFF) != 0) {
        VI_STATUS_REG &= ~VI_CTRL_GAMMA_DITHER_ON;
    }

    if ((func & OS_VI_DIVOT_ON) != 0) {
        VI_STATUS_REG |= VI_CTRL_DIVOT_ON;
    }

    if ((func & OS_VI_DIVOT_OFF) != 0) {
        VI_STATUS_REG &= ~VI_CTRL_DIVOT_ON;
    }

    if ((func & OS_VI_DITHER_FILTER_ON) != 0) {
        VI_STATUS_REG |= VI_CTRL_DITHER_FILTER_ON;
        VI_STATUS_REG &= ~VI_CTRL_ANTIALIAS_MASK;
    }

    if ((func & OS_VI_DITHER_FILTER_OFF) != 0) {
        VI_STATUS_REG &= ~VI_CTRL_DITHER_FILTER_ON;
        //VI_STATUS_REG |= __osViNext->modep->comRegs.ctrl & VI_CTRL_ANTIALIAS_MASK;
    }
}

extern "C" void osViBlack(uint8_t active) {
    vi_black = active;
}

extern "C" void osViSetXScale(float scale) {
    if (scale != 1.0f) {
        assert(false);
    }
}

extern "C" void osViSetYScale(float scale) {
    if (scale != 1.0f) {
        assert(false);
    }
}

extern "C" PTR(void) osViGetNextFramebuffer() {
    return events_context.vi.next_buffer;
}

extern "C" PTR(void) osViGetCurrentFramebuffer() {
    return events_context.vi.current_buffer;
}

void ultramodern::submit_rsp_task(RDRAM_ARG PTR(OSTask) task_) {
    OSTask* task = TO_PTR(OSTask, task_);

    // Send gfx tasks to the graphics action queue
    if (task->t.type == M_GFXTASK) {
        events_context.action_queue.enqueue(SpTaskAction{ *task });
    }
    // Set all other tasks as the RSP task
    else {
        events_context.sp_task_queue.enqueue(task);
    }
}

void ultramodern::send_si_message(RDRAM_ARG1) {
    osSendMesg(PASS_RDRAM events_context.si.mq, events_context.si.msg, OS_MESG_NOBLOCK);
}

std::string get_graphics_api_name(ultramodern::GraphicsApi api) {
    if (api == ultramodern::GraphicsApi::Auto) {
#if defined(_WIN32)
        api = ultramodern::GraphicsApi::D3D12;
#elif defined(__gnu_linux__)
        api = ultramodern::GraphicsApi::Vulkan;
#else
        static_assert(false && "Unimplemented")
#endif
    }

    switch (api) {
        case ultramodern::GraphicsApi::D3D12:
            return "D3D12";
        case ultramodern::GraphicsApi::Vulkan:
            return "Vulkan";
        default:
            return "[Unknown graphics API]";
    }
}

void ultramodern::init_events(RDRAM_ARG ultramodern::WindowHandle window_handle) {
    moodycamel::LightweightSemaphore gfx_thread_ready;
    moodycamel::LightweightSemaphore task_thread_ready;
    events_context.rdram = rdram;
    events_context.sp.gfx_thread = std::thread{ gfx_thread_func, rdram, &gfx_thread_ready, window_handle };
    events_context.sp.task_thread = std::thread{ task_thread_func, rdram, &task_thread_ready };
    
    // Wait for the two sp threads to be ready before continuing to prevent the game from
    // running before we're able to handle RSP tasks.
    gfx_thread_ready.wait();
    task_thread_ready.wait();

    ultramodern::RT64SetupResult setup_result = rt64_setup_result.load();
    if (rt64_setup_result != ultramodern::RT64SetupResult::Success) {
        auto show_rt64_error = [](const std::string& msg) {
            // TODO move recomp code out of ultramodern (message boxes).
            recomp::message_box(("An error has been encountered on startup: " + msg).c_str());
        };
        const std::string driver_os_suffix = "\nPlease make sure your GPU drivers and your OS are up to date.";
        switch (rt64_setup_result) {
            case ultramodern::RT64SetupResult::DynamicLibrariesNotFound:
                show_rt64_error("Failed to load dynamic libraries. Make sure the DLLs are next to the recomp executable.");
                break;
            case ultramodern::RT64SetupResult::InvalidGraphicsAPI:
                show_rt64_error(get_graphics_api_name(cur_config.load().api_option) + " is not supported on this platform. Please select a different graphics API.");
                break;
            case ultramodern::RT64SetupResult::GraphicsAPINotFound:
                show_rt64_error("Unable to initialize " + get_graphics_api_name(cur_config.load().api_option) + "." + driver_os_suffix);
                break;
            case ultramodern::RT64SetupResult::GraphicsDeviceNotFound:
                show_rt64_error("Unable to find compatible graphics device." + driver_os_suffix);
                break;
        }
        throw std::runtime_error("Failed to initialize RT64");
    }

    events_context.vi.thread = std::thread{ vi_thread_func };
}

void ultramodern::join_event_threads() {
    events_context.sp.gfx_thread.join();
    events_context.vi.thread.join();

    // Send a null RSP task to indicate that the RSP task thread should exit.
    events_context.sp_task_queue.enqueue(nullptr);
    events_context.sp.task_thread.join();
}
