#ifndef __RECOMP_INPUT_H__
#define __RECOMP_INPUT_H__

#include <cstdint>
#include <variant>
#include <vector>
#include <type_traits>
#include <span>
#include <string>

namespace recomp {
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

    enum class InputDevice {
        Controller,
        Keyboard,
        COUNT
    };

    void start_scanning_input(InputDevice device);
    void finish_scanning_input(InputField scanned_field);
    InputField get_scanned_input();
    
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
    };

    extern const DefaultN64Mappings default_n64_keyboard_mappings;
    extern const DefaultN64Mappings default_n64_controller_mappings;

    constexpr size_t bindings_per_input = 2;

    // Loads the user's saved controller mapping if one exists, loads the default mappings if no saved mapping exists.
    void init_control_mappings();
    size_t get_num_inputs();
    const std::string& get_input_name(size_t input_index);
    const std::string& get_input_enum_name(size_t input_index);
    InputField& get_input_binding(size_t input_index, size_t binding_index, InputDevice device);
    void set_input_binding(size_t input_index, size_t binding_index, InputDevice device, InputField value);

    void get_n64_input(uint16_t* buttons_out, float* x_out, float* y_out);
    void handle_events();

    bool game_input_disabled();
    bool all_input_disabled();

    // TODO move these
    void quicksave_save();
    void quicksave_load();
}

#endif
