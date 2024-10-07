#ifndef __APPLE__

#include "zelda_support.h"
#include <functional>

void zelda64::dispatch_on_main_thread(std::function<void()> func) {
    func();
}

#endif