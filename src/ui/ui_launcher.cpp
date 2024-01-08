#include "recomp_ui.h"
#include "../../ultramodern/ultramodern.hpp"
#include "RmlUi/Core.h"

class LauncherMenu : public recomp::MenuController {
public:
    LauncherMenu() {

    }
	~LauncherMenu() override {

	}
	Rml::ElementDocument* load_document(Rml::Context* context) override {
        return context->LoadDocument("assets/launcher.rml");
	}
	void register_events(recomp::UiEventListenerInstancer& listener) override {
		recomp::register_event(listener, "start_game",
			[](const std::string& param, Rml::Event& event) {
				ultramodern::start_game(0);
				recomp::set_current_menu(recomp::Menu::Config);
			}
		);
		recomp::register_event(listener, "exit_game",
			[](const std::string& param, Rml::Event& event) {
				ultramodern::quit();
			}
		);
	}
	void make_bindings(Rml::Context* context) override {

	}
};

std::unique_ptr<recomp::MenuController> recomp::create_launcher_menu() {
    return std::make_unique<LauncherMenu>();
}
