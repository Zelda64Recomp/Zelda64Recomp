#include "patches.h"
#include "ui_funcs.h"

// @recomp Patched to run UI callbacks.
RECOMP_PATCH void Graph_UpdateGame(GameState* gameState) {
    recomp_run_ui_callbacks();

    GameState_GetInput(gameState);
    GameState_IncrementFrameCount(gameState);
    if (SREG(20) < 3) {
        Audio_Update();
    }
}

