#include <atomic>

#include "../ultramodern/ultramodern.hpp"
#include "recomp.h"
#include "recomp_input.h"
#include "recomp_ui.h"
#include "SDL.h"
#include "rt64_layer.h"


static struct {
    const Uint8* keys = nullptr;
    int numkeys = 0;
    std::atomic_int32_t mouse_wheel_pos = 0;
    std::vector<SDL_JoystickID> controller_ids{};
    std::vector<SDL_GameController*> cur_controllers{};
} InputState;

bool sdl_event_filter(void* userdata, SDL_Event* event) {
    switch (event->type) {
    case SDL_EventType::SDL_KEYDOWN:
        {
            SDL_KeyboardEvent* keyevent = (SDL_KeyboardEvent*)event;

            if (keyevent->keysym.scancode == SDL_Scancode::SDL_SCANCODE_RETURN && (keyevent->keysym.mod & SDL_Keymod::KMOD_ALT)) {
                RT64ChangeWindow();
            }
        }
        recomp::queue_event(*event);
        break;
    case SDL_EventType::SDL_CONTROLLERDEVICEADDED:
        {
            SDL_ControllerDeviceEvent* controller_event = (SDL_ControllerDeviceEvent*)event;
            SDL_GameController* controller = SDL_GameControllerOpen(controller_event->which);
            printf("Controller added: %d\n", controller_event->which);
            if (controller != nullptr) {
                printf("  Instance ID: %d\n", SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller)));
                InputState.controller_ids.push_back(SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller)));
            }
        }
        break;
    case SDL_EventType::SDL_CONTROLLERDEVICEREMOVED:
        {
            SDL_ControllerDeviceEvent* controller_event = (SDL_ControllerDeviceEvent*)event;
            printf("Controller removed: %d\n", controller_event->which);
            std::erase(InputState.controller_ids, controller_event->which);
        }
        break;
    case SDL_EventType::SDL_QUIT:
        ultramodern::quit();
        return true;
    case SDL_EventType::SDL_MOUSEWHEEL:
        {
            SDL_MouseWheelEvent* wheel_event = (SDL_MouseWheelEvent*)event;    
            InputState.mouse_wheel_pos.fetch_add(wheel_event->y * (wheel_event->direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1));
        }
        recomp::queue_event(*event);
        break;
    default:
        recomp::queue_event(*event);
        break;
    }
    return false;
}

void recomp::handle_events() {
    SDL_Event cur_event;
    static bool exited = false;
    while (SDL_PollEvent(&cur_event) && !exited) {
        exited = sdl_event_filter(nullptr, &cur_event);
    }
}

enum class DeviceType {
    Keyboard,
    Mouse,
    ControllerDigital,
    ControllerAnalog // Axis input_id values are the SDL value + 1
};

constexpr SDL_GameControllerButton SDL_CONTROLLER_BUTTON_SOUTH = SDL_CONTROLLER_BUTTON_A;
constexpr SDL_GameControllerButton SDL_CONTROLLER_BUTTON_EAST = SDL_CONTROLLER_BUTTON_B;
constexpr SDL_GameControllerButton SDL_CONTROLLER_BUTTON_WEST = SDL_CONTROLLER_BUTTON_X;
constexpr SDL_GameControllerButton SDL_CONTROLLER_BUTTON_NORTH = SDL_CONTROLLER_BUTTON_Y;

