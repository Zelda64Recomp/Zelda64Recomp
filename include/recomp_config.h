#ifndef __RECOMP_CONFIG_H__
#define __RECOMP_CONFIG_H__

#include <filesystem>
#include <string_view>
#include "../ultramodern/config.hpp"

namespace recomp {
    constexpr std::u8string_view program_id = u8"Zelda64Recompiled";
    constexpr std::u8string_view mm_game_id = u8"mm.n64.us.1.0";

    void load_config();
    void save_config();
    
    void reset_input_bindings();
    void reset_cont_input_bindings();
    void reset_kb_input_bindings();

    std::filesystem::path get_app_folder_path();
    
    bool get_debug_mode_enabled();
    void set_debug_mode_enabled(bool enabled);

    enum class AutosaveMode {
        On,
        Off,
		OptionCount
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(recomp::AutosaveMode, {
        {recomp::AutosaveMode::On, "On"},
        {recomp::AutosaveMode::Off, "Off"}
    });

    AutosaveMode get_autosave_mode();
    void set_autosave_mode(AutosaveMode mode);
};

#endif
