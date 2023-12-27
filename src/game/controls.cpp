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

void recomp::get_n64_input(uint16_t* buttons_out, float* x_out, float* y_out) {

    uint16_t cur_buttons = 0;
    float cur_x = 0.0f;
    float cur_y = 0.0f;

    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.a) ? N64Inputs::A : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.b) ? N64Inputs::B : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.l) ? N64Inputs::L : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.r) ? N64Inputs::R : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.z) ? N64Inputs::Z : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.start) ? N64Inputs::START : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.c_left)  ? N64Inputs::C_LEFT : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.c_right) ? N64Inputs::C_RIGHT : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.c_up)    ? N64Inputs::C_UP : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.c_down)  ? N64Inputs::C_DOWN : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.dpad_left)  ? N64Inputs::DPAD_LEFT : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.dpad_right) ? N64Inputs::DPAD_RIGHT : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.dpad_up)    ? N64Inputs::DPAD_UP : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.dpad_down)  ? N64Inputs::DPAD_DOWN : 0;

    *buttons_out = cur_buttons;
    cur_x = recomp::get_input_analog(recomp::default_n64_mappings.analog_right) - recomp::get_input_analog(recomp::default_n64_mappings.analog_left);
    cur_y = recomp::get_input_analog(recomp::default_n64_mappings.analog_up) - recomp::get_input_analog(recomp::default_n64_mappings.analog_down);
    *x_out = cur_x;
    *y_out = cur_y;
}

extern "C" void recomp_get_item_inputs(uint8_t* rdram, recomp_context* ctx) {
    u32* buttons_out = _arg<0, u32*>(rdram, ctx);

    uint32_t cur_buttons = 0;

    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.b) ? N64Inputs::B : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.c_left) ? N64Inputs::C_LEFT : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.c_right) ? N64Inputs::C_RIGHT : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.c_down) ? N64Inputs::C_DOWN : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.dpad_left) ? N64Inputs::DPAD_LEFT : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.dpad_right) ? N64Inputs::DPAD_RIGHT : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.dpad_up) ? N64Inputs::DPAD_UP : 0;
    cur_buttons |= recomp::get_input_digital(recomp::default_n64_mappings.dpad_down) ? N64Inputs::DPAD_DOWN : 0;

    *buttons_out = cur_buttons;
}

bool recomp_digital_input_state[RECOMP_DIGITAL_INPUT_MAX];
float recomp_analog_input_state[RECOMP_ANALOG_INPUT_MAX];

extern "C" void recomp_update_inputs(uint8_t* rdram, recomp_context* ctx) {
    recomp::poll_inputs();
}

extern "C" void recomp_get_digital_input(uint8_t* rdram, recomp_context* ctx) {
    u32 input_slot = _arg<0, u32>(rdram, ctx);

    // TODO implement this

    _return<u32>(ctx, 0);
}

extern "C" void recomp_get_analog_input(uint8_t* rdram, recomp_context* ctx) {
    u32 input_slot = _arg<0, u32>(rdram, ctx);

    // TODO implement this

    _return<float>(ctx, 0);
}

extern "C" void recomp_get_camera_inputs(uint8_t* rdram, recomp_context* ctx) {
    float* x_out = _arg<0, float*>(rdram, ctx);
    float* y_out = _arg<1, float*>(rdram, ctx);

    float x_val = recomp::get_input_analog(recomp::default_n64_mappings.c_right) - recomp::get_input_analog(recomp::default_n64_mappings.c_left);
    float y_val = recomp::get_input_analog(recomp::default_n64_mappings.c_up) - recomp::get_input_analog(recomp::default_n64_mappings.c_down);

    *x_out = x_val;
    *y_out = y_val;
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
