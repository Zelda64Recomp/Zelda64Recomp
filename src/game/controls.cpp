#include "recomp_helpers.h"
#include "recomp_input.h"
#include "../ultramodern/ultramodern.hpp"
#include "../patches/input.h"

namespace N64Inputs {
    enum Input : uint16_t {
        A          = 0x8000,
        B          = 0x4000,
        Z          = 0x2000,
        START      = 0x1000,
        DPAD_UP    = 0x0800,
        DPAD_DOWN  = 0x0400,
        DPAD_LEFT  = 0x0200,
        DPAD_RIGHT = 0x0100,
        L          = 0x0020,
        R          = 0x0010,
        C_UP       = 0x0008,
        C_DOWN     = 0x0004,
        C_LEFT     = 0x0002,
        C_RIGHT    = 0x0001,
    };
}

constexpr float controller_default_threshold = 0.7f;
struct GameControllerAxisMapping {
    int32_t axis;
    float threshold; // Positive or negative to indicate direction
    uint32_t output_mask;
};
using axis_map_t = std::vector<GameControllerAxisMapping>;

struct GameControllerButtonMapping {
    uint32_t button;
    uint32_t output_mask;
};
using button_map_t = std::vector<GameControllerButtonMapping>;

uint32_t process_controller_mappings(const recomp::ControllerState& controller_state, const axis_map_t& axis_map, const button_map_t& button_map) {
    uint32_t cur_buttons = 0;

    for (const auto& mapping : axis_map) {
        float input_value = controller_state.axes[mapping.axis];
        if (mapping.threshold > 0) {
            if (input_value > mapping.threshold) {
                cur_buttons |= mapping.output_mask;
            }
        }
        else {
            if (input_value < mapping.threshold) {
                cur_buttons |= mapping.output_mask;
            }
        }
    }

    for (const auto& mapping : button_map) {
        int input_value = controller_state.buttons & mapping.button;
        if (input_value) {
            cur_buttons |= mapping.output_mask;
        }
    }

    return cur_buttons;
}

void recomp::get_n64_input(uint16_t* buttons_out, float* x_out, float* y_out) {
    static const axis_map_t general_axis_map{
        { recomp::ControllerState::AXIS_RIGHT_X,      -controller_default_threshold, N64Inputs::C_LEFT },
        { recomp::ControllerState::AXIS_RIGHT_X,       controller_default_threshold, N64Inputs::C_RIGHT },
        { recomp::ControllerState::AXIS_RIGHT_Y,      -controller_default_threshold, N64Inputs::C_UP },
        { recomp::ControllerState::AXIS_RIGHT_Y,       controller_default_threshold, N64Inputs::C_DOWN },
        { recomp::ControllerState::AXIS_LEFT_TRIGGER, 0.30f,                         N64Inputs::Z },
    };
    static const button_map_t general_button_map{
        { recomp::ControllerState::BUTTON_START,      N64Inputs::START },
        { recomp::ControllerState::BUTTON_SOUTH,      N64Inputs::A },
        { recomp::ControllerState::BUTTON_EAST,       N64Inputs::B },
        { recomp::ControllerState::BUTTON_WEST,       N64Inputs::B },
        { recomp::ControllerState::BUTTON_L1,         N64Inputs::L },
        { recomp::ControllerState::BUTTON_R1,         N64Inputs::R },
        { recomp::ControllerState::BUTTON_DPAD_LEFT,  N64Inputs::DPAD_LEFT },
        { recomp::ControllerState::BUTTON_DPAD_RIGHT, N64Inputs::DPAD_RIGHT },
        { recomp::ControllerState::BUTTON_DPAD_UP,    N64Inputs::DPAD_UP },
        { recomp::ControllerState::BUTTON_DPAD_DOWN,  N64Inputs::DPAD_DOWN },
    };

    uint16_t cur_buttons = 0;
    float cur_x = 0.0f;
    float cur_y = 0.0f;

    recomp::get_keyboard_input(&cur_buttons, &cur_x, &cur_y);

    std::vector<InputState> input_states = recomp::get_input_states();

    for (const InputState& state : input_states) {
        if (const auto* controller_state = std::get_if<ControllerState>(&state)) {
            cur_x += controller_state->axes[ControllerState::AXIS_LEFT_X];
            cur_y -= controller_state->axes[ControllerState::AXIS_LEFT_Y];

            cur_buttons |= (uint16_t)process_controller_mappings(*controller_state, general_axis_map, general_button_map);
        }
        else if (const auto* mouse_state = std::get_if<MouseState>(&state)) {
            // Mouse currently unused
        }
    }

    *buttons_out = cur_buttons;
    cur_x = std::clamp(cur_x, -1.0f, 1.0f);
    cur_y = std::clamp(cur_y, -1.0f, 1.0f);
    *x_out = cur_x;
    *y_out = cur_y;
}

