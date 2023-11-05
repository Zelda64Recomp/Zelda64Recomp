#ifndef __RECOMP_UI__
#define __RECOMP_UI__

#include "SDL.h"

void queue_event(const SDL_Event& event);
bool try_deque_event(SDL_Event& out);

#endif
