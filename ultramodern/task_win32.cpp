#ifdef _WIN32

#include <Windows.h>

#include "ultra64.h"
#include "ultramodern.hpp"

extern "C" unsigned int sleep(unsigned int seconds) {
    Sleep(seconds * 1000);
    return 0;
}

#endif