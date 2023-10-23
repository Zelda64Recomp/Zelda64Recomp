#ifndef __RT64_LAYER_H__
#define __RT64_LAYER_H__

#include "../portultra/multilibultra.hpp"

typedef struct {
    // void* hWnd;
    // void* hStatusBar;

    // int Reserved;

    unsigned char* HEADER;  /* This is the rom header (first 40h bytes of the rom) */
    unsigned char* RDRAM;
    unsigned char* DMEM;
    unsigned char* IMEM;

    unsigned int* MI_INTR_REG;

    unsigned int* DPC_START_REG;
    unsigned int* DPC_END_REG;
    unsigned int* DPC_CURRENT_REG;
    unsigned int* DPC_STATUS_REG;
    unsigned int* DPC_CLOCK_REG;
    unsigned int* DPC_BUFBUSY_REG;
    unsigned int* DPC_PIPEBUSY_REG;
    unsigned int* DPC_TMEM_REG;

    unsigned int* VI_STATUS_REG;
    unsigned int* VI_ORIGIN_REG;
    unsigned int* VI_WIDTH_REG;
    unsigned int* VI_INTR_REG;
    unsigned int* VI_V_CURRENT_LINE_REG;
    unsigned int* VI_TIMING_REG;
    unsigned int* VI_V_SYNC_REG;
    unsigned int* VI_H_SYNC_REG;
    unsigned int* VI_LEAP_REG;
    unsigned int* VI_H_START_REG;
    unsigned int* VI_V_START_REG;
    unsigned int* VI_V_BURST_REG;
    unsigned int* VI_X_SCALE_REG;
    unsigned int* VI_Y_SCALE_REG;

    void (*CheckInterrupts)(void);

    unsigned int version;
    unsigned int* SP_STATUS_REG;
    const unsigned int* RDRAM_SIZE;
} GFX_INFO;

#ifdef _WIN32
#define DLLEXPORT extern "C" __declspec(dllexport)  
#define DLLIMPORT extern "C" __declspec(dllimport)
#define CALL   __cdecl
#else
#define DLLEXPORT extern "C" __attribute__((visibility("default")))
#define DLLIMPORT extern "C"
#endif

// Dynamic loading
//DLLEXPORT int (CALL *InitiateGFX)(GFX_INFO Gfx_Info) = nullptr;
//DLLEXPORT void (CALL *ProcessRDPList)(void) = nullptr;
//DLLEXPORT void (CALL *ProcessDList)(void) = nullptr;
//DLLEXPORT void (CALL *UpdateScreen)(void) = nullptr;
//DLLEXPORT void (CALL *PumpEvents)(void) = nullptr;

#if defined(_WIN32)
extern "C" int InitiateGFXWindows(GFX_INFO Gfx_Info, HWND hwnd);
#elif defined(__ANDROID__)
static_assert(false && "Unimplemented");
#elif defined(__linux__)
extern "C" int InitiateGFXLinux(GFX_INFO Gfx_Info, Window window, Display *display);
#else
static_assert(false && "Unimplemented");
#endif
DLLIMPORT void ProcessRDPList(void);
DLLIMPORT void ProcessDList(void);
DLLIMPORT void UpdateScreen(void);
DLLIMPORT void ChangeWindow(void);

#endif

