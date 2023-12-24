#include <cstdio>
#include <cassert>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <numeric>

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

static SDL_AudioCVT audio_convert;
static SDL_AudioDeviceID audio_device = 0;

// Samples per channel per second.
static uint32_t sample_rate = 48000;
static uint32_t output_sample_rate = 48000;
// Channel count.
constexpr uint32_t input_channels = 2;
static uint32_t output_channels = 2;

// Terminology: a frame is a collection of samples for each channel. e.g. 2 input samples is one input frame. This is unrelated to graphical frames.

// In order to prevent resampling discontinuities, the last few frames of the previous audio chunk are prepended to the current chunk before
// resampling it so there's enough information for interpolation.
constexpr uint32_t min_duplicated_frames = 32;
// The number of input frames to duplicate for interpolation to prevent discontinuities.
static uint32_t duplicated_input_frames;
// The number of output frames to skip for playback (to avoid playing duplicate inputs twice).
static uint32_t discarded_output_frames;

void queue_samples(int16_t* audio_data, size_t sample_count) {
    // Buffer for holding the output of swapping the audio channels. This is reused across
    // calls to reduce runtime allocations.
    static std::vector<float> swap_buffer;
    static std::vector<float> duplicated_sample_buffer;
    
    assert((sample_count / input_channels) / duplicated_input_frames * duplicated_input_frames == (sample_count / input_channels));

    if (duplicated_input_frames * input_channels > duplicated_sample_buffer.size()) {
        duplicated_sample_buffer.resize(duplicated_input_frames * input_channels);
    }

    size_t max_sample_count = std::max(sample_count, sample_count * audio_convert.len_mult);

    // Make sure the swap buffer is large enough to hold the audio data.
    if (max_sample_count > swap_buffer.size()) {
        swap_buffer.resize(max_sample_count);
    }
    
    // Copy the duplicated frames from last chunk into this chunk
    for (size_t i = 0; i < duplicated_input_frames * input_channels; i++) {
        swap_buffer[i] = duplicated_sample_buffer[i];
    }

    // Convert the audio from 16-bit values to floats and swap the audio channels into the
    // swap buffer to correct for the address xor caused by endianness handling.
    for (size_t i = 0; i < sample_count; i += input_channels) {
        swap_buffer[i + 0 + duplicated_input_frames * input_channels] = audio_data[i + 1] * (0.5f / 32768.0f);
        swap_buffer[i + 1 + duplicated_input_frames * input_channels] = audio_data[i + 0] * (0.5f / 32768.0f);
    }
    
    assert(sample_count > duplicated_input_frames * input_channels);

    // Copy the last converted samples into the duplicated sample buffer to reuse in resampling the next queued chunk.
    for (size_t i = 0; i < duplicated_input_frames * 2; i++) {
        duplicated_sample_buffer[i] = swap_buffer[i + sample_count];
    }
    
    audio_convert.buf = reinterpret_cast<Uint8*>(swap_buffer.data());
    audio_convert.len = (sample_count + duplicated_input_frames * input_channels) * sizeof(swap_buffer[0]);

    SDL_ConvertAudio(&audio_convert);

    // Queue the swapped audio data.
    SDL_QueueAudio(audio_device, swap_buffer.data() + output_channels * discarded_output_frames,
        sample_count * sizeof(swap_buffer[0]) * output_sample_rate * output_channels / (sample_rate * input_channels));
}

constexpr uint32_t bytes_per_frame = input_channels * sizeof(float);

size_t get_frames_remaining() {
    constexpr float buffer_offset_frames = 1.0f;
    // Get the number of remaining buffered audio bytes.
    uint32_t buffered_byte_count = SDL_GetQueuedAudioSize(audio_device);

    // Scale the byte count based on the ratio of sample rates and channel counts.
    buffered_byte_count = buffered_byte_count * 2 * sample_rate / output_sample_rate / output_channels;

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

void update_audio_converter() {
    SDL_BuildAudioCVT(&audio_convert, AUDIO_F32, 2, sample_rate, AUDIO_F32, output_channels, output_sample_rate);

    // Calculate the number of samples to duplicate and discard based on the greatest common denominator fo the input and output sample rates.
    // Keeping them at the same ratio as the sample rates themselves ensures an integer number of output samples are produced from an
    // integer number of input samples. 
    size_t rate_gcd = std::gcd(sample_rate, output_sample_rate);
    size_t gcd_input_samples = sample_rate / rate_gcd;
    size_t gcd_output_samples = output_sample_rate / rate_gcd;
    size_t num_duplicated_chunks = (gcd_input_samples + min_duplicated_frames - 1) / min_duplicated_frames;
    // Duplicate twice as many input frames as the corresponding skipped input frames as we need to prevent discontinuities at
    // both the start and end of a given chunk.
    duplicated_input_frames = num_duplicated_chunks * gcd_input_samples * 2;
    discarded_output_frames = num_duplicated_chunks * gcd_output_samples;
}

void set_frequency(uint32_t freq) {
    assert(freq == 32000 || freq == 48000);
    sample_rate = freq;
    
    update_audio_converter();
}

void reset_audio(uint32_t output_freq) {
    SDL_AudioSpec spec_desired{
        .freq = (int)output_freq,
        .format = AUDIO_F32,
        .channels = (Uint8)output_channels,
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

    output_sample_rate = output_freq;
    update_audio_converter();
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
    reset_audio(48000);

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
        .poll_input = recomp::poll_inputs,
        .get_input = recomp::get_n64_input,
    };

    ultramodern::start({}, audio_callbacks, input_callbacks, gfx_callbacks);

    return EXIT_SUCCESS;
}
