#include <cstdio>
#include <cassert>
#include <unordered_map>
#include <vector>
#include <filesystem>

#include "../../portultra/ultra64.h"
#include "../../portultra/multilibultra.hpp"
#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include "SDL.h"
#else
#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "SDL_syswm.h"
#endif

extern "C" void init();
/*extern "C"*/ void start(Multilibultra::WindowHandle window_handle, const Multilibultra::audio_callbacks_t* audio_callbacks, const Multilibultra::input_callbacks_t* input_callbacks);

template<typename... Ts>
void exit_error(const char* str, Ts ...args) {
    // TODO pop up an error
    ((void)fprintf(stderr, str, args), ...);
    assert(false);
    std::quick_exit(EXIT_FAILURE);
}


std::vector<std::pair<SDL_Scancode, int>> keyboard_button_map{
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

struct GameControllerAxisMapping {
    SDL_GameControllerAxis axis;
    int threshold; // Positive or negative to indicate direction
    uint16_t output_mask;
};

constexpr int controller_default_threshold = 20000;

std::vector<GameControllerAxisMapping> controller_axis_map{
    { SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX,      -controller_default_threshold, 0x0002 }, // c left
    { SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX,       controller_default_threshold, 0x0001 }, // c right
    { SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY,      -controller_default_threshold, 0x0008 }, // c up
    { SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY,       controller_default_threshold, 0x0004 }, // c down
    { SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERLEFT, 10000,                         0x2000 }, // z
    //{ SDL_Scancode::SDL_SCANCODE_RIGHT,  0x0001 }, // c right
    //{ SDL_Scancode::SDL_SCANCODE_UP,     0x0008 }, // c up
    //{ SDL_Scancode::SDL_SCANCODE_DOWN,   0x0004 }, // c down
    //{ SDL_Scancode::SDL_SCANCODE_RETURN, 0x1000 }, // start
    //{ SDL_Scancode::SDL_SCANCODE_SPACE,  0x8000 }, // a
    //{ SDL_Scancode::SDL_SCANCODE_LSHIFT, 0x4000 }, // b
    //{ SDL_Scancode::SDL_SCANCODE_Q,      0x2000 }, // z
    //{ SDL_Scancode::SDL_SCANCODE_E,      0x0020 }, // l
    //{ SDL_Scancode::SDL_SCANCODE_R,      0x0010 }, // r
    //{ SDL_Scancode::SDL_SCANCODE_J,      0x0200 }, // dpad left
    //{ SDL_Scancode::SDL_SCANCODE_L,      0x0100 }, // dpad right
    //{ SDL_Scancode::SDL_SCANCODE_I,      0x0800 }, // dpad up
    //{ SDL_Scancode::SDL_SCANCODE_K,      0x0400 }, // dpad down
};

struct GameControllerButtonMapping {
    SDL_GameControllerButton button;
    uint16_t output_mask;
};
std::vector<GameControllerButtonMapping> controller_button_map{
    { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START,         0x1000 }, // start
    { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A,             0x8000 }, // a
    { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B,             0x4000 }, // b
    { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X,             0x4000 }, // b
    { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  0x0020 }, // l
    { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, 0x0010 }, // r
    { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_LEFT,     0x0200 }, // dpad left
    { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_RIGHT,    0x0100 }, // dpad right
    { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_UP,       0x0800 }, // dpad up
    { SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_DOWN,     0x0400 }, // dpad down
};

std::vector<SDL_JoystickID> controllers{};

int sdl_event_filter(void* userdata, SDL_Event* event) {
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
        std::quick_exit(EXIT_SUCCESS);
        break;
    }
    return 1;
}

Multilibultra::gfx_callbacks_t::gfx_data_t create_gfx() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) > 0) {
        exit_error("Failed to initialize SDL2: %s\n", SDL_GetError());
    }

    return {};
}

SDL_Window* window;

Multilibultra::WindowHandle create_window(Multilibultra::gfx_callbacks_t::gfx_data_t) {
    window = SDL_CreateWindow("Majora's Mask", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_RESIZABLE);

    if (window == nullptr) {
        exit_error("Failed to create window: %s\n", SDL_GetError());
    }

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);

#if defined(_WIN32)
    return wmInfo.info.win.window;
#elif defined(__ANDROID__)
    static_assert(false && "Unimplemented");
#elif defined(__linux__)
    return Multilibultra::WindowHandle{ wmInfo.info.x11.display, wmInfo.info.x11.window };
#else
    static_assert(false && "Unimplemented");
#endif
}

void update_gfx(void*) {
    // Handle events
    constexpr int max_events_per_frame = 16;
    SDL_Event cur_event;
    int i = 0;
    while (i++ < max_events_per_frame && SDL_PollEvent(&cur_event)) {
        sdl_event_filter(nullptr, &cur_event);
    }
}

