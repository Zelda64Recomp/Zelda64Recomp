#include <array>

#include "recomp_helpers.h"
#include "recomp_input.h"
#include "../ultramodern/ultramodern.hpp"
#include "../patches/input.h"

// x-macros to build input enums and arrays. First parameter is the name, second parameter is the bit field for the input (or 0 if there is no associated one)
#define DEFINE_N64_BUTTON_INPUTS() \
    DEFINE_INPUT(A, 0x8000) \
    DEFINE_INPUT(B, 0x4000) \
    DEFINE_INPUT(Z, 0x2000) \
    DEFINE_INPUT(START, 0x1000) \
    DEFINE_INPUT(DPAD_UP, 0x0800) \
    DEFINE_INPUT(DPAD_DOWN, 0x0400) \
    DEFINE_INPUT(DPAD_LEFT, 0x0200) \
    DEFINE_INPUT(DPAD_RIGHT, 0x0100) \
    DEFINE_INPUT(L, 0x0020) \
    DEFINE_INPUT(R, 0x0010) \
    DEFINE_INPUT(C_UP, 0x0008) \
    DEFINE_INPUT(C_DOWN, 0x0004) \
    DEFINE_INPUT(C_LEFT, 0x0002) \
    DEFINE_INPUT(C_RIGHT, 0x0001)

#define DEFINE_N64_AXIS_INPUTS() \
    DEFINE_INPUT(X_AXIS_NEG, 0) \
    DEFINE_INPUT(X_AXIS_POS, 0) \
    DEFINE_INPUT(Y_AXIS_NEG, 0) \
    DEFINE_INPUT(Y_AXIS_POS, 0) \

// Make the input enum.
#define DEFINE_INPUT(name, value) name,
enum class GameInput {
    DEFINE_N64_BUTTON_INPUTS()
    DEFINE_N64_AXIS_INPUTS()

    COUNT,
    N64_BUTTON_START = A,
    N64_BUTTON_COUNT = C_RIGHT - N64_BUTTON_START + 1,
    N64_AXIS_START = X_AXIS_NEG,
    N64_AXIS_COUNT = Y_AXIS_POS - N64_AXIS_START + 1,
};
#undef DEFINE_INPUT

// Arrays that hold the mappings for every input for keyboard and controller respectively.
using input_mapping_array = std::array<std::vector<recomp::InputField>, (size_t)GameInput::COUNT>;
static std::array<std::vector<recomp::InputField>, (size_t)GameInput::COUNT> keyboard_input_mappings{};
static std::array<std::vector<recomp::InputField>, (size_t)GameInput::COUNT> controller_input_mappings{};

// Make the button value array, which maps a button index to its bit field.
#define DEFINE_INPUT(name, value) uint16_t(value##u),
static const std::array n64_button_values = {
    DEFINE_N64_BUTTON_INPUTS()
};
#undef DEFINE_INPUT

void recomp::init_control_mappings() {
    // TODO load from a file if one exists.

    auto assign_mapping = [](input_mapping_array& mapping, GameInput input, const std::vector<recomp::InputField>& value) {
        mapping[(size_t)input] = value;
    };

    auto assign_all_mappings = [&](input_mapping_array& mapping, const recomp::DefaultN64Mappings& values) {
        assign_mapping(mapping, GameInput::A, values.a);
        assign_mapping(mapping, GameInput::A, values.a);
        assign_mapping(mapping, GameInput::B, values.b);
        assign_mapping(mapping, GameInput::Z, values.z);
        assign_mapping(mapping, GameInput::START, values.start);
        assign_mapping(mapping, GameInput::DPAD_UP, values.dpad_up);
        assign_mapping(mapping, GameInput::DPAD_DOWN, values.dpad_down);
        assign_mapping(mapping, GameInput::DPAD_LEFT, values.dpad_left);
        assign_mapping(mapping, GameInput::DPAD_RIGHT, values.dpad_right);
        assign_mapping(mapping, GameInput::L, values.l);
        assign_mapping(mapping, GameInput::R, values.r);
        assign_mapping(mapping, GameInput::C_UP, values.c_up);
        assign_mapping(mapping, GameInput::C_DOWN, values.c_down);
        assign_mapping(mapping, GameInput::C_LEFT, values.c_left);
        assign_mapping(mapping, GameInput::C_RIGHT, values.c_right);

        assign_mapping(mapping, GameInput::X_AXIS_NEG, values.analog_left);
        assign_mapping(mapping, GameInput::X_AXIS_POS, values.analog_right);
        assign_mapping(mapping, GameInput::Y_AXIS_NEG, values.analog_down);
        assign_mapping(mapping, GameInput::Y_AXIS_POS, values.analog_up);
    };

    assign_all_mappings(keyboard_input_mappings, recomp::default_n64_keyboard_mappings);
    assign_all_mappings(controller_input_mappings, recomp::default_n64_controller_mappings);
}

void recomp::get_n64_input(uint16_t* buttons_out, float* x_out, float* y_out) {
    uint16_t cur_buttons = 0;
    float cur_x = 0.0f;
    float cur_y = 0.0f;

    if (!recomp::game_input_disabled()) {
        for (size_t i = 0; i < n64_button_values.size(); i++) {
            size_t input_index = (size_t)GameInput::N64_BUTTON_START + i;
            cur_buttons |= recomp::get_input_digital(keyboard_input_mappings[input_index]) ? n64_button_values[i] : 0;
            cur_buttons |= recomp::get_input_digital(controller_input_mappings[input_index]) ? n64_button_values[i] : 0;
        }

        cur_x = recomp::get_input_analog(keyboard_input_mappings[(size_t)GameInput::X_AXIS_POS])
                - recomp::get_input_analog(keyboard_input_mappings[(size_t)GameInput::X_AXIS_NEG])
                + recomp::get_input_analog(controller_input_mappings[(size_t)GameInput::X_AXIS_POS])
                - recomp::get_input_analog(controller_input_mappings[(size_t)GameInput::X_AXIS_NEG]);
        cur_y = recomp::get_input_analog(keyboard_input_mappings[(size_t)GameInput::Y_AXIS_POS])
                - recomp::get_input_analog(keyboard_input_mappings[(size_t)GameInput::Y_AXIS_NEG])
                + recomp::get_input_analog(controller_input_mappings[(size_t)GameInput::Y_AXIS_POS])
                - recomp::get_input_analog(controller_input_mappings[(size_t)GameInput::Y_AXIS_NEG]);
    }

    *buttons_out = cur_buttons;
    *x_out = std::clamp(cur_x, -1.0f, 1.0f);
    *y_out = std::clamp(cur_y, -1.0f, 1.0f);
}

extern "C" void recomp_update_inputs(uint8_t* rdram, recomp_context* ctx) {
    recomp::poll_inputs();
}

// TODO move these
extern "C" void recomp_puts(uint8_t* rdram, recomp_context* ctx) {
    PTR(char) cur_str = _arg<0, PTR(char)>(rdram, ctx);
    u32 length = _arg<1, u32>(rdram, ctx);

    for (u32 i = 0; i < length; i++) {
        fputc(MEM_B(i, (gpr)cur_str), stdout);
    }
}

extern "C" void recomp_exit(uint8_t* rdram, recomp_context* ctx) {
    ultramodern::quit();
}
