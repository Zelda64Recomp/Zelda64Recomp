#include <atomic>
#include "recomp_debug.h"
#include "recomp_helpers.h"
#include "../patches/input.h"

std::atomic<uint16_t> pending_warp = 0xFFFF;
std::atomic<uint32_t> pending_set_time = 0xFFFF;

void recomp::do_warp(int area, int scene, int entrance) {
    const recomp::SceneWarps game_scene = recomp::game_warps[area].scenes[scene];
    int game_scene_index = game_scene.index;
    pending_warp.store(((game_scene_index & 0xFF) << 8) | ((entrance & 0x0F) << 4));
}

extern "C" void recomp_get_pending_warp(uint8_t* rdram, recomp_context* ctx) {
    // Return the current warp value and reset it.
    _return(ctx, pending_warp.exchange(0xFFFF));
}

void recomp::set_time(uint8_t day, uint8_t hour, uint8_t minute) {
    pending_set_time.store((day << 16) | (uint16_t(hour) << 8) | minute);
}

extern "C" void recomp_get_pending_set_time(uint8_t* rdram, recomp_context* ctx) {
    // Return the current set time value and reset it.
    _return(ctx, pending_set_time.exchange(0xFFFF));
}
