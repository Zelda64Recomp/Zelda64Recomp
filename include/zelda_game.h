#ifndef __ZELDA_GAME_H__
#define __ZELDA_GAME_H__

#include <cstdint>
#include <span>
#include <vector>

#include "librecomp/game.hpp"

namespace zelda64 {
    enum Game {
        OoT,
        MM
    };

    constexpr zelda64::Game default_game = Game::MM;

    zelda64::Game get_current_game();
    zelda64::Game swap_current_game();

    const recomp::GameEntry &get_game_entry(Game game);

    void quicksave_save();
    void quicksave_load();
    std::vector<uint8_t> decompress_mm(std::span<const uint8_t> compressed_rom);
    std::vector<uint8_t> decompress_oot(std::span<const uint8_t> compressed_rom);
};

#endif
