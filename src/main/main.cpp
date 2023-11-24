#include <cstdio>
#include <cassert>
#include <unordered_map>
#include <vector>
#include <filesystem>

#include "../../ultramodern/ultra64.h"
#include "../../ultramodern/ultramodern.hpp"
#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include "SDL.h"
#else
#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"
#endif

#include "recomp_ui.h"
#include "recomp_input.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "SDL_syswm.h"
#endif

extern "C" void init();
/*extern "C"*/ void start(ultramodern::WindowHandle window_handle, const ultramodern::audio_callbacks_t* audio_callbacks, const ultramodern::input_callbacks_t* input_callbacks);

template<typename... Ts>
void exit_error(const char* str, Ts ...args) {
    // TODO pop up an error
    ((void)fprintf(stderr, str, args), ...);
    assert(false);
    std::quick_exit(EXIT_FAILURE);
}

ultramodern::gfx_callbacks_t::gfx_data_t create_gfx() {
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "system");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) > 0) {
        exit_error("Failed to initialize SDL2: %s\n", SDL_GetError());
    }

    return {};
}

SDL_Window* window;

ultramodern::WindowHandle create_window(ultramodern::gfx_callbacks_t::gfx_data_t) {
    window = SDL_CreateWindow("Recomp", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_RESIZABLE );

    if (window == nullptr) {
        exit_error("Failed to create window: %s\n", SDL_GetError());
    }

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);

#if defined(_WIN32)
    return ultramodern::WindowHandle{ wmInfo.info.win.window, GetCurrentThreadId() };
#elif defined(__ANDROID__)
    static_assert(false && "Unimplemented");
#elif defined(__linux__)
    return ultramodern::WindowHandle{ wmInfo.info.x11.display, wmInfo.info.x11.window };
#else
    static_assert(false && "Unimplemented");
#endif
}

void update_gfx(void*) {
    recomp::handle_events();
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

    ultramodern::gfx_callbacks_t gfx_callbacks{
        .create_gfx = create_gfx,
        .create_window = create_window,
        .update_gfx = update_gfx,
    };

    ultramodern::audio_callbacks_t audio_callbacks{
        .queue_samples = queue_samples,
        .get_frames_remaining = get_frames_remaining,
        .set_frequency = set_frequency,
    };

    ultramodern::input_callbacks_t input_callbacks{
        .get_input = recomp::get_n64_input,
    };

    ultramodern::start({}, audio_callbacks, input_callbacks, gfx_callbacks);

    return EXIT_SUCCESS;
}
