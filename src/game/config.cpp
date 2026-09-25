#include "zelda_config.h"
#include "zelda_debug.h"
#include "recompui/recompui.h"
#include "recompui/config.h"
#include "recompinput/recompinput.h"
#include "zelda_sound.h"
#include "zelda_support.h"
#include "ultramodern/config.hpp"
#include "librecomp/files.hpp"
#include "librecomp/config.hpp"
#include "util/file.h"
#include <filesystem>
#include <fstream>
#include <iomanip>

#if defined(_WIN32)
#include <Shlobj.h>
#elif defined(__linux__)
#include <unistd.h>
#include <pwd.h>
#elif defined(__APPLE__)
#include "apple/rt64_apple.h"
#endif

bool get_general_config_bool_value(const std::string& option_id) {
    return std::get<bool>(recompui::config::get_general_config().get_option_value(option_id));
}

template <typename T = uint32_t>
T get_general_config_enum_value(const std::string& option_id) {
    return static_cast<T>(std::get<uint32_t>(recompui::config::get_general_config().get_option_value(option_id)));
}

template <typename T = uint32_t>
T get_general_config_number_value(const std::string& option_id) {
    return static_cast<T>(std::get<double>(recompui::config::get_general_config().get_option_value(option_id)));
}

template <typename T = uint32_t>
T get_graphics_config_enum_value(const std::string& option_id) {
    return static_cast<T>(std::get<uint32_t>(recompui::config::get_graphics_config().get_option_value(option_id)));
}

template <typename T = uint32_t>
T get_graphics_config_number_value(const std::string& option_id) {
    return static_cast<T>(std::get<double>(recompui::config::get_graphics_config().get_option_value(option_id)));
}

template <typename T = uint32_t>
T get_sound_config_enum_value(const std::string& option_id) {
    return static_cast<T>(std::get<uint32_t>(recompui::config::get_sound_config().get_option_value(option_id)));
}

template <typename T = uint32_t>
T get_sound_config_number_value(const std::string& option_id) {
    return static_cast<T>(std::get<double>(recompui::config::get_sound_config().get_option_value(option_id)));
}

