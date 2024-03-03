#include "recomp_config.h"
#include "recomp_input.h"
#include "../../ultramodern/config.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>

#if defined(_WIN32)
#include <Shlobj.h>
#elif defined(__linux__)
#include <unistd.h>
#include <pwd.h>
#endif

constexpr std::u8string_view graphics_filename = u8"graphics.json";
constexpr std::u8string_view controls_filename = u8"controls.json";

constexpr auto res_default            = ultramodern::Resolution::Auto;
constexpr auto wm_default             = ultramodern::WindowMode::Windowed;
constexpr auto ar_default             = RT64::UserConfiguration::AspectRatio::Expand;
constexpr auto msaa_default           = RT64::UserConfiguration::Antialiasing::MSAA4X;
constexpr auto rr_default             = RT64::UserConfiguration::RefreshRate::Display;
constexpr int rr_manual_default       = 60;
constexpr bool developer_mode_default = false;

namespace ultramodern {
    void to_json(json& j, const GraphicsConfig& config) {
        j = json{
            {"res_option",      config.res_option},
            {"wm_option",       config.wm_option},
            {"ar_option",       config.ar_option},
            {"msaa_option",     config.msaa_option},
            {"rr_option",       config.rr_option},
            {"rr_manual_value", config.rr_manual_value},
            {"developer_mode",  config.developer_mode},
        };
    }

    template <typename T>
    void from_or_default(const json& j, const std::string& key, T& out, T default_value) {
        auto find_it = j.find(key);
        if (find_it != j.end()) {
            find_it->get_to(out);
        }
        else {
            out = default_value;
        }
    }

    void from_json(const json& j, GraphicsConfig& config) {
        from_or_default(j, "res_option",      config.res_option, res_default);
        from_or_default(j, "wm_option",       config.wm_option, wm_default);
        from_or_default(j, "ar_option",       config.ar_option, ar_default);
        from_or_default(j, "msaa_option",     config.msaa_option, msaa_default);
        from_or_default(j, "rr_option",       config.rr_option, rr_default);
        from_or_default(j, "rr_manual_value", config.rr_manual_value, rr_manual_default);
        from_or_default(j, "developer_mode",  config.developer_mode, developer_mode_default);
    }
}

namespace recomp {
    void to_json(json& j, const InputField& field) {
        j = json{ {"input_type", field.input_type}, {"input_id", field.input_id} };
    }

    void from_json(const json& j, InputField& field) {
        j.at("input_type").get_to(field.input_type);
        j.at("input_id").get_to(field.input_id);
    }
}

std::filesystem::path get_config_folder_path() {
   std::filesystem::path recomp_dir{};

#if defined(_WIN32)
   // Deduce local app data path.
   PWSTR known_path = NULL;
   HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &known_path);
   if (result == S_OK) {
       recomp_dir = std::filesystem::path{known_path} / recomp::program_id;
   }

   CoTaskMemFree(known_path);
#elif defined(__linux__)
   const char *homedir;

   if ((homedir = getenv("HOME")) == nullptr) {
       homedir = getpwuid(getuid())->pw_dir;
   }

   if (homedir != nullptr) {
       recomp_dir = std::filesystem::path{homedir} / (std::string{"."} + recomp::program_id);
   }
#endif

    return recomp_dir;
}

void assign_mapping(recomp::InputDevice device, recomp::GameInput input, const std::vector<recomp::InputField>& value) {
    for (size_t binding_index = 0; binding_index < std::min(value.size(), recomp::bindings_per_input); binding_index++) {
        recomp::set_input_binding(input, binding_index, device, value[binding_index]);
    }
};

void assign_all_mappings(recomp::InputDevice device, const recomp::DefaultN64Mappings& values) {
    assign_mapping(device, recomp::GameInput::A, values.a);
    assign_mapping(device, recomp::GameInput::B, values.b);
    assign_mapping(device, recomp::GameInput::Z, values.z);
    assign_mapping(device, recomp::GameInput::START, values.start);
    assign_mapping(device, recomp::GameInput::DPAD_UP, values.dpad_up);
    assign_mapping(device, recomp::GameInput::DPAD_DOWN, values.dpad_down);
    assign_mapping(device, recomp::GameInput::DPAD_LEFT, values.dpad_left);
    assign_mapping(device, recomp::GameInput::DPAD_RIGHT, values.dpad_right);
    assign_mapping(device, recomp::GameInput::L, values.l);
    assign_mapping(device, recomp::GameInput::R, values.r);
    assign_mapping(device, recomp::GameInput::C_UP, values.c_up);
    assign_mapping(device, recomp::GameInput::C_DOWN, values.c_down);
    assign_mapping(device, recomp::GameInput::C_LEFT, values.c_left);
    assign_mapping(device, recomp::GameInput::C_RIGHT, values.c_right);

    assign_mapping(device, recomp::GameInput::X_AXIS_NEG, values.analog_left);
    assign_mapping(device, recomp::GameInput::X_AXIS_POS, values.analog_right);
    assign_mapping(device, recomp::GameInput::Y_AXIS_NEG, values.analog_down);
    assign_mapping(device, recomp::GameInput::Y_AXIS_POS, values.analog_up);
};

