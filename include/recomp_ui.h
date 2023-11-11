#ifndef __RECOMP_UI__
#define __RECOMP_UI__

#include <memory>

#include "SDL.h"

void queue_event(const SDL_Event& event);
bool try_deque_event(SDL_Event& out);

namespace Rml {
	class ElementDocument;
	class EventListenerInstancer;
}

std::unique_ptr<Rml::EventListenerInstancer> make_event_listener_instancer();

enum class Menu {
	Launcher,
	None
};

void set_current_menu(Menu menu);

#endif
