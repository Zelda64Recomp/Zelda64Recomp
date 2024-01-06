#ifndef __RECOMP_UI__
#define __RECOMP_UI__

#include <memory>
#include <string>

#include "SDL.h"

namespace Rml {
	class ElementDocument;
	class EventListenerInstancer;
	class Context;
	class Event;
}

namespace recomp {
	class UiEventListenerInstancer;

	class MenuController {
	public:
		virtual ~MenuController() {}
		virtual Rml::ElementDocument* load_document(Rml::Context* context) = 0;
		virtual void register_events(UiEventListenerInstancer& listener) = 0;
		virtual void make_bindings(Rml::Context* context) = 0;
	};

	std::unique_ptr<MenuController> create_launcher_menu();
	std::unique_ptr<MenuController> create_config_menu();

	using event_handler_t = void(Rml::Event&);

	void queue_event(const SDL_Event& event);
	bool try_deque_event(SDL_Event& out);

	std::unique_ptr<UiEventListenerInstancer> make_event_listener_instancer();
	void register_event(UiEventListenerInstancer& listener, const std::string& name, event_handler_t* handler);

	enum class Menu {
		Launcher,
		Config,
		None
	};

	void set_current_menu(Menu menu);
	void destroy_ui();
	void apply_color_hack();
}

#endif