void recomp::reset_input_bindings() {
    assign_all_mappings(recomp::InputDevice::Keyboard, recomp::default_n64_keyboard_mappings);
    assign_all_mappings(recomp::InputDevice::Controller, recomp::default_n64_controller_mappings);
}

void recomp::reset_graphics_options() {
    ultramodern::GraphicsConfig new_config{};
    new_config.res_option = res_default;
    new_config.wm_option = wm_default;
    new_config.ar_option = ar_default;
    new_config.msaa_option = msaa_default;
    new_config.rr_option = rr_default;
    new_config.rr_manual_value = rr_manual_default;
    new_config.developer_mode = developer_mode_default;
    ultramodern::set_graphics_config(new_config);
}

void save_graphics_config(const std::filesystem::path& path) {
    std::ofstream config_file{path};
    
    nlohmann::json config_json{};
    ultramodern::to_json(config_json, ultramodern::get_graphics_config());
    config_file << std::setw(4) << config_json;
}

void load_graphics_config(const std::filesystem::path& path) {
    std::ifstream config_file{path};
    nlohmann::json config_json{};

    config_file >> config_json;

    ultramodern::GraphicsConfig new_config{};
    ultramodern::from_json(config_json, new_config);
    ultramodern::set_graphics_config(new_config);
}

void add_input_bindings(nlohmann::json& out, recomp::GameInput input, recomp::InputDevice device) {
    const std::string& input_name = recomp::get_input_enum_name(input);
    nlohmann::json& out_array = out[input_name];
    out_array = nlohmann::json::array();
    for (size_t binding_index = 0; binding_index < recomp::bindings_per_input; binding_index++) {
        out_array[binding_index] = recomp::get_input_binding(input, binding_index, device);
    }
};

void save_controls_config(const std::filesystem::path& path) {
    nlohmann::json config_json{};
    config_json["keyboard"] = {};
    config_json["controller"] = {};

    for (size_t i = 0; i < recomp::get_num_inputs(); i++) {
        recomp::GameInput cur_input = static_cast<recomp::GameInput>(i);

        add_input_bindings(config_json["keyboard"], cur_input, recomp::InputDevice::Keyboard);
        add_input_bindings(config_json["controller"], cur_input, recomp::InputDevice::Controller);
    }

    std::ofstream config_file{path};
    config_file << std::setw(4) << config_json;
}

bool load_input_device_from_json(const nlohmann::json& config_json, recomp::InputDevice device, const std::string& key) {
    // Check if the json object for the given key exists.
    auto find_it = config_json.find(key);
    if (find_it == config_json.end()) {
        return false;
    }

    const nlohmann::json& mappings_json = *find_it;

    for (size_t i = 0; i < recomp::get_num_inputs(); i++) {
        recomp::GameInput cur_input = static_cast<recomp::GameInput>(i);
        const std::string& input_name = recomp::get_input_enum_name(cur_input);

        // Check if the json object for the given input exists and that it's an array.
        auto find_input_it = mappings_json.find(input_name);
        if (find_input_it == mappings_json.end() || !find_input_it->is_array()) {
            continue;
        }
        const nlohmann::json& input_json = *find_input_it;

        // Deserialize all the bindings from the json array (up to the max number of bindings per input).
        for (size_t binding_index = 0; binding_index < std::min(recomp::bindings_per_input, input_json.size()); binding_index++) {
            recomp::InputField cur_field{};
            recomp::from_json(input_json[binding_index], cur_field);
            recomp::set_input_binding(cur_input, binding_index, device, cur_field);
        }
    }

    return true;
}

void load_controls_config(const std::filesystem::path& path) {
    std::ifstream config_file{path};
    nlohmann::json config_json{};

    config_file >> config_json;

    if (!load_input_device_from_json(config_json, recomp::InputDevice::Keyboard, "keyboard")) {
        assign_all_mappings(recomp::InputDevice::Keyboard, recomp::default_n64_keyboard_mappings);
    }

    if (!load_input_device_from_json(config_json, recomp::InputDevice::Controller, "controller")) {
        assign_all_mappings(recomp::InputDevice::Controller, recomp::default_n64_controller_mappings);
    }
}

void recomp::load_config() {
    std::filesystem::path recomp_dir = get_config_folder_path();
    std::filesystem::path graphics_path = recomp_dir / graphics_filename;
    std::filesystem::path controls_path = recomp_dir / controls_filename;

    if (std::filesystem::exists(graphics_path)) {
        load_graphics_config(graphics_path);
    }
    else {
        recomp::reset_graphics_options();
        save_graphics_config(graphics_path);
    }

    if (std::filesystem::exists(controls_path)) {
        load_controls_config(controls_path);
    }
    else {
        recomp::reset_input_bindings();
        save_controls_config(controls_path);
    }
}

void recomp::save_config() {
    std::filesystem::path recomp_dir = get_config_folder_path();

    if (recomp_dir.empty()) {
        return;
    }

    std::filesystem::create_directories(recomp_dir);

    save_graphics_config(recomp_dir / graphics_filename);
    save_controls_config(recomp_dir / controls_filename);
}
