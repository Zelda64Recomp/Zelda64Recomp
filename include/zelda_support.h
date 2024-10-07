#ifndef __ZELDA_SUPPORT_H__
#define __ZELDA_SUPPORT_H__

#include <functional>

namespace zelda64 {
    void dispatch_on_main_thread(std::function<void()> func);

#ifdef __APPLE__
    const char* get_bundle_resource_directory();
#endif
}

#endif
