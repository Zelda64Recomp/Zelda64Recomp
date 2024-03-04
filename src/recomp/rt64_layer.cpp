#include <memory>
#include <cstring>
// #include <Windows.h>

#define HLSL_CPU
#include "hle/rt64_application.h"
#include "rt64_layer.h"
#include "rt64_render_hooks.h"

typedef struct {
    // void* hWnd;
    // void* hStatusBar;

    // int Reserved;

    unsigned char* HEADER;  /* This is the rom header (first 40h bytes of the rom) */
    unsigned char* RDRAM;
    unsigned char* DMEM;
    unsigned char* IMEM;

    unsigned int* MI_INTR_REG;

    unsigned int* DPC_START_REG;
    unsigned int* DPC_END_REG;
    unsigned int* DPC_CURRENT_REG;
    unsigned int* DPC_STATUS_REG;
    unsigned int* DPC_CLOCK_REG;
    unsigned int* DPC_BUFBUSY_REG;
    unsigned int* DPC_PIPEBUSY_REG;
    unsigned int* DPC_TMEM_REG;

    unsigned int* VI_STATUS_REG;
    unsigned int* VI_ORIGIN_REG;
    unsigned int* VI_WIDTH_REG;
    unsigned int* VI_INTR_REG;
    unsigned int* VI_V_CURRENT_LINE_REG;
    unsigned int* VI_TIMING_REG;
    unsigned int* VI_V_SYNC_REG;
    unsigned int* VI_H_SYNC_REG;
    unsigned int* VI_LEAP_REG;
    unsigned int* VI_H_START_REG;
    unsigned int* VI_V_START_REG;
    unsigned int* VI_V_BURST_REG;
    unsigned int* VI_X_SCALE_REG;
    unsigned int* VI_Y_SCALE_REG;

    void (*CheckInterrupts)(void);

    unsigned int version;
    unsigned int* SP_STATUS_REG;
    const unsigned int* RDRAM_SIZE;
} GFX_INFO;

#ifdef _WIN32
#define DLLEXPORT extern "C" __declspec(dllexport)  
#define DLLIMPORT extern "C" __declspec(dllimport)
#define CALL   __cdecl
#else
#define DLLEXPORT extern "C" __attribute__((visibility("default")))
#define DLLIMPORT extern "C"
#endif

#if defined(_WIN32)
extern "C" RT64::Application* InitiateGFXWindows(GFX_INFO Gfx_Info, HWND hwnd, DWORD threadId, uint8_t debug);
#elif defined(__ANDROID__)
static_assert(false && "Unimplemented");
#elif defined(__linux__)
extern "C" RT64::Application* InitiateGFXLinux(GFX_INFO Gfx_Info, Window window, Display *display, uint8_t debug);
#else
static_assert(false && "Unimplemented");
#endif

DLLIMPORT void ProcessRDPList(void);
DLLIMPORT void ProcessDList(void);
DLLIMPORT void UpdateScreen(void);
DLLIMPORT void ChangeWindow(void);
DLLIMPORT void PluginShutdown(void);

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

static RT64::UserConfiguration::Antialiasing device_max_msaa = RT64::UserConfiguration::Antialiasing::None;

#define GET_FUNC(lib, name) \
    name = (decltype(name))GetProcAddress(lib, #name)

void dummy_check_interrupts() {

}

RT64::UserConfiguration::Antialiasing compute_max_supported_aa(RT64::RenderSampleCounts bits) {
    if (bits & RT64::RenderSampleCount::Bits::COUNT_2) {
        if (bits & RT64::RenderSampleCount::Bits::COUNT_4) {
            if (bits & RT64::RenderSampleCount::Bits::COUNT_8) {
                return RT64::UserConfiguration::Antialiasing::MSAA8X;
            }
            return RT64::UserConfiguration::Antialiasing::MSAA4X;
        }
        return RT64::UserConfiguration::Antialiasing::MSAA2X;
    };
    return RT64::UserConfiguration::Antialiasing::None;
}

RT64::Application* RT64Init(uint8_t* rdram, ultramodern::WindowHandle window_handle, bool debug) {
    static unsigned char dummy_rom_header[0x40];
    set_rt64_hooks();

    GFX_INFO gfx_info{};

    gfx_info.HEADER = dummy_rom_header;
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
    RT64::Application* ret = InitiateGFXWindows(gfx_info, window_handle.window, window_handle.thread_id, debug);
#elif defined(__ANDROID__)
    static_assert(false && "Unimplemented");
#elif defined(__linux__)
	RT64::Application* ret = InitiateGFXLinux(gfx_info, window_handle.window, window_handle.display, debug);
#else
    static_assert(false && "Unimplemented");
#endif

    // Before configuring multisampling, make sure the device actually supports it for the formats we'll use. If it doesn't, turn off antialiasing in the configuration.
    RT64::RenderSampleCounts color_sample_counts = ret->device->getSampleCountsSupported(RT64::RenderFormat::R8G8B8A8_UNORM);
    RT64::RenderSampleCounts depth_sample_counts = ret->device->getSampleCountsSupported(RT64::RenderFormat::D32_FLOAT);
    RT64::RenderSampleCounts common_sample_counts = color_sample_counts & depth_sample_counts;

    device_max_msaa = compute_max_supported_aa(common_sample_counts);

    // Force gbi depth branches to prevent LODs from kicking in.
    ret->enhancementConfig.f3dex.forceBranch = true;
    // Scale LODs based on the output resolution.
    ret->enhancementConfig.textureLOD.scale = true;

    ret->updateEnhancementConfig();

    return ret;
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

void RT64UpdateConfig(RT64::Application* application, const ultramodern::GraphicsConfig& old_config, const ultramodern::GraphicsConfig& new_config) {
    if (new_config.wm_option != old_config.wm_option) {
        application->setFullScreen(new_config.wm_option == ultramodern::WindowMode::Fullscreen);
    }

    switch (new_config.res_option) {
        default:
        case ultramodern::Resolution::Auto:
            application->userConfig.resolution = RT64::UserConfiguration::Resolution::WindowIntegerScale;
            break;
        case ultramodern::Resolution::Original:
            application->userConfig.resolution = RT64::UserConfiguration::Resolution::Original;
            break;
        case ultramodern::Resolution::Original2x:
            application->userConfig.resolution = RT64::UserConfiguration::Resolution::Manual;
            application->userConfig.resolutionMultiplier = 2.0;
            break;
    }

    application->userConfig.aspectRatio = new_config.ar_option;
    application->userConfig.antialiasing = new_config.msaa_option;
    application->userConfig.refreshRate = new_config.rr_option;
    application->userConfig.refreshRateTarget = new_config.rr_manual_value;

    application->updateUserConfig(true);

    if (new_config.msaa_option != old_config.msaa_option) {
        application->updateMultisampling();
    }
}

void RT64EnableInstantPresent(RT64::Application* application) {
    // Enable the present early presentation mode for minimal latency.
    application->enhancementConfig.presentation.mode = RT64::EnhancementConfiguration::Presentation::Mode::PresentEarly;

    application->updateEnhancementConfig();
}

RT64::UserConfiguration::Antialiasing RT64MaxMSAA() {
    return device_max_msaa;
}

uint32_t RT64GetDisplayFramerate(RT64::Application* application) {
    return application->presentQueue->ext.sharedResources->swapChainRate;
}
