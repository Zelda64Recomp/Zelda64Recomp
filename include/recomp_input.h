#ifndef __RECOMP_INPUT_H__
#define __RECOMP_INPUT_H__

#include <cstdint>
#include <variant>
#include <vector>
#include <type_traits>
#include <span>
#include <string>
#include <string_view>

#include "ultramodern/input.hpp"

#include "json/json.hpp"

namespace recomp {
    // x-macros to build input enums and arrays.
    // First parameter is the enum name, second parameter is the bit field for the input (or 0 if there is no associated one), third is the readable name.
    // TODO refactor this to allow projects to rename these, or get rid of the readable name and leave that up to individual projects to map.
    #define DEFINE_N64_BUTTON_INPUTS() \
        DEFINE_INPUT(A, 0x8000, "Action") \
        DEFINE_INPUT(B, 0x4000, "Attack/Cancel") \
        DEFINE_INPUT(Z, 0x2000, "Target") \
        DEFINE_INPUT(START, 0x1000, "Start") \
        DEFINE_INPUT(L, 0x0020, "Toggle map") \
        DEFINE_INPUT(R, 0x0010, "Shield") \
        DEFINE_INPUT(C_UP, 0x0008, "Look/Fairy") \
        DEFINE_INPUT(C_LEFT, 0x0002, "Item 1") \
        DEFINE_INPUT(C_DOWN, 0x0004, "Item 2") \
        DEFINE_INPUT(C_RIGHT, 0x0001, "Item 3") \
        DEFINE_INPUT(DPAD_UP, 0x0800, "Special Item 1") \
        DEFINE_INPUT(DPAD_RIGHT, 0x0100, "Special Item 2") \
        DEFINE_INPUT(DPAD_DOWN, 0x0400, "Special Item 3") \
        DEFINE_INPUT(DPAD_LEFT, 0x0200, "Special Item 4")

    #define DEFINE_N64_AXIS_INPUTS() \
        DEFINE_INPUT(Y_AXIS_POS, 0, "Up") \
        DEFINE_INPUT(Y_AXIS_NEG, 0, "Down") \
        DEFINE_INPUT(X_AXIS_NEG, 0, "Left") \
        DEFINE_INPUT(X_AXIS_POS, 0, "Right") \

    #define DEFINE_RECOMP_UI_INPUTS() \
        DEFINE_INPUT(TOGGLE_MENU, 0, "Toggle Menu") \
        DEFINE_INPUT(ACCEPT_MENU, 0, "Accept (Menu)") \
        DEFINE_INPUT(APPLY_MENU, 0, "Apply (Menu)")

    #define DEFINE_ALL_INPUTS() \
        DEFINE_N64_BUTTON_INPUTS() \
        DEFINE_N64_AXIS_INPUTS() \
        DEFINE_RECOMP_UI_INPUTS()

    // Enum containing every recomp input.
    #define DEFINE_INPUT(name, value, readable) name,
    enum class GameInput {
        DEFINE_ALL_INPUTS()

        COUNT,
        N64_BUTTON_START = A,
        N64_BUTTON_COUNT = C_RIGHT - N64_BUTTON_START + 1,
        N64_AXIS_START = X_AXIS_NEG,
        N64_AXIS_COUNT = Y_AXIS_POS - N64_AXIS_START + 1,
    };
    #undef DEFINE_INPUT

    struct InputField {
        uint32_t input_type;
        int32_t input_id;
        std::string to_string() const;
        auto operator<=>(const InputField& rhs) const = default;
    };

    void poll_inputs();
    float get_input_analog(const InputField& field);
    float get_input_analog(const std::span<const recomp::InputField> fields);
    bool get_input_digital(const InputField& field);
    bool get_input_digital(const std::span<const recomp::InputField> fields);
    void get_gyro_deltas(float* x, float* y);
    void get_mouse_deltas(float* x, float* y);
    void get_right_analog(float* x, float* y);

    enum class InputDevice {
        Controller,
        Keyboard,
        COUNT
    };

    void start_scanning_input(InputDevice device);
    void stop_scanning_input();
    void finish_scanning_input(InputField scanned_field);
    void cancel_scanning_input();
    void config_menu_set_cont_or_kb(bool cont_interacted);
    InputField get_scanned_input();
    int get_scanned_input_index();
    