static void add_general_options(recomp::config::Config &config) {
    using EnumOptionVector = const std::vector<recomp::config::ConfigOptionEnumOption>;

    // debug_mode
    config.add_bool_option(
        zelda64::configkeys::general::debug_mode,
        "Debug Mode",
        "Controls whether the debug menu is visible",
        false,
        true
    );

    // targeting_mode
    static EnumOptionVector targeting_mode_options = {
        {zelda64::TargetingMode::Switch, "Switch"},
        {zelda64::TargetingMode::Hold, "Hold"},
    };
    config.add_enum_option(
        zelda64::configkeys::general::targeting_mode,
        "Targeting Mode",
        "Controls how targeting enemies and objects works. <recomp-color primary>Switch</recomp-color> will start or stop targeting each time the target button is pressed. <recomp-color primary>Hold</recomp-color> will start when the target button is pressed and stop when the button is released.",
        targeting_mode_options,
        zelda64::TargetingMode::Switch
    );

    // autosave_mode
    static EnumOptionVector autosave_mode_options = {
        {zelda64::AutosaveMode::Off, "Off"},
        {zelda64::AutosaveMode::On, "On"},
    };
    config.add_enum_option(
        zelda64::configkeys::general::autosave_mode,
        "Autosaving",
        "Turns on autosaving and prevents owl saves from being deleted on load. Autosaves act as owl saves and take up the same slot as they do.\n"
        "\n"
        "Loading an autosave will place you in Clock Town or at the entrance of the current dungeon if you were in one.\n"
        "\n"
        "\n"
        "<recomp-color primary>If autosaving is disabled, existing autosaves will be deleted when loaded.</recomp-color>",
        autosave_mode_options,
        zelda64::AutosaveMode::On
    );

    // camera_invert_mode
    static EnumOptionVector camera_invert_mode_options = {
        {zelda64::CameraInvertMode::InvertNone, "None"},
        {zelda64::CameraInvertMode::InvertX, "Invert X"},
        {zelda64::CameraInvertMode::InvertY, "Invert Y"},
        {zelda64::CameraInvertMode::InvertBoth, "Invert Both"},
    };
    config.add_enum_option(
        zelda64::configkeys::general::camera_invert_mode,
        "Aiming Camera Mode",
        "Inverts the camera controls for first-person aiming. <recomp-color primary>Invert Y</recomp-color> is the default and matches the original game.",
        camera_invert_mode_options,
        zelda64::CameraInvertMode::InvertY
    );

    // analog_cam_mode
    static EnumOptionVector analog_cam_mode_options = {
        {zelda64::AnalogCamMode::Off, "Off"},
        {zelda64::AnalogCamMode::On, "On"},
    };
    config.add_enum_option(
        zelda64::configkeys::general::analog_cam_mode,
        "Analog Camera",
        "Enables an analog \"free\" camera similar to later entries in the series that's mapped to the right analog stick on the controller.\n"
        "\n"
        "When you move the right stick, the camera will enter free mode and stop centering behind Link. Press the <recomp-color primary>Target</recomp-color> button at any time to go back into the normal camera mode. The camera will also return to normal mode after a cutscene plays or when you move between areas.\n"
        "\n"
        "This option also enables right stick control while looking and aiming.",
        analog_cam_mode_options,
        zelda64::AnalogCamMode::Off
    );

    // analog_camera_invert_mode
    static EnumOptionVector analog_camera_invert_mode_options = {
        {zelda64::CameraInvertMode::InvertNone, "None"},
        {zelda64::CameraInvertMode::InvertX, "Invert X"},
        {zelda64::CameraInvertMode::InvertY, "Invert Y"},
        {zelda64::CameraInvertMode::InvertBoth, "Invert Both"},
    };
    config.add_enum_option(
        zelda64::configkeys::general::analog_camera_invert_mode,
        "Analog Camera Mode",
        "Inverts the camera controls for the analog camera if it's enabled. <recomp-color primary>None</recomp-color> is the default.",
        analog_camera_invert_mode_options,
        zelda64::CameraInvertMode::InvertNone
    );
}

bool zelda64::get_debug_mode_enabled() {
    return get_general_config_bool_value(zelda64::configkeys::general::debug_mode);
}

zelda64::AutosaveMode zelda64::get_autosave_mode() {
    return get_general_config_enum_value<zelda64::AutosaveMode>(zelda64::configkeys::general::autosave_mode);
}

zelda64::TargetingMode zelda64::get_targeting_mode() {
    return get_general_config_enum_value<zelda64::TargetingMode>(zelda64::configkeys::general::targeting_mode);
}

zelda64::CameraInvertMode zelda64::get_camera_invert_mode() {
    return get_general_config_enum_value<zelda64::CameraInvertMode>(zelda64::configkeys::general::camera_invert_mode);
}

zelda64::AnalogCamMode zelda64::get_analog_cam_mode() {
    return get_general_config_enum_value<zelda64::AnalogCamMode>(zelda64::configkeys::general::analog_cam_mode);
}

zelda64::CameraInvertMode zelda64::get_analog_camera_invert_mode() {
    return get_general_config_enum_value<zelda64::CameraInvertMode>(zelda64::configkeys::general::analog_camera_invert_mode);
}

static void add_graphics_options(recomp::config::Config &config) {
    using EnumOptionVector = const std::vector<recomp::config::ConfigOptionEnumOption>;

    // Nothing to do here.
}

