#include "../ultramodern/ultramodern.hpp"
#include "recomp_helpers.h"

static ultramodern::input_callbacks_t input_callbacks;

void set_input_callbacks(const ultramodern::input_callbacks_t& callbacks) {
    input_callbacks = callbacks;
}

static int max_controllers = 0;

extern "C" void osContInit_recomp(uint8_t* rdram, recomp_context* ctx) {
    PTR(void) bitpattern = _arg<1, PTR(void)>(rdram, ctx);
    PTR(void) status = _arg<2, PTR(void)>(rdram, ctx);

    // Set bit 0 to indicate that controller 0 is present
    MEM_B(0, bitpattern) = 0x01;

    // Mark controller 0 as present
    MEM_H(0, status) = 0x0005; // type: CONT_TYPE_NORMAL (from joybus)
    MEM_B(2, status) = 0x00; // status: 0 (from joybus)
    MEM_B(3, status) = 0x00; // errno: 0 (from libultra)

    max_controllers = 4;

    // Mark controllers 1-3 as not connected
    for (size_t controller = 1; controller < max_controllers; controller++) {
        // Libultra doesn't write status or type for absent controllers
        MEM_B(4 * controller + 3, status) = 0x80 >> 4; // errno: CONT_NO_RESPONSE_ERROR >> 4
    }

    _return<s32>(ctx, 0);
}

extern "C" void osContStartReadData_recomp(uint8_t* rdram, recomp_context* ctx) {
    ultramodern::send_si_message();
}

extern "C" void osContGetReadData_recomp(uint8_t* rdram, recomp_context* ctx) {
    PTR(void) pad = _arg<0, PTR(void)>(rdram, ctx);

    uint16_t buttons = 0;
    float x = 0.0f;
    float y = 0.0f;

    if (input_callbacks.get_input) {
        input_callbacks.get_input(&buttons, &x, &y);
    }

    if (max_controllers > 0) {
        // button
        MEM_H(0, pad) = buttons;
        // stick_x
        MEM_B(2, pad) = (int8_t)(127 * x);
        // stick_y
        MEM_B(3, pad) = (int8_t)(127 * y);
        // errno
        MEM_B(4, pad) = 0;
    }
    for (int controller = 1; controller < max_controllers; controller++) {
        MEM_B(6 * controller + 4, pad) = 0x80 >> 4; // errno: CONT_NO_RESPONSE_ERROR >> 4
    }
}

extern "C" void osContStartQuery_recomp(uint8_t * rdram, recomp_context * ctx) {
    ultramodern::send_si_message();
}

extern "C" void osContGetQuery_recomp(uint8_t * rdram, recomp_context * ctx) {
    PTR(void) status = _arg<0, PTR(void)>(rdram, ctx);

    // Mark controller 0 as present
    MEM_H(0, status) = 0x0005; // type: CONT_TYPE_NORMAL (from joybus)
    MEM_B(2, status) = 0x00; // status: 0 (from joybus)
    MEM_B(3, status) = 0x00; // errno: 0 (from libultra)

    // Mark controllers 1-3 as not connected
    for (size_t controller = 1; controller < max_controllers; controller++) {
        // Libultra doesn't write status or type for absent controllers
        MEM_B(4 * controller + 3, status) = 0x80 >> 4; // errno: CONT_NO_RESPONSE_ERROR >> 4
    }
}

extern "C" void osContSetCh_recomp(uint8_t* rdram, recomp_context* ctx) {
    max_controllers = std::min(_arg<0, u8>(rdram, ctx), u8(4));
    _return<s32>(ctx, 0);
}

extern "C" void __osMotorAccess_recomp(uint8_t* rdram, recomp_context* ctx) {

}

extern "C" void osMotorInit_recomp(uint8_t* rdram, recomp_context* ctx) {
    ;
}

extern "C" void osMotorStart_recomp(uint8_t* rdram, recomp_context* ctx) {
    ;
}

extern "C" void osMotorStop_recomp(uint8_t* rdram, recomp_context* ctx) {
    ;
}
