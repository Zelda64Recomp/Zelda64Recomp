#ifndef __ZELDA_CONFIG_H__
#define __ZELDA_CONFIG_H__

#include <filesystem>
#include <string>

namespace zelda64 {
    inline const std::u8string program_id = u8"Zelda64Recompiled";
    inline const std::string program_name = "Zelda 64: Recompiled";

    namespace configkeys {
        namespace general {
            inline const std::string debug_mode = "debug_mode";
            inline const std::string targeting_mode = "targeting_mode";
            inline const std::string autosave_mode = "autosave_mode";
            inline const std::string camera_invert_mode = "camera_invert_mode";
            inline const std::string analog_cam_mode = "analog_cam_mode";
            inline const std::string analog_camera_invert_mode = "analog_camera_invert_mode";
        }

        namespace sound {
            inline const std::string bgm_volume = "bgm_volume";
            inline const std::string low_health_beeps = "low_health_beeps";
        }

        namespace graphics {
        }
    }

    // TODO: Move loading configs to the runtime once we have a way to allow per-project customization.
    void init_config();
    
    bool get_debug_mode_enabled();

    enum class AutosaveMode {
        Off,
        On,
    };

    AutosaveMode get_autosave_mode();

    enum class TargetingMode {
        Switch,
        Hold
    };
    
    TargetingMode get_targeting_mode();

    enum class CameraInvertMode {
        InvertNone,
        InvertX,
        InvertY,
        InvertBoth
    };
    
    CameraInvertMode get_camera_invert_mode();
    CameraInvertMode get_analog_camera_invert_mode();

    enum class AnalogCamMode {
        Off,
        On,
    };
    
    AnalogCamMode get_analog_cam_mode();

    enum class LowHealthBeepsMode {
        Off,
        On,
    };
};

#endif
