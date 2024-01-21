#include "recomp_config.h"
#include "recomp_input.h"
#include "../../ultramodern/config.hpp"

void recomp::reset_input_bindings() {
    auto assign_mapping = [](recomp::InputDevice device, recomp::GameInput input, const std::vector<recomp::InputField>& value) {
        for (size_t binding_index = 0; binding_index < std::min(value.size(), recomp::bindings_per_input); binding_index++) {
            recomp::set_input_binding(input, binding_index, device, value[binding_index]);
        }
    };

    auto assign_all_mappings = [&](recomp::InputDevice device, const recomp::DefaultN64Mappings& values) {
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

    assign_all_mappings(recomp::InputDevice::Keyboard, recomp::default_n64_keyboard_mappings);
    assign_all_mappings(recomp::InputDevice::Controller, recomp::default_n64_controller_mappings);
}

void recomp::reset_graphics_options() {
    ultramodern::GraphicsConfig new_config{};
    new_config.res_option = ultramodern::Resolution::Auto;
    new_config.wm_option = ultramodern::WindowMode::Fullscreen;
    new_config.ar_option = RT64::UserConfiguration::AspectRatio::Expand;
    new_config.msaa_option = RT64::UserConfiguration::Antialiasing::MSAA4X;
    new_config.rr_option = RT64::UserConfiguration::RefreshRate::Original;
    new_config.rr_manual_value = 60;
    ultramodern::set_graphics_config(new_config);
}

void recomp::load_config() {
    // TODO load from a file if one exists.
    recomp::reset_input_bindings();

    // TODO load from a file if one exists.
    recomp::reset_graphics_options();
}

void recomp::save_config() {
    
}