const recomp::DefaultN64Mappings recomp::default_n64_keyboard_mappings = {
    .a = {
        {.device_type = (uint32_t)DeviceType::Keyboard, .input_id = SDL_SCANCODE_SPACE}
    },
    .b = {
        {.device_type = (uint32_t)DeviceType::Keyboard, .input_id = SDL_SCANCODE_LSHIFT}
    },
    .l = {
        {.device_type = (uint32_t)DeviceType::Keyboard, .input_id = SDL_SCANCODE_E}
    },
    .r = {
        {.device_type = (uint32_t)DeviceType::Keyboard, .input_id = SDL_SCANCODE_R}
    },
    .z = {
        {.device_type = (uint32_t)DeviceType::Keyboard, .input_id = SDL_SCANCODE_Q}
    },
    .start = {
        {.device_type = (uint32_t)DeviceType::Keyboard, .input_id = SDL_SCANCODE_RETURN}
    },
    .c_left = {
        {.device_type = (uint32_t)DeviceType::Keyboard, .input_id = SDL_SCANCODE_LEFT}
    },
    .c_right = {
        {.device_type = (uint32_t)DeviceType::Keyboard, .input_id = SDL_SCANCODE_RIGHT}
    },
    .c_up = {
        {.device_type = (uint32_t)DeviceType::Keyboard, .input_id = SDL_SCANCODE_UP}
    },
    .c_down = {
        {.device_type = (uint32_t)DeviceType::Keyboard, .input_id = SDL_SCANCODE_DOWN}
    },
    .dpad_left = {
        {.device_type = (uint32_t)DeviceType::Keyboard, .input_id = SDL_SCANCODE_J}
    },
    .dpad_right = {
        {.device_type = (uint32_t)DeviceType::Keyboard, .input_id = SDL_SCANCODE_L}
    },
    .dpad_up = {
        {.device_type = (uint32_t)DeviceType::Keyboard, .input_id = SDL_SCANCODE_I}
    },
    .dpad_down = {
        {.device_type = (uint32_t)DeviceType::Keyboard, .input_id = SDL_SCANCODE_K}
    },
    .analog_left = {
        {.device_type = (uint32_t)DeviceType::Keyboard, .input_id = SDL_SCANCODE_A}
    },
    .analog_right = {
        {.device_type = (uint32_t)DeviceType::Keyboard, .input_id = SDL_SCANCODE_D}
    },
    .analog_up = {
        {.device_type = (uint32_t)DeviceType::Keyboard, .input_id = SDL_SCANCODE_W}
    },
    .analog_down = {
        {.device_type = (uint32_t)DeviceType::Keyboard, .input_id = SDL_SCANCODE_S}
    },
};

const recomp::DefaultN64Mappings recomp::default_n64_controller_mappings = {
    .a = {
        {.device_type = (uint32_t)DeviceType::ControllerDigital, .input_id = SDL_CONTROLLER_BUTTON_SOUTH},
    },
    .b = {
        {.device_type = (uint32_t)DeviceType::ControllerDigital, .input_id = SDL_CONTROLLER_BUTTON_EAST},
        {.device_type = (uint32_t)DeviceType::ControllerDigital, .input_id = SDL_CONTROLLER_BUTTON_WEST},
    },
    .l = {
        {.device_type = (uint32_t)DeviceType::ControllerDigital, .input_id = SDL_CONTROLLER_BUTTON_LEFTSHOULDER},
    },
    .r = {
        {.device_type = (uint32_t)DeviceType::ControllerDigital, .input_id = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER},
        {.device_type = (uint32_t)DeviceType::ControllerAnalog, .input_id = SDL_CONTROLLER_AXIS_TRIGGERRIGHT + 1},
    },
    .z = {
        {.device_type = (uint32_t)DeviceType::ControllerAnalog, .input_id = SDL_CONTROLLER_AXIS_TRIGGERLEFT + 1},
    },
    .start = {
        {.device_type = (uint32_t)DeviceType::ControllerDigital, .input_id = SDL_CONTROLLER_BUTTON_START},
    },
    .c_left = {
        {.device_type = (uint32_t)DeviceType::ControllerAnalog, .input_id = -(SDL_CONTROLLER_AXIS_RIGHTX + 1)},
    },
    .c_right = {
        {.device_type = (uint32_t)DeviceType::ControllerAnalog, .input_id = SDL_CONTROLLER_AXIS_RIGHTX + 1},
    },
    .c_up = {
        {.device_type = (uint32_t)DeviceType::ControllerAnalog, .input_id = -(SDL_CONTROLLER_AXIS_RIGHTY + 1)},
    },
    .c_down = {
        {.device_type = (uint32_t)DeviceType::ControllerAnalog, .input_id = SDL_CONTROLLER_AXIS_RIGHTY + 1},
    },
    .dpad_left = {
        {.device_type = (uint32_t)DeviceType::ControllerDigital, .input_id = SDL_CONTROLLER_BUTTON_DPAD_LEFT},
    },
    .dpad_right = {
        {.device_type = (uint32_t)DeviceType::ControllerDigital, .input_id = SDL_CONTROLLER_BUTTON_DPAD_RIGHT},
    },
    .dpad_up = {
        {.device_type = (uint32_t)DeviceType::ControllerDigital, .input_id = SDL_CONTROLLER_BUTTON_DPAD_UP},
    },
    .dpad_down = {
        {.device_type = (uint32_t)DeviceType::ControllerDigital, .input_id = SDL_CONTROLLER_BUTTON_DPAD_DOWN},
    },
    .analog_left = {
        {.device_type = (uint32_t)DeviceType::ControllerAnalog, .input_id = -(SDL_CONTROLLER_AXIS_LEFTX + 1)},
    },
    .analog_right = {
        {.device_type = (uint32_t)DeviceType::ControllerAnalog, .input_id = SDL_CONTROLLER_AXIS_LEFTX + 1},
    },
    .analog_up = {
        {.device_type = (uint32_t)DeviceType::ControllerAnalog, .input_id = -(SDL_CONTROLLER_AXIS_LEFTY + 1)},
    },
    .analog_down = {
        {.device_type = (uint32_t)DeviceType::ControllerAnalog, .input_id = SDL_CONTROLLER_AXIS_LEFTY + 1},
    },
};

