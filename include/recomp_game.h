#ifndef __RECOMP_GAME__
#define __RECOMP_GAME__

#include <vector>
#include <filesystem>

#include "recomp.h"
#include "../ultramodern/ultramodern.hpp"
#include "rt64_layer.h"

namespace recomp {
	enum class Game {
		OoT,
		MM,
		None,
		Quit
	};
	enum class RomValidationError {
		Good,
		FailedToOpen,
		NotARom,
		IncorrectRom,
		NotYet,
		IncorrectVersion,
		OtherError
	};
	void check_all_stored_roms();
	bool load_stored_rom(Game game);
	RomValidationError select_rom(const std::filesystem::path& rom_path, Game game);
	bool is_rom_valid(Game game);
	bool is_rom_loaded();
	void set_rom_contents(std::vector<uint8_t>&& new_rom);
	void do_rom_read(uint8_t* rdram, gpr ram_address, uint32_t physical_addr, size_t num_bytes);
	void do_rom_pio(uint8_t* rdram, gpr ram_address, uint32_t physical_addr);
	void start(ultramodern::WindowHandle window_handle, const ultramodern::audio_callbacks_t& audio_callbacks, const ultramodern::input_callbacks_t& input_callbacks, const ultramodern::gfx_callbacks_t& gfx_callbacks);
	void start_game(Game game);
	void message_box(const char* message);
}

#endif
