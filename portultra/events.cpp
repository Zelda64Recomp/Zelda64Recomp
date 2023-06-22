#include <thread>
#include <atomic>
#include <chrono>
#include <cinttypes>
#include <variant>
#include <unordered_map>
#include <utility>
#include <mutex>
#include <queue>

#include <Windows.h>
#include "blockingconcurrentqueue.h"

#include "ultra64.h"
#include "multilibultra.hpp"
#include "recomp.h"
#include "rsp.h"

struct SpTaskAction {
    OSTask task;
};

struct SwapBuffersAction {
    uint32_t origin;
};

using Action = std::variant<SpTaskAction, SwapBuffersAction>;

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
        std::thread thread;
        PTR(OSMesgQueue) mq = NULLPTR;
        OSMesg msg = (OSMesg)0;
    } dp;
    struct {
        std::thread thread;
        PTR(OSMesgQueue) mq = NULLPTR;
        OSMesg msg = (OSMesg)0;
    } ai;
    struct {
        std::thread thread;
        PTR(OSMesgQueue) mq = NULLPTR;
        OSMesg msg = (OSMesg)0;
    } si;
    // The same message queue may be used for multiple events, so share a mutex for all of them
    std::mutex message_mutex;
    uint8_t* rdram;
    moodycamel::BlockingConcurrentQueue<Action> action_queue{};
    std::atomic<OSTask*> sp_task = nullptr;
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

void vi_thread_func() {
    using namespace std::chrono_literals;
    
    uint64_t total_vis = 0;
    int remaining_retraces = events_context.vi.retrace_count;

    while (true) {
        // Determine the next VI time (more accurate than adding 16ms each VI interrupt)
        auto next = Multilibultra::get_start() + (total_vis * 1000000us) / (60 * Multilibultra::get_speed_multiplier());
        //if (next > std::chrono::system_clock::now()) {
        //    printf("Sleeping for %" PRIu64 " us to get from %" PRIu64 " us to %" PRIu64 " us \n",
        //        (next - std::chrono::system_clock::now()) / 1us,
        //        (std::chrono::system_clock::now() - events_context.start) / 1us,
        //        (next - events_context.start) / 1us);
        //} else {
        //    printf("No need to sleep\n");
        //}
        std::this_thread::sleep_until(next);
        // Calculate how many VIs have passed
        uint64_t new_total_vis = (Multilibultra::time_since_start() * (60 * Multilibultra::get_speed_multiplier()) / 1000ms) + 1;
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

                if (events_context.vi.mq != NULLPTR) {
                    if (osSendMesg(PASS_RDRAM events_context.vi.mq, events_context.vi.msg, OS_MESG_NOBLOCK) == -1) {
                        //printf("Game skipped a VI frame!\n");
                    }
                }
            }
            if (events_context.ai.mq != NULLPTR) {
                if (osSendMesg(PASS_RDRAM events_context.ai.mq, events_context.ai.msg, OS_MESG_NOBLOCK) == -1) {
                    //printf("Game skipped a AI frame!\n");
                }
            }
        }
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

void RT64Init(uint8_t* rom, uint8_t* rdram, void* window_handle);
void RT64SendDL(uint8_t* rdram, const OSTask* task);
void RT64UpdateScreen(uint32_t vi_origin);
void RT64ChangeWindow();

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
        rspReciprocals[index] = u16(b + 1 >> 8);
    }

    for (u16 index = 0; index < 512; index++) {
        u64 a = index + 512 >> ((index % 2 == 1) ? 1 : 0);
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


void task_thread_func(uint8_t* rdram, uint8_t* rom, std::atomic_flag* thread_ready) {
    // Notify the caller thread that this thread is ready.
    thread_ready->test_and_set();
    thread_ready->notify_all();

    while (1) {
        // Wait until an RSP task has been sent
        events_context.sp_task.wait(nullptr);

        // Retrieve the task pointer and clear the pending RSP task
        OSTask* task = events_context.sp_task;
        events_context.sp_task.store(nullptr);

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

void gfx_thread_func(uint8_t* rdram, uint8_t* rom, std::atomic_flag* thread_ready, void* window_handle) {
    using namespace std::chrono_literals;
    RT64Init(rom, rdram, window_handle);
    
    rsp_constants_init();

    // Notify the caller thread that this thread is ready.
    thread_ready->test_and_set();
    thread_ready->notify_all();

    while (true) {
        // Try to pull an action from the queue
        Action action;
        if (events_context.action_queue.wait_dequeue_timed(action, 1ms)) {
            // Determine the action type and act on it
            if (const auto* task_action = std::get_if<SpTaskAction>(&action)) {
                // Tell the game that the RSP completed instantly. This will allow it to queue other task types, but it won't
                // start another graphics task until the RDP is also complete. Games usually preserve the RSP inputs until the RDP
                // is finished as well, so sending this early shouldn't be an issue in most cases.
                // If this causes issues then the logic can be replaced with responding to yield requests.
                sp_complete();
                RT64SendDL(rdram, &task_action->task);
                dp_complete();
            } else if (const auto* swap_action = std::get_if<SwapBuffersAction>(&action)) {
                static volatile int i = 0;
                if (i >= 100) {
                    i = 0;
                }
                i++;
                events_context.vi.current_buffer = events_context.vi.next_buffer;
                RT64UpdateScreen(swap_action->origin);
            }
        }
    }
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

void Multilibultra::submit_rsp_task(RDRAM_ARG PTR(OSTask) task_) {
    OSTask* task = TO_PTR(OSTask, task_);

    // Send gfx tasks to the graphics action queue
    if (task->t.type == M_GFXTASK) {
        events_context.action_queue.enqueue(SpTaskAction{ *task });
    }
    // Set all other tasks as the RSP task
    else {
        events_context.sp_task.store(task);
        events_context.sp_task.notify_all();
    }
}

void Multilibultra::send_si_message() {
    uint8_t* rdram = events_context.rdram;
    osSendMesg(PASS_RDRAM events_context.si.mq, events_context.si.msg, OS_MESG_NOBLOCK);
}

void Multilibultra::init_events(uint8_t* rdram, uint8_t* rom, void* window_handle) {
    std::atomic_flag gfx_thread_ready;
    std::atomic_flag task_thread_ready;
    events_context.rdram = rdram;
    events_context.sp.gfx_thread = std::thread{ gfx_thread_func, rdram, rom, &gfx_thread_ready, window_handle };
    events_context.sp.task_thread = std::thread{ task_thread_func, rdram, rom, &task_thread_ready };
    
    // Wait for the two sp threads to be ready before continuing to prevent the game from
    // running before we're able to handle RSP tasks.
    gfx_thread_ready.wait(false);
    task_thread_ready.wait(false);

    events_context.vi.thread = std::thread{ vi_thread_func };
}
