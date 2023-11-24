#ifndef __RECOMP_INPUT_H__
#define __RECOMP_INPUT_H__

#include <cstdint>
#include <variant>
#include <vector>
#include <type_traits>

namespace recomp {
    struct ControllerState {
        enum Button : uint32_t {
            BUTTON_NORTH      = 1 << 0,
            BUTTON_SOUTH      = 1 << 1,
            BUTTON_EAST       = 1 << 2,
            BUTTON_WEST       = 1 << 3,
            BUTTON_L1         = 1 << 4, // Left Bumper
            BUTTON_R1         = 1 << 5, // Right Bumper
            BUTTON_L2         = 1 << 6, // Left Trigger Press
            BUTTON_R2         = 1 << 7, // Right Trigger Press
            BUTTON_L3         = 1 << 8, // Left Joystick Press
            BUTTON_R3         = 1 << 9, // Right Joystick Press
            BUTTON_DPAD_UP    = 1 << 10,
            BUTTON_DPAD_DOWN  = 1 << 11,
            BUTTON_DPAD_RIGHT = 1 << 12,
            BUTTON_DPAD_LEFT  = 1 << 13,
            BUTTON_START      = 1 << 14,
        };
        enum Axis : size_t {
            AXIS_LEFT_X,
            AXIS_LEFT_Y,
            AXIS_RIGHT_X,
            AXIS_RIGHT_Y,
            AXIS_LEFT_TRIGGER,
            AXIS_RIGHT_TRIGGER,
            AXIS_MAX
        };
        uint32_t buttons;
        float axes[AXIS_MAX];
    };

    struct MouseState {
        enum Button : uint32_t {
            LEFT = 1 << 0,
            RIGHT = 1 << 1,
            MIDDLE = 1 << 2,
            BACK = 1 << 3,
            FORWARD = 1 << 4,
        };
        int32_t wheel_pos;
        int32_t position_x;
        int32_t position_y;
        uint32_t buttons;
    };

    using InputState = std::variant<ControllerState, MouseState>;

    void get_keyboard_input(uint16_t* buttons_out, float* x_out, float* y_out);
    void get_n64_input(uint16_t* buttons_out, float* x_out, float* y_out);
    std::vector<InputState> get_input_states();
    void handle_events();
}

#endif
