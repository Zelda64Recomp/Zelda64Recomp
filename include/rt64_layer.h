#ifndef __RT64_LAYER_H__
#define __RT64_LAYER_H__

#include "../ultramodern/ultramodern.hpp"
#include "../ultramodern/config.hpp"

namespace RT64 {
    struct Application;
}

namespace ultramodern {
    struct WindowHandle;
}

RT64::Application* RT64Init(uint8_t* rom, uint8_t* rdram, ultramodern::WindowHandle window_handle, bool developer_mode);
void RT64UpdateConfig(RT64::Application* application, const ultramodern::GraphicsConfig& old_config, const ultramodern::GraphicsConfig& new_config);
void RT64EnableInstantPresent(RT64::Application* application);
void RT64SendDL(uint8_t* rdram, const OSTask* task);
void RT64UpdateScreen(uint32_t vi_origin);
void RT64ChangeWindow();
void RT64Shutdown();
RT64::UserConfiguration::Antialiasing RT64MaxMSAA();
uint32_t RT64GetDisplayFramerate(RT64::Application* application);

void set_rt64_hooks();

#endif