extern "C" void recomp_get_item_inputs(uint8_t* rdram, recomp_context* ctx) {
    static const axis_map_t item_axis_map{
        { recomp::ControllerState::AXIS_RIGHT_X, -controller_default_threshold, N64Inputs::C_LEFT },
        { recomp::ControllerState::AXIS_RIGHT_X,  controller_default_threshold, N64Inputs::C_RIGHT },
        { recomp::ControllerState::AXIS_RIGHT_Y,  controller_default_threshold, N64Inputs::C_DOWN },
    };

    static const button_map_t item_button_map {
        { recomp::ControllerState::BUTTON_EAST, N64Inputs::B },
        { recomp::ControllerState::BUTTON_WEST, N64Inputs::B },
        { recomp::ControllerState::BUTTON_DPAD_LEFT,  N64Inputs::DPAD_LEFT },
        { recomp::ControllerState::BUTTON_DPAD_RIGHT, N64Inputs::DPAD_RIGHT },
        { recomp::ControllerState::BUTTON_DPAD_UP,    N64Inputs::DPAD_UP },
        { recomp::ControllerState::BUTTON_DPAD_DOWN,  N64Inputs::DPAD_DOWN },
    };

    u32* buttons_out = _arg<0, u32*>(rdram, ctx);

    uint32_t cur_buttons = 0;

    // TODO do this in a way that will allow for remapping keyboard inputs
    uint16_t keyboard_buttons;
    float dummy_x, dummy_y;
    recomp::get_keyboard_input(&keyboard_buttons, &dummy_x, &dummy_y);
    cur_buttons |= keyboard_buttons;

    // Process controller inputs
    std::vector<recomp::InputState> input_states = recomp::get_input_states();

    for (const recomp::InputState& state : input_states) {
        if (const auto* controller_state = std::get_if<recomp::ControllerState>(&state)) {
            cur_buttons |= process_controller_mappings(*controller_state, item_axis_map, item_button_map);
        }
        else if (const auto* mouse_state = std::get_if<recomp::MouseState>(&state)) {
            // Mouse currently unused
        }
    }

    *buttons_out = cur_buttons;
}

extern "C" void recomp_get_camera_inputs(uint8_t* rdram, recomp_context* ctx) {
    float* x_out = _arg<0, float*>(rdram, ctx);
    float* y_out = _arg<1, float*>(rdram, ctx);

    float x_val = 0.0f;
    float y_val = 0.0f;

    // Process controller inputs
    std::vector<recomp::InputState> input_states = recomp::get_input_states();

    for (const recomp::InputState& state : input_states) {
        if (const auto* controller_state = std::get_if<recomp::ControllerState>(&state)) {
            x_val += controller_state->axes[recomp::ControllerState::AXIS_RIGHT_X];
            y_val += controller_state->axes[recomp::ControllerState::AXIS_RIGHT_Y];
        }
        else if (const auto* mouse_state = std::get_if<recomp::MouseState>(&state)) {
            // Mouse currently unused
        }
    }

    *x_out = x_val;
    *y_out = y_val;
}

// TODO move this
extern "C" void recomp_puts(uint8_t* rdram, recomp_context* ctx) {
    PTR(char) cur_str = _arg<0, PTR(char)>(rdram, ctx);
    u32 length = _arg<1, u32>(rdram, ctx);

    for (u32 i = 0; i < length; i++) {
        fputc(MEM_B(i, (gpr)cur_str), stdout);
    }
}
