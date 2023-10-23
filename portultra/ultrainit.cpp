#include "ultra64.h"
#include "multilibultra.hpp"

void Multilibultra::preinit(uint8_t* rdram, uint8_t* rom, Multilibultra::WindowHandle window_handle) {
    Multilibultra::set_main_thread();
    Multilibultra::init_events(rdram, rom, window_handle);
    Multilibultra::init_timers(rdram);
    Multilibultra::init_audio();
    Multilibultra::save_init();
}

extern "C" void osInitialize() {
    Multilibultra::init_scheduler();
}
