#include <memory>
#include <cstring>
// #include <Windows.h>

#include "../ultramodern/ultramodern.hpp"
#include "rt64_layer.h"
#include "rt64_render_hooks.h"

static uint8_t DMEM[0x1000];
static uint8_t IMEM[0x1000];

unsigned int MI_INTR_REG = 0;

unsigned int DPC_START_REG = 0;
unsigned int DPC_END_REG = 0;
unsigned int DPC_CURRENT_REG = 0;
unsigned int DPC_STATUS_REG = 0;
unsigned int DPC_CLOCK_REG = 0;
unsigned int DPC_BUFBUSY_REG = 0;
unsigned int DPC_PIPEBUSY_REG = 0;
unsigned int DPC_TMEM_REG = 0;

unsigned int VI_STATUS_REG = 0;
unsigned int VI_ORIGIN_REG = 0;
unsigned int VI_WIDTH_REG = 0;
unsigned int VI_INTR_REG = 0;
unsigned int VI_V_CURRENT_LINE_REG = 0;
unsigned int VI_TIMING_REG = 0;
unsigned int VI_V_SYNC_REG = 0;
unsigned int VI_H_SYNC_REG = 0;
unsigned int VI_LEAP_REG = 0;
unsigned int VI_H_START_REG = 0;
unsigned int VI_V_START_REG = 0;
unsigned int VI_V_BURST_REG = 0;
unsigned int VI_X_SCALE_REG = 0;
unsigned int VI_Y_SCALE_REG = 0;

unsigned int SP_STATUS_REG = 0;
unsigned int RDRAM_SIZE = 0x800000;

#define GET_FUNC(lib, name) \
    name = (decltype(name))GetProcAddress(lib, #name)

void dummy_check_interrupts() {

}

void RT64Init(uint8_t* rom, uint8_t* rdram, ultramodern::WindowHandle window_handle) {
    set_rt64_hooks();
    // Dynamic loading
    //auto RT64 = LoadLibrary("RT64.dll");
    //if (RT64 == 0) {
    //    fprintf(stdout, "Failed to load RT64\n");
    //    std::exit(EXIT_FAILURE);
    //}
    //GET_FUNC(RT64, InitiateGFX);
    //GET_FUNC(RT64, ProcessRDPList);
    //GET_FUNC(RT64, ProcessDList);
    //GET_FUNC(RT64, UpdateScreen);

    GFX_INFO gfx_info{};
    // gfx_info.hWnd = window_handle;
    // gfx_info.hStatusBar = nullptr;

    gfx_info.HEADER = rom;
    gfx_info.RDRAM = rdram;
    gfx_info.DMEM = DMEM;
    gfx_info.IMEM = IMEM;

    gfx_info.MI_INTR_REG = &MI_INTR_REG;

    gfx_info.DPC_START_REG = &DPC_START_REG;
    gfx_info.DPC_END_REG = &DPC_END_REG;
    gfx_info.DPC_CURRENT_REG = &DPC_CURRENT_REG;
    gfx_info.DPC_STATUS_REG = &DPC_STATUS_REG;
    gfx_info.DPC_CLOCK_REG = &DPC_CLOCK_REG;
    gfx_info.DPC_BUFBUSY_REG = &DPC_BUFBUSY_REG;
    gfx_info.DPC_PIPEBUSY_REG = &DPC_PIPEBUSY_REG;
    gfx_info.DPC_TMEM_REG = &DPC_TMEM_REG;

    gfx_info.VI_STATUS_REG = &VI_STATUS_REG;
    gfx_info.VI_ORIGIN_REG = &VI_ORIGIN_REG;
    gfx_info.VI_WIDTH_REG = &VI_WIDTH_REG;
    gfx_info.VI_INTR_REG = &VI_INTR_REG;
    gfx_info.VI_V_CURRENT_LINE_REG = &VI_V_CURRENT_LINE_REG;
    gfx_info.VI_TIMING_REG = &VI_TIMING_REG;
    gfx_info.VI_V_SYNC_REG = &VI_V_SYNC_REG;
    gfx_info.VI_H_SYNC_REG = &VI_H_SYNC_REG;
    gfx_info.VI_LEAP_REG = &VI_LEAP_REG;
    gfx_info.VI_H_START_REG = &VI_H_START_REG;
    gfx_info.VI_V_START_REG = &VI_V_START_REG;
    gfx_info.VI_V_BURST_REG = &VI_V_BURST_REG;
    gfx_info.VI_X_SCALE_REG = &VI_X_SCALE_REG;
    gfx_info.VI_Y_SCALE_REG = &VI_Y_SCALE_REG;

    gfx_info.CheckInterrupts = dummy_check_interrupts;

    gfx_info.version = 2;
    gfx_info.SP_STATUS_REG = &SP_STATUS_REG;
    gfx_info.RDRAM_SIZE = &RDRAM_SIZE;

#if defined(_WIN32)
    InitiateGFXWindows(gfx_info, window_handle.window, window_handle.thread_id);
#elif defined(__ANDROID__)
    static_assert(false && "Unimplemented");
#elif defined(__linux__)
	InitiateGFXLinux(gfx_info, window_handle.window, window_handle.display);
#else
    static_assert(false && "Unimplemented");
#endif
}

void RT64SendDL(uint8_t* rdram, const OSTask* task) {
    OSTask task_copy = *task;
    task_copy.t.data_ptr &= 0x3FFFFFF;
    task_copy.t.ucode &= 0x3FFFFFF;
    task_copy.t.ucode_data &= 0x3FFFFFF;

    memcpy(DMEM + 0xFC0, &task_copy, 0x40);

    ProcessDList();
}

void RT64UpdateScreen(uint32_t vi_origin) {
    VI_ORIGIN_REG = vi_origin;

    UpdateScreen();
}

void RT64ChangeWindow() {
    ChangeWindow();
}

void RT64Shutdown() {
    PluginShutdown();
}
