#include "ultra64.h"
#include "multilibultra.hpp"
#include <cassert>

static uint32_t sample_rate = 48000;

static Multilibultra::audio_callbacks_t audio_callbacks;

void set_audio_callbacks(const Multilibultra::audio_callbacks_t& callbacks) {
	audio_callbacks = callbacks;
}

void Multilibultra::init_audio() {
	// Pick an initial dummy sample rate; this will be set by the game later to the true sample rate.
	set_audio_frequency(48000);
}

void Multilibultra::set_audio_frequency(uint32_t freq) {
	if (audio_callbacks.set_frequency) {
		audio_callbacks.set_frequency(freq);
	}
	sample_rate = freq;
}

void Multilibultra::queue_audio_buffer(RDRAM_ARG PTR(int16_t) audio_data_, uint32_t byte_count) {
	// Buffer for holding the output of swapping the audio channels. This is reused across
	// calls to reduce runtime allocations.
	static std::vector<int16_t> swap_buffer;

	// Ensure that the byte count is an integer multiple of samples.
	assert((byte_count & 1) == 0);

	// Calculate the number of samples from the number of bytes.
	uint32_t sample_count = byte_count / sizeof(int16_t);

	// Make sure the swap buffer is large enough to hold all the incoming audio data.
	if (sample_count > swap_buffer.size()) {
		swap_buffer.resize(sample_count);
	}

	// Swap the audio channels into the swap buffer to correct for the address xor caused by endianness handling.
	int16_t* audio_data = TO_PTR(int16_t, audio_data_);
	for (size_t i = 0; i < sample_count; i += 2) {
		swap_buffer[i + 0] = audio_data[i + 1];
		swap_buffer[i + 1] = audio_data[i + 0];
	}

	// Queue the swapped audio data.
	if (audio_callbacks.queue_samples) {
		audio_callbacks.queue_samples(swap_buffer.data(), sample_count);
	}
}

// For SDL2
//uint32_t buffer_offset_frames = 1;
// For Godot
float buffer_offset_frames = 0.5f;

// If there's ever any audio popping, check here first. Some games are very sensitive to
// the remaining sample count and reporting a number that's too high here can lead to issues.
// Reporting a number that's too low can lead to audio lag in some games.
uint32_t Multilibultra::get_remaining_audio_bytes() {
	// Get the number of remaining buffered audio bytes.
	uint32_t buffered_byte_count;
	if (audio_callbacks.get_frames_remaining != nullptr) {
		buffered_byte_count = audio_callbacks.get_frames_remaining() * 2 * sizeof(int16_t);
	}
	else {
		buffered_byte_count = 100;
	}
	 // Adjust the reported count to be some number of refreshes in the future, which helps ensure that
	 // there are enough samples even if the audio thread experiences a small amount of lag. This prevents
	 // audio popping on games that use the buffered audio byte count to determine how many samples
	 // to generate.
	 uint32_t samples_per_vi = (sample_rate / 60);
	 if (buffered_byte_count > static_cast<uint32_t>(buffer_offset_frames * sizeof(int16_t) * samples_per_vi)) {
	 	buffered_byte_count -= static_cast<uint32_t>(buffer_offset_frames * sizeof(int16_t) * samples_per_vi);
	 }
	 else {
	 	buffered_byte_count = 0;
	 }
	 return buffered_byte_count;
}