static void add_sound_options(recomp::config::Config &config) {
    using EnumOptionVector = const std::vector<recomp::config::ConfigOptionEnumOption>;

    // bgm_volume
    config.add_percent_number_option(
        zelda64::configkeys::sound::bgm_volume,
        "Background Music Volume",
        "Controls the overall volume of background music.",
        100.0f
    );

    // low_health_beeps
    static EnumOptionVector low_health_beeps_mode_options = {
        {zelda64::LowHealthBeepsMode::Off, "Off"},
        {zelda64::LowHealthBeepsMode::On, "On"},
    };
    config.add_enum_option(
        zelda64::configkeys::sound::low_health_beeps,
        "Low Health Beeps",
        "Toggles whether or not the low-health beeping sound plays.",
        low_health_beeps_mode_options,
        zelda64::LowHealthBeepsMode::On
    );
}

int zelda64::get_bgm_volume() {
    return get_sound_config_number_value<int>(zelda64::configkeys::sound::bgm_volume);
}

bool zelda64::get_low_health_beeps_enabled() {
    return get_sound_config_enum_value<zelda64::LowHealthBeepsMode>(zelda64::configkeys::sound::low_health_beeps) == zelda64::LowHealthBeepsMode::On;
}

static void set_control_defaults() {
    // Nothing to do here.
}

static void set_control_descriptions() {
    // TODO descriptions
    recompinput::set_game_input_description(recompinput::GameInput::Y_AXIS_POS, "TODO description");
    recompinput::set_game_input_description(recompinput::GameInput::Y_AXIS_NEG, "TODO description");
    recompinput::set_game_input_description(recompinput::GameInput::X_AXIS_NEG, "TODO description");
    recompinput::set_game_input_description(recompinput::GameInput::X_AXIS_POS, "TODO description");
    recompinput::set_game_input_description(recompinput::GameInput::A,          "TODO description");
    recompinput::set_game_input_description(recompinput::GameInput::B,          "TODO description");
    recompinput::set_game_input_description(recompinput::GameInput::Z,          "TODO description");
    recompinput::set_game_input_description(recompinput::GameInput::L,          "TODO description");
    recompinput::set_game_input_description(recompinput::GameInput::R,          "TODO description");
    recompinput::set_game_input_description(recompinput::GameInput::START,      "TODO description");
    recompinput::set_game_input_description(recompinput::GameInput::C_UP,       "TODO description");
    recompinput::set_game_input_description(recompinput::GameInput::C_DOWN,     "TODO description");
    recompinput::set_game_input_description(recompinput::GameInput::C_LEFT,     "TODO description");
    recompinput::set_game_input_description(recompinput::GameInput::C_RIGHT,    "TODO description");
    recompinput::set_game_input_description(recompinput::GameInput::DPAD_UP,    "TODO description");
    recompinput::set_game_input_description(recompinput::GameInput::DPAD_DOWN,  "TODO description");
    recompinput::set_game_input_description(recompinput::GameInput::DPAD_LEFT,  "TODO description");
    recompinput::set_game_input_description(recompinput::GameInput::DPAD_RIGHT, "TODO description");
}

void zelda64::init_config() {
    std::filesystem::path recomp_dir = recompui::file::get_app_folder_path();

    if (!recomp_dir.empty()) {
        std::filesystem::create_directories(recomp_dir);
    }

    recompui::config::GeneralTabOptions general_options{};
    general_options.has_rumble_strength = true;
    general_options.has_gyro_sensitivity = true;
    general_options.has_mouse_sensitivity = true;

    auto &general_config = recompui::config::create_general_tab(general_options);
    add_general_options(general_config);

    set_control_defaults();
    set_control_descriptions();
    recompui::config::create_controls_tab();

    auto &graphics_config = recompui::config::create_graphics_tab();
    add_graphics_options(graphics_config);

    auto &sound_config = recompui::config::create_sound_tab();
    add_sound_options(sound_config);

    recompui::config::create_mods_tab();

    recompui::config::create_tab(
        zelda64::debug::tab_name,
        zelda64::debug::tab_id,
        [](recompui::ContextId context, recompui::Element* parent) {
            // TODO implement debug tab
        }
    );

    recompui::config::finalize();

    recompui::config::set_tab_visible(zelda64::debug::tab_id, get_debug_mode_enabled());
}
