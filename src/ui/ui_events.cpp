#define _CRT_SECURE_NO_WARNINGS

#include "recomp_ui.h"
#include "../../ultramodern/ultramodern.hpp"

#include "nfd.h"
#include "RmlUi/Core.h"

using event_handler_t = void(Rml::Event&);

class UiEventListener : public Rml::EventListener {
	event_handler_t* handler_;
public:
	UiEventListener(event_handler_t* handler) : handler_(handler) {}
	void ProcessEvent(Rml::Event& event) override {
		handler_(event);
	}
};

class UiEventListenerInstancer : public Rml::EventListenerInstancer {
	std::unordered_map<Rml::String, UiEventListener> listener_map_;
public:
	Rml::EventListener* InstanceEventListener(const Rml::String& value, Rml::Element* element) override {
		printf("Instancing event listener for %s\n", value.c_str());
		auto find_it = listener_map_.find(value);

		if (find_it != listener_map_.end()) {
			return &find_it->second;
		}

		return nullptr;
	}

	void register_event(const Rml::String& value, event_handler_t* handler) {
		listener_map_.emplace(value, UiEventListener{ handler });
	}
};

std::unique_ptr<Rml::EventListenerInstancer> make_event_listener_instancer() {
	std::unique_ptr<UiEventListenerInstancer> ret = std::make_unique<UiEventListenerInstancer>();

	ret->register_event("start_game",
		[](Rml::Event& event) {
			ultramodern::start_game(0);
			set_current_menu(Menu::None);
		}
	);

	return ret;
}
