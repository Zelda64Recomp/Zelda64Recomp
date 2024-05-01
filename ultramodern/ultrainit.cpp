#include "ultra64.h"
#include "ultramodern.hpp"

void ultramodern::preinit(RDRAM_ARG ultramodern::WindowHandle window_handle) {
    ultramodern::set_main_thread();
    ultramodern::init_events(PASS_RDRAM window_handle);
    ultramodern::init_timers(PASS_RDRAM1);
    ultramodern::init_audio();
    ultramodern::init_saving(PASS_RDRAM1);
    ultramodern::init_thread_cleanup();
}

extern "C" void osInitialize() {
}
