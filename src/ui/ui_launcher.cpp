#include "recomp_ui.h"
#include "recomp_config.h"
#include "recomp_game.h"
#include "../../ultramodern/ultramodern.hpp"
#include "RmlUi/Core.h"
#include "nfd.h"
#include <filesystem>

std::string version_number = "v1.1.0";

Rml::DataModelHandle model_handle;
bool mm_rom_valid = false;

void select_rom() {
	nfdnchar_t* native_path = nullptr;
	nfdresult_t result = NFD_OpenDialogN(&native_path, nullptr, 0, nullptr);

	if (result == NFD_OKAY) {
		std::filesystem::path path{native_path};

		NFD_FreePathN(native_path);
		native_path = nullptr;

		recomp::RomValidationError rom_error = recomp::select_rom(path, recomp::Game::MM);

		switch (rom_error) {
			case recomp::RomValidationError::Good:
				mm_rom_valid = true;
				model_handle.DirtyVariable("mm_rom_valid");
				break;
			case recomp::RomValidationError::FailedToOpen:
				recomp::message_box("Failed to open ROM file.");
				break;
			case recomp::RomValidationError::NotARom:
				recomp::message_box("This is not a valid ROM file.");
				break;
			case recomp::RomValidationError::IncorrectRom:
				recomp::message_box("This ROM is not the correct game.");
				break;
			case recomp::RomValidationError::NotYet:
				recomp::message_box("This game isn't supported yet.");
				break;
			case recomp::RomValidationError::IncorrectVersion:
				recomp::message_box("This ROM is the correct game, but the wrong version.\nThis project requires the NTSC-U N64 version of the game.");
				break;
			case recomp::RomValidationError::OtherError:
				recomp::message_box("An unknown error has occurred.");
				break;
		}
	}
}

class LauncherMenu : public recomp::MenuController {
public:
    LauncherMenu() {
		mm_rom_valid = recomp::is_rom_valid(recomp::Game::MM);
    }
	~LauncherMenu() override {

	}
	Rml::ElementDocument* load_document(Rml::Context* context) override {
        return context->LoadDocument("assets/launcher.rml");
	}
	void register_events(recomp::UiEventListenerInstancer& listener) override {
		recomp::register_event(listener, "select_rom",
			[](const std::string& param, Rml::Event& event) {
				select_rom();
			}
		);
		recomp::register_event(listener, "rom_selected",
			[](const std::string& param, Rml::Event& event) {
				mm_rom_valid = true;
				model_handle.DirtyVariable("mm_rom_valid");
			}
		);
		recomp::register_event(listener, "start_game",
			[](const std::string& param, Rml::Event& event) {
				recomp::start_game(recomp::Game::MM);
				recomp::set_current_menu(recomp::Menu::None);
			}
		);
		recomp::register_event(listener, "open_controls",
			[](const std::string& param, Rml::Event& event) {
				recomp::set_current_menu(recomp::Menu::Config);
				recomp::set_config_submenu(recomp::ConfigSubmenu::Controls);
			}
		);
		recomp::register_event(listener, "open_settings",
			[](const std::string& param, Rml::Event& event) {
				recomp::set_current_menu(recomp::Menu::Config);
				recomp::set_config_submenu(recomp::ConfigSubmenu::General);
			}
		);
		recomp::register_event(listener, "exit_game",
			[](const std::string& param, Rml::Event& event) {
				ultramodern::quit();
			}
		);
	}
	void make_bindings(Rml::Context* context) override {
		Rml::DataModelConstructor constructor = context->CreateDataModel("launcher_model");

		constructor.Bind("mm_rom_valid", &mm_rom_valid);
		constructor.Bind("version_number", &version_number);

		model_handle = constructor.GetModelHandle();
	}
};

std::unique_ptr<recomp::MenuController> recomp::create_launcher_menu() {
    return std::make_unique<LauncherMenu>();
}
