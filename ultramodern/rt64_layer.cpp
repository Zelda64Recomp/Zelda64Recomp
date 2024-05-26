#include <memory>
#include <cstring>
// #include <Windows.h>

#define HLSL_CPU
#include "hle/rt64_application.h"
#include "rt64_layer.h"
#include "rt64_render_hooks.h"

ultramodern::RT64Context::~RT64Context() = default;

static RT64::UserConfiguration::Antialiasing device_max_msaa = RT64::UserConfiguration::Antialiasing::None;
static bool sample_positions_supported = false;
static bool high_precision_fb_enabled = false;

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

RT64::UserConfiguration::InternalColorFormat to_rt64(ultramodern::HighPrecisionFramebuffer option) {
    switch (option) {
        case ultramodern::HighPrecisionFramebuffer::Off:
            return RT64::UserConfiguration::InternalColorFormat::Standard;
        case ultramodern::HighPrecisionFramebuffer::On:
            return RT64::UserConfiguration::InternalColorFormat::High;
        case ultramodern::HighPrecisionFramebuffer::Auto:
            return RT64::UserConfiguration::InternalColorFormat::Automatic;
        default:
            return RT64::UserConfiguration::InternalColorFormat::OptionCount;
    }
}

void set_application_user_config(RT64::Application* application, const ultramodern::GraphicsConfig& config) {
    switch (config.res_option) {
        default:
        case ultramodern::Resolution::Auto:
            application->userConfig.resolution = RT64::UserConfiguration::Resolution::WindowIntegerScale;
            application->userConfig.downsampleMultiplier = 1;
            break;
        case ultramodern::Resolution::Original:
            application->userConfig.resolution = RT64::UserConfiguration::Resolution::Manual;
            application->userConfig.resolutionMultiplier = config.ds_option;
            application->userConfig.downsampleMultiplier = config.ds_option;
            break;
        case ultramodern::Resolution::Original2x:
            application->userConfig.resolution = RT64::UserConfiguration::Resolution::Manual;
            application->userConfig.resolutionMultiplier = 2.0 * config.ds_option;
            application->userConfig.downsampleMultiplier = config.ds_option;
            break;
    }

    switch (config.hr_option) {
        default:
        case ultramodern::HUDRatioMode::Original:
            application->userConfig.extAspectRatio = RT64::UserConfiguration::AspectRatio::Original;
            break;
        case ultramodern::HUDRatioMode::Clamp16x9:
            application->userConfig.extAspectRatio = RT64::UserConfiguration::AspectRatio::Manual;
            application->userConfig.extAspectTarget = 16.0/9.0;
            break;
        case ultramodern::HUDRatioMode::Full:
            application->userConfig.extAspectRatio = RT64::UserConfiguration::AspectRatio::Expand;
            break;
    }

    application->userConfig.aspectRatio = config.ar_option;
    application->userConfig.antialiasing = config.msaa_option;
    application->userConfig.refreshRate = config.rr_option;
    application->userConfig.refreshRateTarget = config.rr_manual_value;
    application->userConfig.internalColorFormat = to_rt64(config.hpfb_option);
}

ultramodern::RT64SetupResult map_setup_result(RT64::Application::SetupResult rt64_result) {
    switch (rt64_result) {
        case RT64::Application::SetupResult::Success:
            return ultramodern::RT64SetupResult::Success;
        case RT64::Application::SetupResult::DynamicLibrariesNotFound:
            return ultramodern::RT64SetupResult::DynamicLibrariesNotFound;
        case RT64::Application::SetupResult::InvalidGraphicsAPI:
            return ultramodern::RT64SetupResult::InvalidGraphicsAPI;
        case RT64::Application::SetupResult::GraphicsAPINotFound:
            return ultramodern::RT64SetupResult::GraphicsAPINotFound;
        case RT64::Application::SetupResult::GraphicsDeviceNotFound:
            return ultramodern::RT64SetupResult::GraphicsDeviceNotFound;
    }
}