void get_input(uint16_t* buttons_out, float* x_out, float* y_out) {
    uint16_t cur_buttons = 0;
    float cur_x = 0.0f;
    float cur_y = 0.0f;

    const Uint8* key_states = SDL_GetKeyboardState(nullptr);
    int new_button = 0;

    for (const auto& mapping : keyboard_button_map) {
        if (key_states[mapping.first]) {
            cur_buttons |= mapping.second;
        }
    }

    cur_x += (100.0f / 100.0f) * (key_states[SDL_Scancode::SDL_SCANCODE_D] - key_states[SDL_Scancode::SDL_SCANCODE_A]);
    cur_y += (100.0f / 100.0f) * (key_states[SDL_Scancode::SDL_SCANCODE_W] - key_states[SDL_Scancode::SDL_SCANCODE_S]);

    for (SDL_JoystickID controller_id : controllers) {
        SDL_GameController* controller = SDL_GameControllerFromInstanceID(controller_id);
        if (controller != nullptr) {
            cur_x += SDL_GameControllerGetAxis(controller, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX) * (1/32768.0f);
            cur_y -= SDL_GameControllerGetAxis(controller, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY) * (1/32768.0f);
        }

        for (const auto& mapping : controller_axis_map) {
            int input_value = SDL_GameControllerGetAxis(controller, mapping.axis);
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

        for (const auto& mapping : controller_button_map) {
            int input_value = SDL_GameControllerGetButton(controller, mapping.button);
            if (input_value) {
                cur_buttons |= mapping.output_mask;
            }
        }
    }

    *buttons_out = cur_buttons;
    cur_x = std::clamp(cur_x, -1.0f, 1.0f);
    cur_y = std::clamp(cur_y, -1.0f, 1.0f);
    *x_out = cur_x;
    *y_out = cur_y;
}

static SDL_AudioDeviceID audio_device = 0;
static uint32_t sample_rate = 48000;

void queue_samples(int16_t* audio_data, size_t sample_count) {
    // Buffer for holding the output of swapping the audio channels. This is reused across
    // calls to reduce runtime allocations.
    static std::vector<float> swap_buffer;

    // Make sure the swap buffer is large enough to hold all the incoming audio data.
    if (sample_count > swap_buffer.size()) {
        swap_buffer.resize(sample_count);
    }

    // Convert the audio from 16-bit values to floats and swap the audio channels into the
    // swap buffer to correct for the address xor caused by endianness handling.
    for (size_t i = 0; i < sample_count; i += 2) {
        swap_buffer[i + 0] = audio_data[i + 1] * (0.5f / 32768.0f);
        swap_buffer[i + 1] = audio_data[i + 0] * (0.5f / 32768.0f);
    }

    // Queue the swapped audio data.
    SDL_QueueAudio(audio_device, swap_buffer.data(), sample_count * sizeof(swap_buffer[0]));
}

constexpr int channel_count = 2;
constexpr int bytes_per_frame = channel_count * sizeof(float);

size_t get_frames_remaining() {
    constexpr float buffer_offset_frames = 1.0f;
    // Get the number of remaining buffered audio bytes.
    uint32_t buffered_byte_count = SDL_GetQueuedAudioSize(audio_device);

    // Adjust the reported count to be some number of refreshes in the future, which helps ensure that
    // there are enough samples even if the audio thread experiences a small amount of lag. This prevents
    // audio popping on games that use the buffered audio byte count to determine how many samples
    // to generate.
    uint32_t frames_per_vi = (sample_rate / 60);
    if (buffered_byte_count > (buffer_offset_frames * bytes_per_frame * frames_per_vi)) {
        buffered_byte_count -= (buffer_offset_frames * bytes_per_frame * frames_per_vi);
    }
    else {
        buffered_byte_count = 0;
    }
    // Convert from byte count to sample count.
    return buffered_byte_count / bytes_per_frame;
}

void set_frequency(uint32_t freq) {
    if (audio_device != 0) {
        SDL_CloseAudioDevice(audio_device);
    }
    SDL_AudioSpec spec_desired{
        .freq = (int)freq,
        .format = AUDIO_F32,
        .channels = channel_count,
        .silence = 0, // calculated
        .samples = 0x100, // Fairly small sample count to reduce the latency of internal buffering
        .padding = 0, // unused
        .size = 0, // calculated
        .callback = nullptr,//feed_audio, // Use a callback as QueueAudio causes popping
        .userdata = nullptr
    };

    audio_device = SDL_OpenAudioDevice(nullptr, false, &spec_desired, nullptr, 0);
    if (audio_device == 0) {
        exit_error("SDL error opening audio device: %s\n", SDL_GetError());
    }
    SDL_PauseAudioDevice(audio_device, 0);
    sample_rate = freq;
}

int main(int argc, char** argv) {

#ifdef _WIN32
    // Set up console output to accept UTF-8 on windows
    SetConsoleOutputCP(CP_UTF8);

    // Change to a font that supports Japanese characters
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof cfi;
    cfi.nFont = 0;
    cfi.dwFontSize.X = 0;
    cfi.dwFontSize.Y = 16;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"NSimSun");
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
#else
    std::setlocale(LC_ALL, "en_US.UTF-8");
#endif

    printf("Current dir: %ls\n", std::filesystem::current_path().c_str());

    // Initialize SDL audio.
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    // Pick an initial dummy sample rate; this will be set by the game later to the true sample rate.
    set_frequency(sample_rate);

    init();

    Multilibultra::gfx_callbacks_t gfx_callbacks{
        .create_gfx = create_gfx,
        .create_window = create_window,
        .update_gfx = update_gfx,
    };

    Multilibultra::audio_callbacks_t audio_callbacks{
        .queue_samples = queue_samples,
        .get_frames_remaining = get_frames_remaining,
        .set_frequency = set_frequency,
    };

    Multilibultra::input_callbacks_t input_callbacks{
        .get_input = get_input,
    };

    //create_gfx();
    //void* window_handle = create_window(nullptr);

    Multilibultra::set_gfx_callbacks(&gfx_callbacks);
    start(Multilibultra::WindowHandle{}, &audio_callbacks, &input_callbacks);

    // Do nothing forever
    while (1) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
        //update_gfx(nullptr);
        //std::this_thread::sleep_for(1ms);
    }

    return EXIT_SUCCESS;
}
