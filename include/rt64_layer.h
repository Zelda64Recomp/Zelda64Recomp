#ifndef __RT64_LAYER_H__
#define __RT64_LAYER_H__

#include "../ultramodern/ultramodern.hpp"
#include "../ultramodern/config.hpp"

namespace RT64 {
    struct Application;
}

namespace ultramodern {
    enum class RT64SetupResult {
        Success,
        DynamicLibrariesNotFound,
        InvalidGraphicsAPI,
        GraphicsAPINotFound,
        GraphicsDeviceNotFound
    };

    struct WindowHandle;
    struct RT64Context {
        public:
            ~RT64Context();
            RT64Context(uint8_t* rdram, WindowHandle window_handle, bool developer_mode);
            bool valid() { return static_cast<bool>(app); }
            RT64SetupResult get_setup_result() { return setup_result; }

            void update_config(const GraphicsConfig& old_config, const GraphicsConfig& new_config);
            void enable_instant_present();
            void send_dl(const OSTask* task);
            void update_screen(uint32_t vi_origin);
            void shutdown();
            void set_dummy_vi();
            uint32_t get_display_framerate();
            float get_resolution_scale();
            void load_shader_cache(std::span<const char> cache_binary);
        private:
            RT64SetupResult setup_result;
            std::unique_ptr<RT64::Application> app;
    };
    
    RT64::UserConfiguration::Antialiasing RT64MaxMSAA();
    bool RT64SamplePositionsSupported();
    bool RT64HighPrecisionFBEnabled();
}

void set_rt64_hooks();

#endif