ultramodern::RT64Context::RT64Context(uint8_t* rdram, ultramodern::WindowHandle window_handle, bool debug) {
    static unsigned char dummy_rom_header[0x40];
    set_rt64_hooks();

    // Set up the RT64 application core fields.
    RT64::Application::Core appCore{};
#if defined(_WIN32)
    appCore.window = window_handle.window;
#elif defined(__ANDROID__)
    assert(false && "Unimplemented");
#elif defined(__linux__)
    appCore.window.display = window_handle.display;
    appCore.window.window = window_handle.window;
#endif

    appCore.checkInterrupts = dummy_check_interrupts;

    appCore.HEADER = dummy_rom_header;
    appCore.RDRAM = rdram;
    appCore.DMEM = DMEM;
    appCore.IMEM = IMEM;

    appCore.MI_INTR_REG = &MI_INTR_REG;

    appCore.DPC_START_REG = &DPC_START_REG;
    appCore.DPC_END_REG = &DPC_END_REG;
    appCore.DPC_CURRENT_REG = &DPC_CURRENT_REG;
    appCore.DPC_STATUS_REG = &DPC_STATUS_REG;
    appCore.DPC_CLOCK_REG = &DPC_CLOCK_REG;
    appCore.DPC_BUFBUSY_REG = &DPC_BUFBUSY_REG;
    appCore.DPC_PIPEBUSY_REG = &DPC_PIPEBUSY_REG;
    appCore.DPC_TMEM_REG = &DPC_TMEM_REG;

    appCore.VI_STATUS_REG = &VI_STATUS_REG;
    appCore.VI_ORIGIN_REG = &VI_ORIGIN_REG;
    appCore.VI_WIDTH_REG = &VI_WIDTH_REG;
    appCore.VI_INTR_REG = &VI_INTR_REG;
    appCore.VI_V_CURRENT_LINE_REG = &VI_V_CURRENT_LINE_REG;
    appCore.VI_TIMING_REG = &VI_TIMING_REG;
    appCore.VI_V_SYNC_REG = &VI_V_SYNC_REG;
    appCore.VI_H_SYNC_REG = &VI_H_SYNC_REG;
    appCore.VI_LEAP_REG = &VI_LEAP_REG;
    appCore.VI_H_START_REG = &VI_H_START_REG;
    appCore.VI_V_START_REG = &VI_V_START_REG;
    appCore.VI_V_BURST_REG = &VI_V_BURST_REG;
    appCore.VI_X_SCALE_REG = &VI_X_SCALE_REG;
    appCore.VI_Y_SCALE_REG = &VI_Y_SCALE_REG;

    // Set up the RT64 application configuration fields.
    RT64::ApplicationConfiguration appConfig;
    appConfig.useConfigurationFile = false;

    // Create the RT64 application.
    app = std::make_unique<RT64::Application>(appCore, appConfig);

    // Set initial user config settings based on the current settings.
    ultramodern::GraphicsConfig cur_config = ultramodern::get_graphics_config();
    set_application_user_config(app.get(), cur_config);
    app->userConfig.developerMode = debug;
    // Force gbi depth branches to prevent LODs from kicking in.
    app->enhancementConfig.f3dex.forceBranch = true;
    // Scale LODs based on the output resolution.
    app->enhancementConfig.textureLOD.scale = true;
    // Pick an API if the user has set an override.
    switch (cur_config.api_option) {
        case ultramodern::GraphicsApi::D3D12:
            app->userConfig.graphicsAPI = RT64::UserConfiguration::GraphicsAPI::D3D12;
            break;
        case ultramodern::GraphicsApi::Vulkan:
            app->userConfig.graphicsAPI = RT64::UserConfiguration::GraphicsAPI::Vulkan;
            break;
        default:
        case ultramodern::GraphicsApi::Auto:
            // Don't override if auto is selected.
            break;
    }

    // Set up the RT64 application.
    uint32_t thread_id = 0;
#ifdef _WIN32
    thread_id = window_handle.thread_id;
#endif
    setup_result = map_setup_result(app->setup(thread_id));
    if (setup_result != ultramodern::RT64SetupResult::Success) {
        app = nullptr;
        return;
    }

    // Set the application's fullscreen state.
    app->setFullScreen(cur_config.wm_option == ultramodern::WindowMode::Fullscreen);

    // Check if the selected device actually supports MSAA sample positions and MSAA for for the formats that will be used
    // and downgrade the configuration accordingly.
    if (app->device->getCapabilities().sampleLocations) {
        RT64::RenderSampleCounts color_sample_counts = app->device->getSampleCountsSupported(RT64::RenderFormat::R8G8B8A8_UNORM);
        RT64::RenderSampleCounts depth_sample_counts = app->device->getSampleCountsSupported(RT64::RenderFormat::D32_FLOAT);
        RT64::RenderSampleCounts common_sample_counts = color_sample_counts & depth_sample_counts;
        device_max_msaa = compute_max_supported_aa(common_sample_counts);
        sample_positions_supported = true;
    }
    else {
        device_max_msaa = RT64::UserConfiguration::Antialiasing::None;
        sample_positions_supported = false;
    }

    high_precision_fb_enabled = app->shaderLibrary->usesHDR;
}

