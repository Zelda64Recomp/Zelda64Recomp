#include <atomic>

#include "../ultramodern/ultramodern.hpp"
#include "recomp.h"
#include "recomp_input.h"
#include "recomp_ui.h"
#include "SDL.h"

std::atomic_int32_t mouse_wheel_pos = 0;
std::vector<SDL_JoystickID> controllers{};

bool sdl_event_filter(void* userdata, SDL_Event* event) {
    switch (event->type) {
    //case SDL_EventType::SDL_KEYUP:
    //case SDL_EventType::SDL_KEYDOWN:
    //    {
    //        const Uint8* key_states = SDL_GetKeyboardState(nullptr);
    //        int new_button = 0;

    //        for (const auto& mapping : keyboard_button_map) {
    //            if (key_states[mapping.first]) {
    //                new_button |= mapping.second;
    //            }
    //        }

    //        button = new_button;

    //        stick_x = (100.0f / 100.0f) * (key_states[SDL_Scancode::SDL_SCANCODE_D] - key_states[SDL_Scancode::SDL_SCANCODE_A]);
    //        stick_y = (100.0f / 100.0f) * (key_states[SDL_Scancode::SDL_SCANCODE_W] - key_states[SDL_Scancode::SDL_SCANCODE_S]);
    //    }
    //    break;
    case SDL_EventType::SDL_CONTROLLERDEVICEADDED:
        {
            SDL_ControllerDeviceEvent* controller_event = (SDL_ControllerDeviceEvent*)event;
            SDL_GameController* controller = SDL_GameControllerOpen(controller_event->which);
            printf("Controller added: %d\n", controller_event->which);
            if (controller != nullptr) {
                printf("  Instance ID: %d\n", SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller)));
                controllers.push_back(SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller)));
            }
        }
        break;
    case SDL_EventType::SDL_CONTROLLERDEVICEREMOVED:
        {
            SDL_ControllerDeviceEvent* controller_event = (SDL_ControllerDeviceEvent*)event;
            printf("Controller removed: %d\n", controller_event->which);
            std::erase(controllers, controller_event->which);
        }
        break;
    case SDL_EventType::SDL_QUIT:
        ultramodern::quit();
        return true;
    case SDL_EventType::SDL_MOUSEWHEEL:
        {
            SDL_MouseWheelEvent* wheel_event = (SDL_MouseWheelEvent*)event;    
            mouse_wheel_pos.fetch_add(wheel_event->y * (wheel_event->direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1));
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

std::vector<recomp::InputState> recomp::get_input_states() {
    std::vector<InputState> ret{};

    int32_t mouse_x;
    int32_t mouse_y;

    Uint32 sdl_mouse_buttons = SDL_GetMouseState(&mouse_x, &mouse_y);

    struct MouseButtonMapping {
        Sint32 input;
        decltype(MouseState::buttons) output;
    };
    static const std::vector<MouseButtonMapping> input_mouse_map{
        { SDL_BUTTON_LMASK,  MouseState::LEFT },
        { SDL_BUTTON_RMASK,  MouseState::RIGHT },
        { SDL_BUTTON_MMASK,  MouseState::MIDDLE },
        { SDL_BUTTON_X1MASK, MouseState::BACK },
        { SDL_BUTTON_X2MASK, MouseState::FORWARD },
    };

    decltype(MouseState::buttons) mouse_buttons = 0;
    for (const MouseButtonMapping& mapping : input_mouse_map) {
        if (sdl_mouse_buttons & mapping.input) {
            mouse_buttons |= mapping.output;
        }
    }

    if (mouse_buttons & MouseState::FORWARD) {
        printf("forward\n");
    }

    if (mouse_buttons & MouseState::BACK) {
        printf("back\n");
    }

    ret.emplace_back(
        MouseState {
            .wheel_pos = mouse_wheel_pos,
            .position_x = mouse_x,
            .position_y = mouse_y,
            .buttons = mouse_buttons
        }
    );

    for (SDL_JoystickID controller_id : controllers) {
        struct InputButtonMapping {
            SDL_GameControllerButton input;
            decltype(ControllerState::buttons) output;
        };
        static const std::vector<InputButtonMapping> input_button_map{
            { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START,         ControllerState::BUTTON_START },
            { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A,             ControllerState::BUTTON_SOUTH },
            { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B,             ControllerState::BUTTON_EAST },
            { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X,             ControllerState::BUTTON_WEST },
            { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y,             ControllerState::BUTTON_NORTH },
            { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  ControllerState::BUTTON_L1 },
            { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, ControllerState::BUTTON_R1 },
            { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSTICK,     ControllerState::BUTTON_L3 },
            { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSTICK,    ControllerState::BUTTON_R3 },
            { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_LEFT,     ControllerState::BUTTON_DPAD_LEFT },
            { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_RIGHT,    ControllerState::BUTTON_DPAD_RIGHT },
            { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_UP,       ControllerState::BUTTON_DPAD_UP },
            { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_DOWN,     ControllerState::BUTTON_DPAD_DOWN },
        };
        SDL_GameController* controller = SDL_GameControllerFromInstanceID(controller_id);
        decltype(ControllerState::buttons) buttons = 0;

        for (const InputButtonMapping& mapping : input_button_map) {
            if (SDL_GameControllerGetButton(controller, mapping.input)) {
                buttons |= mapping.output;
            }
        }

        auto& new_input_state = ret.emplace_back(
            ControllerState {
                .buttons = buttons,
                .axes = {},
            }
        );
        auto& new_state = std::get<ControllerState>(new_input_state);
        new_state.axes[ControllerState::AXIS_LEFT_X] = SDL_GameControllerGetAxis(controller, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX) * (1/32768.0f);
        new_state.axes[ControllerState::AXIS_LEFT_Y] = SDL_GameControllerGetAxis(controller, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY) * (1/32768.0f);
        new_state.axes[ControllerState::AXIS_RIGHT_X] = SDL_GameControllerGetAxis(controller, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX) * (1/32768.0f);
        new_state.axes[ControllerState::AXIS_RIGHT_Y] = SDL_GameControllerGetAxis(controller, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY) * (1/32768.0f);
        new_state.axes[ControllerState::AXIS_LEFT_TRIGGER] = SDL_GameControllerGetAxis(controller, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERLEFT) * (1/32768.0f);
        new_state.axes[ControllerState::AXIS_LEFT_TRIGGER] = SDL_GameControllerGetAxis(controller, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERRIGHT) * (1/32768.0f);
    }

    return ret;
}

// TODO figure out a way to handle this more generally without exposing SDL to controls.cpp
void recomp::get_keyboard_input(uint16_t* buttons_out, float* x_out, float* y_out) {
    static const std::vector<std::pair<SDL_Scancode, int>> keyboard_button_map{
        { SDL_Scancode::SDL_SCANCODE_LEFT,   0x0002 }, // c left
        { SDL_Scancode::SDL_SCANCODE_RIGHT,  0x0001 }, // c right
        { SDL_Scancode::SDL_SCANCODE_UP,     0x0008 }, // c up
        { SDL_Scancode::SDL_SCANCODE_DOWN,   0x0004 }, // c down
        { SDL_Scancode::SDL_SCANCODE_RETURN, 0x1000 }, // start
        { SDL_Scancode::SDL_SCANCODE_SPACE,  0x8000 }, // a
        { SDL_Scancode::SDL_SCANCODE_LSHIFT, 0x4000 }, // b
        { SDL_Scancode::SDL_SCANCODE_Q,      0x2000 }, // z
        { SDL_Scancode::SDL_SCANCODE_E,      0x0020 }, // l
        { SDL_Scancode::SDL_SCANCODE_R,      0x0010 }, // r
        { SDL_Scancode::SDL_SCANCODE_J,      0x0200 }, // dpad left
        { SDL_Scancode::SDL_SCANCODE_L,      0x0100 }, // dpad right
        { SDL_Scancode::SDL_SCANCODE_I,      0x0800 }, // dpad up
        { SDL_Scancode::SDL_SCANCODE_K,      0x0400 }, // dpad down
    };
    
    const Uint8* key_states = SDL_GetKeyboardState(nullptr);

    *buttons_out = 0;

    for (const auto& mapping : keyboard_button_map) {
        if (key_states[mapping.first]) {
            *buttons_out |= mapping.second;
        }
    }

    *x_out = (100.0f / 100.0f) * (key_states[SDL_Scancode::SDL_SCANCODE_D] - key_states[SDL_Scancode::SDL_SCANCODE_A]);
    *y_out = (100.0f / 100.0f) * (key_states[SDL_Scancode::SDL_SCANCODE_W] - key_states[SDL_Scancode::SDL_SCANCODE_S]);
}