    struct DefaultN64Mappings {
        std::vector<InputField> a;
        std::vector<InputField> b;
        std::vector<InputField> l;
        std::vector<InputField> r;
        std::vector<InputField> z;
        std::vector<InputField> start;

        std::vector<InputField> c_left;
        std::vector<InputField> c_right;
        std::vector<InputField> c_up;
        std::vector<InputField> c_down;

        std::vector<InputField> dpad_left;
        std::vector<InputField> dpad_right;
        std::vector<InputField> dpad_up;
        std::vector<InputField> dpad_down;

        std::vector<InputField> analog_left;
        std::vector<InputField> analog_right;
        std::vector<InputField> analog_up;
        std::vector<InputField> analog_down;

        std::vector<InputField> toggle_menu;
        std::vector<InputField> accept_menu;
        std::vector<InputField> apply_menu;
    };

    constexpr const std::vector<InputField>& get_default_mapping_for_input(const DefaultN64Mappings& defaults, const GameInput input) {
        switch (input) {
            case GameInput::A: return defaults.a;
            case GameInput::B: return defaults.b;
            case GameInput::L: return defaults.l;
            case GameInput::R: return defaults.r;
            case GameInput::Z: return defaults.z;
            case GameInput::START: return defaults.start;
            case GameInput::C_LEFT: return defaults.c_left;
            case GameInput::C_RIGHT: return defaults.c_right;
            case GameInput::C_UP: return defaults.c_up;
            case GameInput::C_DOWN: return defaults.c_down;
            case GameInput::DPAD_LEFT: return defaults.dpad_left;
            case GameInput::DPAD_RIGHT: return defaults.dpad_right;
            case GameInput::DPAD_UP: return defaults.dpad_up;
            case GameInput::DPAD_DOWN: return defaults.dpad_down;
            case GameInput::X_AXIS_NEG: return defaults.analog_left;
            case GameInput::X_AXIS_POS: return defaults.analog_right;
            case GameInput::Y_AXIS_POS: return defaults.analog_up;
            case GameInput::Y_AXIS_NEG: return defaults.analog_down;
            case GameInput::TOGGLE_MENU: return defaults.toggle_menu;
            case GameInput::ACCEPT_MENU: return defaults.accept_menu;
            case GameInput::APPLY_MENU: return defaults.apply_menu;
            default: return std::vector<InputField>();
        }
    }

    extern const DefaultN64Mappings default_n64_keyboard_mappings;
    extern const DefaultN64Mappings default_n64_controller_mappings;

    constexpr size_t bindings_per_input = 2;

    size_t get_num_inputs();
    const std::string& get_input_name(GameInput input);
    const std::string& get_input_enum_name(GameInput input);
    GameInput get_input_from_enum_name(const std::string_view name);
    InputField& get_input_binding(GameInput input, size_t binding_index, InputDevice device);
    void set_input_binding(GameInput input, size_t binding_index, InputDevice device, InputField value);

    bool get_n64_input(int controller_num, uint16_t* buttons_out, float* x_out, float* y_out);
    void set_rumble(int controller_num, bool);
    void update_rumble();
    void handle_events();

    ultramodern::input::connected_device_info_t get_connected_device_info(int controller_num);
    
    // Rumble strength ranges from 0 to 100.
    int get_rumble_strength();
    void set_rumble_strength(int strength);
    
    // Gyro and mouse sensitivities range from 0 to 100.
    int get_gyro_sensitivity();
    int get_mouse_sensitivity();
    int get_joystick_deadzone();
    void set_gyro_sensitivity(int strength);
    void set_mouse_sensitivity(int strength);
    void set_joystick_deadzone(int strength);
    void apply_joystick_deadzone(float x_in, float y_in, float* x_out, float* y_out);
    void set_right_analog_suppressed(bool suppressed);

    enum class BackgroundInputMode {
        On,
        Off,
		OptionCount
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(recomp::BackgroundInputMode, {
        {recomp::BackgroundInputMode::On, "On"},
        {recomp::BackgroundInputMode::Off, "Off"}
    });

    BackgroundInputMode get_background_input_mode();
    void set_background_input_mode(BackgroundInputMode mode);

    bool game_input_disabled();
    bool all_input_disabled();
}

#endif