void ultramodern::RT64Context::send_dl(const OSTask* task) {
    app->state->rsp->reset();
    app->interpreter->loadUCodeGBI(task->t.ucode & 0x3FFFFFF, task->t.ucode_data & 0x3FFFFFF, true);
    app->processDisplayLists(app->core.RDRAM, task->t.data_ptr & 0x3FFFFFF, 0, true);
}

void ultramodern::RT64Context::update_screen(uint32_t vi_origin) {
    VI_ORIGIN_REG = vi_origin;

    app->updateScreen();
}

void ultramodern::RT64Context::shutdown() {
    if (app != nullptr) {
        app->end();
    }
}

void ultramodern::RT64Context::update_config(const ultramodern::GraphicsConfig& old_config, const ultramodern::GraphicsConfig& new_config) {
    if (new_config.wm_option != old_config.wm_option) {
        app->setFullScreen(new_config.wm_option == ultramodern::WindowMode::Fullscreen);
    }

    set_application_user_config(app.get(), new_config);

    app->updateUserConfig(true);

    if (new_config.msaa_option != old_config.msaa_option) {
        app->updateMultisampling();
    }
}

void ultramodern::RT64Context::enable_instant_present() {
    // Enable the present early presentation mode for minimal latency.
    app->enhancementConfig.presentation.mode = RT64::EnhancementConfiguration::Presentation::Mode::PresentEarly;

    app->updateEnhancementConfig();
}

uint32_t ultramodern::RT64Context::get_display_framerate() {
    return app->presentQueue->ext.sharedResources->swapChainRate;
}

float ultramodern::RT64Context::get_resolution_scale() {
    constexpr int ReferenceHeight = 240;
    switch (app->userConfig.resolution) {
        case RT64::UserConfiguration::Resolution::WindowIntegerScale:
            if (app->sharedQueueResources->swapChainHeight > 0) {
                return std::max(float((app->sharedQueueResources->swapChainHeight + ReferenceHeight - 1) / ReferenceHeight), 1.0f);
            }
            else {
                return 1.0f;
            }
        case RT64::UserConfiguration::Resolution::Manual:
            return float(app->userConfig.resolutionMultiplier);
        case RT64::UserConfiguration::Resolution::Original:
        default:
            return 1.0f;
    }
}

void ultramodern::RT64Context::load_shader_cache(std::span<const char> cache_binary) {
    // TODO figure out how to avoid a copy here.
    std::istringstream cache_stream{std::string{cache_binary.data(), cache_binary.size()}};

    if (!app->rasterShaderCache->loadOfflineList(cache_stream)) {
       printf("Failed to preload shader cache!\n");
       assert(false);
    }
}

RT64::UserConfiguration::Antialiasing ultramodern::RT64MaxMSAA() {
    return device_max_msaa;
}

bool ultramodern::RT64SamplePositionsSupported() {
    return sample_positions_supported;
}

bool ultramodern::RT64HighPrecisionFBEnabled() {
    return high_precision_fb_enabled;
}