void recomp::poll_inputs() {
    InputState.keys = SDL_GetKeyboardState(&InputState.numkeys);

    InputState.cur_controllers.clear();

    for (SDL_JoystickID id : InputState.controller_ids) {
        SDL_GameController* controller = SDL_GameControllerFromInstanceID(id);
        if (controller != nullptr) {
            InputState.cur_controllers.push_back(controller);
        }
    }
}

bool controller_button_state(int32_t input_id) {
    if (input_id >= 0 && input_id < SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX) {
        SDL_GameControllerButton button = (SDL_GameControllerButton)input_id;
        bool ret = false;

        for (const auto& controller : InputState.cur_controllers) {
            ret |= SDL_GameControllerGetButton(controller, button);
        }

        return ret;
    }
    return false;
}

float controller_axis_state(int32_t input_id) {
    if (abs(input_id) - 1 < SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_MAX) {
        SDL_GameControllerAxis axis = (SDL_GameControllerAxis)(abs(input_id) - 1);
        bool negative_range = input_id < 0;
        float ret = 0.0f;

        for (const auto& controller : InputState.cur_controllers) {
            float cur_val = SDL_GameControllerGetAxis(controller, axis) * (1/32768.0f);
            if (negative_range) {
                cur_val = -cur_val;
            }
            ret += std::clamp(cur_val, 0.0f, 1.0f);
        }

        return std::clamp(ret, 0.0f, 1.0f);
    }
    return false;
}

float recomp::get_input_analog(const recomp::InputField& field) {
    switch ((DeviceType)field.device_type) {
    case DeviceType::Keyboard:
    {
        if (InputState.keys && field.input_id >= 0 && field.input_id < InputState.numkeys) {
            return InputState.keys[field.input_id] ? 1.0f : 0.0f;
        }
        return 0.0f;
    }
    case DeviceType::ControllerDigital:
    {
        return controller_button_state(field.input_id) ? 1.0f : 0.0f;
    }
    case DeviceType::ControllerAnalog:
    {
        return controller_axis_state(field.input_id);
    }
    case DeviceType::Mouse:
    {
        // TODO mouse support
        return 0.0f;
    }
    }
}

float recomp::get_input_analog(const std::span<const recomp::InputField> fields) {
    float ret = 0.0f;
    for (const auto& field : fields) {
        ret += get_input_analog(field);
    }
    return std::clamp(ret, 0.0f, 1.0f);
}

bool recomp::get_input_digital(const recomp::InputField& field) {
    switch ((DeviceType)field.device_type) {
    case DeviceType::Keyboard:
    {
        if (InputState.keys && field.input_id >= 0 && field.input_id < InputState.numkeys) {
            return InputState.keys[field.input_id] != 0;
        }
        return false;
    }
    case DeviceType::ControllerDigital:
    {
        return controller_button_state(field.input_id);
    }
    case DeviceType::ControllerAnalog:
    {
        // TODO adjustable threshold
        return controller_axis_state(field.input_id) >= 0.5f;
    }
    case DeviceType::Mouse:
    {
        // TODO mouse support
        return false;
    }
    }
}

bool recomp::get_input_digital(const std::span<const recomp::InputField> fields) {
    bool ret = 0;
    for (const auto& field : fields) {
        ret |= get_input_digital(field);
    }
    return ret;
}

bool recomp::game_input_disabled() {
    // Disable input if any menu is open.
    return recomp::get_current_menu() != recomp::Menu::None;
}
