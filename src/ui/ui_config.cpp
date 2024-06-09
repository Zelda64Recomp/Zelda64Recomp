#include "recomp_ui.h"
#include "recomp_input.h"
#include "zelda_sound.h"
#include "zelda_config.h"
#include "zelda_debug.h"
#include "zelda_render.h"
#include "promptfont.h"
#include "ultramodern/config.hpp"
#include "ultramodern/ultramodern.hpp"
#include "RmlUi/Core.h"

ultramodern::renderer::GraphicsConfig new_options;
Rml::DataModelHandle nav_help_model_handle;
Rml::DataModelHandle general_model_handle;
Rml::DataModelHandle controls_model_handle;
Rml::DataModelHandle graphics_model_handle;
Rml::DataModelHandle sound_options_model_handle;

recompui::PromptContext prompt_context;

namespace recompui {
	const std::unordered_map<ButtonVariant, std::string> button_variants {
		{ButtonVariant::Primary, "primary"},
		{ButtonVariant::Secondary, "secondary"},
		{ButtonVariant::Tertiary, "tertiary"},
		{ButtonVariant::Success, "success"},
		{ButtonVariant::Error, "error"},
		{ButtonVariant::Warning, "warning"}
	};

	void PromptContext::close_prompt() {
		open = false;
		model_handle.DirtyVariable("prompt__open");
	}

	void PromptContext::open_prompt(
		const std::string& headerText,
		const std::string& contentText,
		const std::string& confirmLabelText,
		const std::string& cancelLabelText,
		std::function<void()> confirmCb,
		std::function<void()> cancelCb,
		ButtonVariant _confirmVariant,
		ButtonVariant _cancelVariant,
		bool _focusOnCancel,
		const std::string& _returnElementId
	) {
		open = true;
		header = headerText;
		content = contentText;
		confirmLabel = confirmLabelText;
		cancelLabel = cancelLabelText;
		onConfirm = confirmCb;
		onCancel = cancelCb;
		confirmVariant = _confirmVariant;
		cancelVariant = _cancelVariant;
		focusOnCancel = _focusOnCancel;
		returnElementId = _returnElementId;

		model_handle.DirtyVariable("prompt__open");
		model_handle.DirtyVariable("prompt__header");
		model_handle.DirtyVariable("prompt__content");
		model_handle.DirtyVariable("prompt__confirmLabel");
		model_handle.DirtyVariable("prompt__cancelLabel");
		shouldFocus = true;
	}

	void PromptContext::on_confirm(void) {
		onConfirm();
		open = false;
		model_handle.DirtyVariable("prompt__open");
	}

	void PromptContext::on_cancel(void) {
		onCancel();
		open = false;
		model_handle.DirtyVariable("prompt__open");
	}

	void PromptContext::on_click(Rml::DataModelHandle model_handle, Rml::Event& event, const Rml::VariantList& inputs) {
		Rml::Element *target = event.GetTargetElement();
		auto id = target->GetId();
		if (id == "prompt__confirm-button" || id == "prompt__confirm-button-label") {
			on_confirm();
			event.StopPropagation();
		} else if (id == "prompt__cancel-button" || id == "prompt__cancel-button-label") {
			on_cancel();
			event.StopPropagation();
		}
		if (event.GetCurrentElement()->GetId() == "prompt-root") {
			event.StopPropagation();
		}
	}

	PromptContext *get_prompt_context() {
		return &prompt_context;
	}
};

// True if controller config menu is open, false if keyboard config menu is open, undefined otherwise
bool configuring_controller = false;

template <typename T>
void get_option(const T& input, Rml::Variant& output) {
	std::string value = "";
	to_json(value, input);

	if (value.empty()) {
		throw std::runtime_error("Invalid value :" + std::to_string(int(input)));
	}

	output = value;
}

template <typename T>
void set_option(T& output, const Rml::Variant& input) {
	T value = T::OptionCount;
	from_json(input.Get<std::string>(), value);

	if (value == T::OptionCount) {
		throw std::runtime_error("Invalid value :" + input.Get<std::string>());
	}

	output = value;
}

template <typename T>
void bind_option(Rml::DataModelConstructor& constructor, const std::string& name, T* option) {
	constructor.BindFunc(name,
		[option](Rml::Variant& out) { get_option(*option, out); },
		[option](const Rml::Variant& in) {
			set_option(*option, in);
			graphics_model_handle.DirtyVariable("options_changed");
			graphics_model_handle.DirtyVariable("ds_info");
		}
	);
};

template <typename T>
void bind_atomic(Rml::DataModelConstructor& constructor, Rml::DataModelHandle handle, const char* name, std::atomic<T>* atomic_val) {
	constructor.BindFunc(name,
		[atomic_val](Rml::Variant& out) {
			out = atomic_val->load();
		},
		[atomic_val, handle, name](const Rml::Variant& in) mutable {
			atomic_val->store(in.Get<T>());
			handle.DirtyVariable(name);
		}
	);
}

static int scanned_binding_index = -1;
static int scanned_input_index = -1;
static int focused_input_index = -1;
static int focused_config_option_index = -1;

static bool msaa2x_supported = false;
static bool msaa4x_supported = false;
static bool msaa8x_supported = false;
static bool sample_positions_supported = false;

static bool cont_active = true;

static recomp::InputDevice cur_device = recomp::InputDevice::Controller;

int recomp::get_scanned_input_index() {
	return scanned_input_index;
}

void recomp::finish_scanning_input(recomp::InputField scanned_field) {
    recomp::set_input_binding(static_cast<recomp::GameInput>(scanned_input_index), scanned_binding_index, cur_device, scanned_field);
	scanned_input_index = -1;
	scanned_binding_index = -1;
	controls_model_handle.DirtyVariable("inputs");
	controls_model_handle.DirtyVariable("active_binding_input");
	controls_model_handle.DirtyVariable("active_binding_slot");
	nav_help_model_handle.DirtyVariable("nav_help__accept");
	nav_help_model_handle.DirtyVariable("nav_help__exit");
	graphics_model_handle.DirtyVariable("gfx_help__apply");
}

void recomp::cancel_scanning_input() {
    recomp::stop_scanning_input();
	scanned_input_index = -1;
	scanned_binding_index = -1;
	controls_model_handle.DirtyVariable("inputs");
	controls_model_handle.DirtyVariable("active_binding_input");
	controls_model_handle.DirtyVariable("active_binding_slot");
	nav_help_model_handle.DirtyVariable("nav_help__accept");
	nav_help_model_handle.DirtyVariable("nav_help__exit");
	graphics_model_handle.DirtyVariable("gfx_help__apply");
}

void recomp::config_menu_set_cont_or_kb(bool cont_interacted) {
	if (cont_active != cont_interacted) {
		cont_active = cont_interacted;

		if (nav_help_model_handle) {
			nav_help_model_handle.DirtyVariable("nav_help__navigate");
			nav_help_model_handle.DirtyVariable("nav_help__accept");
			nav_help_model_handle.DirtyVariable("nav_help__exit");
		}

		if (graphics_model_handle) {
			graphics_model_handle.DirtyVariable("gfx_help__apply");
		}
	}
}

void close_config_menu_impl() {
    zelda64::save_config();

	if (ultramodern::is_game_started()) {
        recompui::set_current_menu(recompui::Menu::None);
	}
	else {
        recompui::set_current_menu(recompui::Menu::Launcher);
	}
}

// TODO: Remove once RT64 gets native fullscreen support on Linux
#if defined(__linux__)
extern SDL_Window* window;
#endif

void apply_graphics_config(void) {
	ultramodern::renderer::set_graphics_config(new_options);
#if defined(__linux__) // TODO: Remove once RT64 gets native fullscreen support on Linux
	if (new_options.wm_option == ultramodern::renderer::WindowMode::Fullscreen) {
		SDL_SetWindowFullscreen(window,SDL_WINDOW_FULLSCREEN_DESKTOP);
	} else {
		SDL_SetWindowFullscreen(window,0);
	}
#endif
}

void close_config_menu() {
	if (ultramodern::renderer::get_graphics_config() != new_options) {
		prompt_context.open_prompt(
			"Graphics options have changed",
			"Would you like to apply or discard the changes?",
			"Apply",
			"Discard",
			[]() {
				apply_graphics_config();
				graphics_model_handle.DirtyAllVariables();
				close_config_menu_impl();
			},
			[]() {
				new_options = ultramodern::renderer::get_graphics_config();
				graphics_model_handle.DirtyAllVariables();
				close_config_menu_impl();
			},
            recompui::ButtonVariant::Success,
            recompui::ButtonVariant::Error,
			true,
			"config__close-menu-button"
		);
		return;
	}

	close_config_menu_impl();
}

void zelda64::open_quit_game_prompt() {
	prompt_context.open_prompt(
		"Are you sure you want to quit?",
		"Any progress since your last save will be lost.",
		"Quit",
		"Cancel",
		[]() {
			ultramodern::quit();
		},
		[]() {},
        recompui::ButtonVariant::Error,
        recompui::ButtonVariant::Tertiary,
		true,
		"config__quit-game-button"
	);
}

// These defaults values don't matter, as the config file handling overrides them.
struct ControlOptionsContext {
	int rumble_strength; // 0 to 100
	int gyro_sensitivity; // 0 to 100
	int mouse_sensitivity; // 0 to 100
	int joystick_deadzone; // 0 to 100
    zelda64::TargetingMode targeting_mode;
	recomp::BackgroundInputMode background_input_mode;
	zelda64::AutosaveMode autosave_mode;
    zelda64::CameraInvertMode camera_invert_mode;
	zelda64::AnalogCamMode analog_cam_mode;
    zelda64::CameraInvertMode analog_camera_invert_mode;
};

ControlOptionsContext control_options_context;

int recomp::get_rumble_strength() {
	return control_options_context.rumble_strength;
}

void recomp::set_rumble_strength(int strength) {
	control_options_context.rumble_strength = strength;
	if (general_model_handle) {
		general_model_handle.DirtyVariable("rumble_strength");
	}
}

int recomp::get_gyro_sensitivity() {
	return control_options_context.gyro_sensitivity;
}

int recomp::get_mouse_sensitivity() {
	return control_options_context.mouse_sensitivity;
}

int recomp::get_joystick_deadzone() {
	return control_options_context.joystick_deadzone;
}

void recomp::set_gyro_sensitivity(int sensitivity) {
	control_options_context.gyro_sensitivity = sensitivity;
	if (general_model_handle) {
		general_model_handle.DirtyVariable("gyro_sensitivity");
	}
}

void recomp::set_mouse_sensitivity(int sensitivity) {
	control_options_context.mouse_sensitivity = sensitivity;
	if (general_model_handle) {
		general_model_handle.DirtyVariable("mouse_sensitivity");
	}
}

void recomp::set_joystick_deadzone(int deadzone) {
	control_options_context.joystick_deadzone = deadzone;
	if (general_model_handle) {
		general_model_handle.DirtyVariable("joystick_deadzone");
	}
}

zelda64::TargetingMode zelda64::get_targeting_mode() {
	return control_options_context.targeting_mode;
}

void zelda64::set_targeting_mode(zelda64::TargetingMode mode) {
	control_options_context.targeting_mode = mode;
	if (general_model_handle) {
		general_model_handle.DirtyVariable("targeting_mode");
	}
}

recomp::BackgroundInputMode recomp::get_background_input_mode() {
	return control_options_context.background_input_mode;
}

void recomp::set_background_input_mode(recomp::BackgroundInputMode mode) {
	control_options_context.background_input_mode = mode;
	if (general_model_handle) {
		general_model_handle.DirtyVariable("background_input_mode");
	}
	SDL_SetHint(
		SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,
		mode == recomp::BackgroundInputMode::On
			? "1"
			: "0"
	);
}

zelda64::AutosaveMode zelda64::get_autosave_mode() {
	return control_options_context.autosave_mode;
}

void zelda64::set_autosave_mode(zelda64::AutosaveMode mode) {
	control_options_context.autosave_mode = mode;
	if (general_model_handle) {
		general_model_handle.DirtyVariable("autosave_mode");
	}
}

zelda64::CameraInvertMode zelda64::get_camera_invert_mode() {
	return control_options_context.camera_invert_mode;
}

void zelda64::set_camera_invert_mode(zelda64::CameraInvertMode mode) {
	control_options_context.camera_invert_mode = mode;
	if (general_model_handle) {
		general_model_handle.DirtyVariable("camera_invert_mode");
	}
}

zelda64::AnalogCamMode zelda64::get_analog_cam_mode() {
	return control_options_context.analog_cam_mode;
}

void zelda64::set_analog_cam_mode(zelda64::AnalogCamMode mode) {
	control_options_context.analog_cam_mode = mode;
	if (general_model_handle) {
		general_model_handle.DirtyVariable("analog_cam_mode");
	}
}

zelda64::CameraInvertMode zelda64::get_analog_camera_invert_mode() {
	return control_options_context.analog_camera_invert_mode;
}

void zelda64::set_analog_camera_invert_mode(zelda64::CameraInvertMode mode) {
	control_options_context.analog_camera_invert_mode = mode;
	if (general_model_handle) {
		general_model_handle.DirtyVariable("analog_camera_invert_mode");
	}
}

struct SoundOptionsContext {
	std::atomic<int> main_volume; // Option to control the volume of all sound
	std::atomic<int> bgm_volume;
	std::atomic<int> low_health_beeps_enabled; // RmlUi doesn't seem to like "true"/"false" strings for setting variants so an int is used here instead.
	void reset() {
		bgm_volume = 100;
		main_volume = 100;
		low_health_beeps_enabled = (int)true;
	}
	SoundOptionsContext() {
		reset();
	}
};

SoundOptionsContext sound_options_context;

void zelda64::reset_sound_settings() {
	sound_options_context.reset();
	if (sound_options_model_handle) {
		sound_options_model_handle.DirtyAllVariables();
	}
}

void zelda64::set_main_volume(int volume) {
	sound_options_context.main_volume.store(volume);
	if (sound_options_model_handle) {
		sound_options_model_handle.DirtyVariable("main_volume");
	}
}

int zelda64::get_main_volume() {
	return sound_options_context.main_volume.load();
}

void zelda64::set_bgm_volume(int volume) {
    sound_options_context.bgm_volume.store(volume);
	if (sound_options_model_handle) {
		sound_options_model_handle.DirtyVariable("bgm_volume");
	}
}

int zelda64::get_bgm_volume() {
    return sound_options_context.bgm_volume.load();
}

void zelda64::set_low_health_beeps_enabled(bool enabled) {
    sound_options_context.low_health_beeps_enabled.store((int)enabled);
	if (sound_options_model_handle) {
		sound_options_model_handle.DirtyVariable("low_health_beeps_enabled");
	}
}

bool zelda64::get_low_health_beeps_enabled() {
    return (bool)sound_options_context.low_health_beeps_enabled.load();
}

struct DebugContext {
	Rml::DataModelHandle model_handle;
	std::vector<std::string> area_names;
	std::vector<std::string> scene_names;
	std::vector<std::string> entrance_names; 
	int area_index = 0;
	int scene_index = 0;
	int entrance_index = 0;
	int set_time_day = 1;
	int set_time_hour = 12;
	int set_time_minute = 0;
	bool debug_enabled = false;

	DebugContext() {
		for (const auto& area : zelda64::game_warps) {
			area_names.emplace_back(area.name);
		}
		update_warp_names();
	}

	void update_warp_names() {
		scene_names.clear();
		for (const auto& scene : zelda64::game_warps[area_index].scenes) {
			scene_names.emplace_back(scene.name);
		}
		
		entrance_names = zelda64::game_warps[area_index].scenes[scene_index].entrances;
	}
};

void recompui::update_rml_display_refresh_rate() {
	static uint32_t lastRate = 0;
	if (!graphics_model_handle) return;

	uint32_t curRate = ultramodern::get_display_refresh_rate();
	if (curRate != lastRate) {
		graphics_model_handle.DirtyVariable("display_refresh_rate");
	}
	lastRate = curRate;
}

DebugContext debug_context;

class ConfigMenu : public recompui::MenuController {
public:
	ConfigMenu() {

	}
	~ConfigMenu() override {

	}
	Rml::ElementDocument* load_document(Rml::Context* context) override {
        return context->LoadDocument("assets/config_menu.rml");
	}
	void register_events(recompui::UiEventListenerInstancer& listener) override {
		recompui::register_event(listener, "apply_options",
			[](const std::string& param, Rml::Event& event) {
				graphics_model_handle.DirtyVariable("options_changed");
				apply_graphics_config();
			});
		recompui::register_event(listener, "config_keydown",
			[](const std::string& param, Rml::Event& event) {
				if (!prompt_context.open && event.GetId() == Rml::EventId::Keydown) {
					auto key = event.GetParameter<Rml::Input::KeyIdentifier>("key_identifier", Rml::Input::KeyIdentifier::KI_UNKNOWN);
					switch (key) {
						case Rml::Input::KeyIdentifier::KI_ESCAPE:
							close_config_menu();
							break;
						case Rml::Input::KeyIdentifier::KI_F:
							graphics_model_handle.DirtyVariable("options_changed");
							apply_graphics_config();
							break;
					}
				}
			});
		// This needs to be separate from `close_config_menu` so it ensures that the event is only on the target
		recompui::register_event(listener, "close_config_menu_backdrop",
			[](const std::string& param, Rml::Event& event) {
				if (event.GetPhase() == Rml::EventPhase::Target) {
					close_config_menu();
				}
			});
		recompui::register_event(listener, "close_config_menu",
			[](const std::string& param, Rml::Event& event) {
				close_config_menu();
			});

		recompui::register_event(listener, "open_quit_game_prompt",
			[](const std::string& param, Rml::Event& event) {
                zelda64::open_quit_game_prompt();
			});

		recompui::register_event(listener, "toggle_input_device",
			[](const std::string& param, Rml::Event& event) {
				cur_device = cur_device == recomp::InputDevice::Controller
					? recomp::InputDevice::Keyboard
					: recomp::InputDevice::Controller;
				controls_model_handle.DirtyVariable("input_device_is_keyboard");
				controls_model_handle.DirtyVariable("inputs");
			});
			
		recompui::register_event(listener, "area_index_changed",
			[](const std::string& param, Rml::Event& event) {
				debug_context.area_index = event.GetParameter<int>("value", 0);
				debug_context.scene_index = 0;
				debug_context.entrance_index = 0;
				debug_context.update_warp_names();
				debug_context.model_handle.DirtyVariable("scene_index");
				debug_context.model_handle.DirtyVariable("entrance_index");
				debug_context.model_handle.DirtyVariable("scene_names");
				debug_context.model_handle.DirtyVariable("entrance_names");
			});
			
		recompui::register_event(listener, "scene_index_changed",
			[](const std::string& param, Rml::Event& event) {
				debug_context.scene_index = event.GetParameter<int>("value", 0);
				debug_context.entrance_index = 0;
				debug_context.update_warp_names();
				debug_context.model_handle.DirtyVariable("entrance_index");
				debug_context.model_handle.DirtyVariable("entrance_names");
			});

		recompui::register_event(listener, "do_warp",
			[](const std::string& param, Rml::Event& event) {
                zelda64::do_warp(debug_context.area_index, debug_context.scene_index, debug_context.entrance_index);
			});

		recompui::register_event(listener, "set_time",
			[](const std::string& param, Rml::Event& event) {
                zelda64::set_time(debug_context.set_time_day, debug_context.set_time_hour, debug_context.set_time_minute);
			});
	}

	void bind_config_list_events(Rml::DataModelConstructor &constructor) {
		constructor.BindEventCallback("set_cur_config_index",
			[](Rml::DataModelHandle model_handle, Rml::Event& event, const Rml::VariantList& inputs) {
				int option_index = inputs.at(0).Get<size_t>();
				// watch for mouseout being overzealous during event bubbling, only clear if the event's attached element matches the current
				if (option_index == -1 && event.GetType() == "mouseout" && event.GetCurrentElement() != event.GetTargetElement()) {
					return;
				}
				focused_config_option_index = option_index;
				model_handle.DirtyVariable("cur_config_index");
			});

		constructor.Bind("cur_config_index", &focused_config_option_index);
	}

	void make_graphics_bindings(Rml::Context* context) {
		Rml::DataModelConstructor constructor = context->CreateDataModel("graphics_model");
		if (!constructor) {
			throw std::runtime_error("Failed to make RmlUi data model for the graphics config menu");
		}

		ultramodern::sleep_milliseconds(50);
		new_options = ultramodern::renderer::get_graphics_config();
		bind_config_list_events(constructor);

		constructor.BindFunc("res_option",
			[](Rml::Variant& out) { get_option(new_options.res_option, out); },
			[](const Rml::Variant& in) {
				set_option(new_options.res_option, in);
				graphics_model_handle.DirtyVariable("options_changed");
				graphics_model_handle.DirtyVariable("ds_info");
				graphics_model_handle.DirtyVariable("ds_option");
			}
		);
		bind_option(constructor, "wm_option", &new_options.wm_option);
		bind_option(constructor, "ar_option", &new_options.ar_option);
		bind_option(constructor, "hr_option", &new_options.hr_option);
		bind_option(constructor, "msaa_option", &new_options.msaa_option);
		bind_option(constructor, "rr_option", &new_options.rr_option);
		constructor.BindFunc("rr_manual_value",
			[](Rml::Variant& out) {
				out = new_options.rr_manual_value;
			},
			[](const Rml::Variant& in) {
				new_options.rr_manual_value = in.Get<int>();
				graphics_model_handle.DirtyVariable("options_changed");
			});
		constructor.BindFunc("ds_option",
			[](Rml::Variant& out) {
				if (new_options.res_option == ultramodern::renderer::Resolution::Auto) {
					out = 1;
				} else {
					out = new_options.ds_option;
				}
			},
			[](const Rml::Variant& in) {
				new_options.ds_option = in.Get<int>();
				graphics_model_handle.DirtyVariable("options_changed");
				graphics_model_handle.DirtyVariable("ds_info");
			});

		constructor.BindFunc("display_refresh_rate",
			[](Rml::Variant& out) {
				out = ultramodern::get_display_refresh_rate();
			});

		constructor.BindFunc("options_changed",
			[](Rml::Variant& out) {
				out = (ultramodern::renderer::get_graphics_config() != new_options);
			});
		constructor.BindFunc("ds_info",
			[](Rml::Variant& out) {
				switch (new_options.res_option) {
					default:
					case ultramodern::renderer::Resolution::Auto:
						out = "Downsampling is not available at auto resolution";
						return;
					case ultramodern::renderer::Resolution::Original:
						if (new_options.ds_option == 2) {
							out = "Rendered in 480p and scaled to 240p";
						} else if (new_options.ds_option == 4) {
							out = "Rendered in 960p and scaled to 240p";
						}
						return;
					case ultramodern::renderer::Resolution::Original2x:
						if (new_options.ds_option == 2) {
							out = "Rendered in 960p and scaled to 480p";
						} else if (new_options.ds_option == 4) {
							out = "Rendered in 4K and scaled to 480p";
						}
						return;
				}
				out = "";
			});
		
		constructor.BindFunc("gfx_help__apply", [](Rml::Variant& out) {
			if (cont_active) {
				out = \
					(recomp::get_input_binding(recomp::GameInput::APPLY_MENU, 0, recomp::InputDevice::Controller).to_string() != "" ?
						" " + recomp::get_input_binding(recomp::GameInput::APPLY_MENU, 0, recomp::InputDevice::Controller).to_string() :
						""
					) + \
					(recomp::get_input_binding(recomp::GameInput::APPLY_MENU, 1, recomp::InputDevice::Controller).to_string() != "" ?
						" " + recomp::get_input_binding(recomp::GameInput::APPLY_MENU, 1, recomp::InputDevice::Controller).to_string() :
						""
					);
			} else {
				out = " " PF_KEYBOARD_F;
			}
		});

		constructor.Bind("msaa2x_supported", &msaa2x_supported);
		constructor.Bind("msaa4x_supported", &msaa4x_supported);
		constructor.Bind("msaa8x_supported", &msaa8x_supported);
		constructor.Bind("sample_positions_supported", &sample_positions_supported);

		graphics_model_handle = constructor.GetModelHandle();
	}

	void make_controls_bindings(Rml::Context* context) {
		Rml::DataModelConstructor constructor = context->CreateDataModel("controls_model");
		if (!constructor) {
			throw std::runtime_error("Failed to make RmlUi data model for the controls config menu");
		}

		constructor.BindFunc("input_count", [](Rml::Variant& out) { out = recomp::get_num_inputs(); } );
		constructor.BindFunc("input_device_is_keyboard", [](Rml::Variant& out) { out = cur_device == recomp::InputDevice::Keyboard; } );

		constructor.RegisterTransformFunc("get_input_name", [](const Rml::VariantList& inputs) {
			return Rml::Variant{recomp::get_input_name(static_cast<recomp::GameInput>(inputs.at(0).Get<size_t>()))};
		});

		constructor.RegisterTransformFunc("get_input_enum_name", [](const Rml::VariantList& inputs) {
			return Rml::Variant{recomp::get_input_enum_name(static_cast<recomp::GameInput>(inputs.at(0).Get<size_t>()))};
		});

		constructor.BindEventCallback("set_input_binding",
			[](Rml::DataModelHandle model_handle, Rml::Event& event, const Rml::VariantList& inputs) {
				scanned_input_index = inputs.at(0).Get<size_t>();
				scanned_binding_index = inputs.at(1).Get<size_t>();
                recomp::start_scanning_input(cur_device);
				model_handle.DirtyVariable("active_binding_input");
				model_handle.DirtyVariable("active_binding_slot");
			});

		constructor.BindEventCallback("reset_input_bindings_to_defaults",
			[](Rml::DataModelHandle model_handle, Rml::Event& event, const Rml::VariantList& inputs) {
				if (cur_device == recomp::InputDevice::Controller) {
                    zelda64::reset_cont_input_bindings();
				} else {
                    zelda64::reset_kb_input_bindings();
				}
				model_handle.DirtyAllVariables();
				nav_help_model_handle.DirtyVariable("nav_help__accept");
				nav_help_model_handle.DirtyVariable("nav_help__exit");
				graphics_model_handle.DirtyVariable("gfx_help__apply");
			});

		constructor.BindEventCallback("clear_input_bindings",
			[](Rml::DataModelHandle model_handle, Rml::Event& event, const Rml::VariantList& inputs) {
                recomp::GameInput input = static_cast<recomp::GameInput>(inputs.at(0).Get<size_t>());
				for (size_t binding_index = 0; binding_index < recomp::bindings_per_input; binding_index++) {
                    recomp::set_input_binding(input, binding_index, cur_device, recomp::InputField{});
				}
				model_handle.DirtyVariable("inputs");
				graphics_model_handle.DirtyVariable("gfx_help__apply");
			});

		constructor.BindEventCallback("reset_single_input_binding_to_default",
			[](Rml::DataModelHandle model_handle, Rml::Event& event, const Rml::VariantList& inputs) {
                recomp::GameInput input = static_cast<recomp::GameInput>(inputs.at(0).Get<size_t>());
				zelda64::reset_single_input_binding(cur_device, input);
				model_handle.DirtyVariable("inputs");
				nav_help_model_handle.DirtyVariable("nav_help__accept");
				nav_help_model_handle.DirtyVariable("nav_help__exit");
			});

		constructor.BindEventCallback("set_input_row_focus",
			[](Rml::DataModelHandle model_handle, Rml::Event& event, const Rml::VariantList& inputs) {
				int input_index = inputs.at(0).Get<size_t>();
				// watch for mouseout being overzealous during event bubbling, only clear if the event's attached element matches the current
				if (input_index == -1 && event.GetType() == "mouseout" && event.GetCurrentElement() != event.GetTargetElement()) {
					return;
				}
				focused_input_index = input_index;
				model_handle.DirtyVariable("cur_input_row");
			});

		// Rml variable definition for an individual InputField.
		struct InputFieldVariableDefinition : public Rml::VariableDefinition {
			InputFieldVariableDefinition() : Rml::VariableDefinition(Rml::DataVariableType::Scalar) {}

			virtual bool Get(void* ptr, Rml::Variant& variant) override { variant = reinterpret_cast<recomp::InputField*>(ptr)->to_string(); return true; }
			virtual bool Set(void* ptr, const Rml::Variant& variant) override { return false; }
		};
		// Static instance of the InputField variable definition to have a pointer to return to RmlUi.
		static InputFieldVariableDefinition input_field_definition_instance{};

		// Rml variable definition for an array of InputField values (e.g. all the bindings for a single input).
		struct BindingContainerVariableDefinition : public Rml::VariableDefinition {
			BindingContainerVariableDefinition() : Rml::VariableDefinition(Rml::DataVariableType::Array) {}

			virtual bool Get(void* ptr, Rml::Variant& variant) override { return false; }
			virtual bool Set(void* ptr, const Rml::Variant& variant) override { return false; }

			virtual int Size(void* ptr) override { return recomp::bindings_per_input; }
			virtual Rml::DataVariable Child(void* ptr, const Rml::DataAddressEntry& address) override {
                recomp::GameInput input = static_cast<recomp::GameInput>((uintptr_t)ptr);
				return Rml::DataVariable{&input_field_definition_instance, &recomp::get_input_binding(input, address.index, cur_device)};
			}
		};
		// Static instance of the InputField array variable definition to have a fixed pointer to return to RmlUi.
		static BindingContainerVariableDefinition binding_container_var_instance{};

		// Rml variable definition for an array of an array of InputField values (e.g. all the bindings for all inputs).
		struct BindingArrayContainerVariableDefinition : public Rml::VariableDefinition {
			BindingArrayContainerVariableDefinition() : Rml::VariableDefinition(Rml::DataVariableType::Array) {}

			virtual bool Get(void* ptr, Rml::Variant& variant) override { return false; }
			virtual bool Set(void* ptr, const Rml::Variant& variant) override { return false; }

			virtual int Size(void* ptr) override { return recomp::get_num_inputs(); }
			virtual Rml::DataVariable Child(void* ptr, const Rml::DataAddressEntry& address) override {
				// Encode the input index as the pointer to avoid needing to do any allocations.
				return Rml::DataVariable(&binding_container_var_instance, (void*)(uintptr_t)address.index);
			}
		};

		// Static instance of the BindingArrayContainerVariableDefinition variable definition to have a fixed pointer to return to RmlUi.
		static BindingArrayContainerVariableDefinition binding_array_var_instance{};

		struct InputContainerVariableDefinition : public Rml::VariableDefinition {
			InputContainerVariableDefinition() : Rml::VariableDefinition(Rml::DataVariableType::Struct) {}

			virtual bool Get(void* ptr, Rml::Variant& variant) override { return true; }
			virtual bool Set(void* ptr, const Rml::Variant& variant) override { return false; }

			virtual int Size(void* ptr) override { return recomp::get_num_inputs(); }
			virtual Rml::DataVariable Child(void* ptr, const Rml::DataAddressEntry& address) override {
				if (address.name == "array") {
					return Rml::DataVariable(&binding_array_var_instance, nullptr);
				}
				else {
                    recomp::GameInput input = recomp::get_input_from_enum_name(address.name);
					if (input != recomp::GameInput::COUNT) {
						return Rml::DataVariable(&binding_container_var_instance, (void*)(uintptr_t)input);
					}
				}
				return Rml::DataVariable{};
			}
		};

		// Dummy type to associate with the variable definition.
		struct InputContainer {};
		constructor.RegisterCustomDataVariableDefinition<InputContainer>(Rml::MakeUnique<InputContainerVariableDefinition>());

		// Dummy instance of the dummy type to bind to the variable.
		static InputContainer dummy_container;
		constructor.Bind("inputs", &dummy_container);

		constructor.BindFunc("cur_input_row", [](Rml::Variant& out) {
			if (focused_input_index == -1) {
				out = "NONE";
			}
			else {
				out = recomp::get_input_enum_name(static_cast<recomp::GameInput>(focused_input_index));
			}
		});

		constructor.BindFunc("active_binding_input", [](Rml::Variant& out) {
			if (scanned_input_index == -1) {
				out = "NONE";
			}
			else {
				out = recomp::get_input_enum_name(static_cast<recomp::GameInput>(scanned_input_index));
			}
		});

		constructor.Bind<int>("active_binding_slot", &scanned_binding_index);

		controls_model_handle = constructor.GetModelHandle();
	}

	void make_nav_help_bindings(Rml::Context* context) {
		Rml::DataModelConstructor constructor = context->CreateDataModel("nav_help_model");
		if (!constructor) {
			throw std::runtime_error("Failed to make RmlUi data model for nav help");
		}

		constructor.BindFunc("nav_help__navigate", [](Rml::Variant& out) {
			if (cont_active) {
				out = PF_DPAD;
			} else {
				out = PF_KEYBOARD_ARROWS PF_KEYBOARD_TAB;
			}
		});

		constructor.BindFunc("nav_help__accept", [](Rml::Variant& out) {
			if (cont_active) {
				out = \
					recomp::get_input_binding(recomp::GameInput::ACCEPT_MENU, 0, recomp::InputDevice::Controller).to_string() + \
					recomp::get_input_binding(recomp::GameInput::ACCEPT_MENU, 1, recomp::InputDevice::Controller).to_string();
			} else {
				out = PF_KEYBOARD_ENTER;
			}
		});

		constructor.BindFunc("nav_help__exit", [](Rml::Variant& out) {
			if (cont_active) {
				out = \
					recomp::get_input_binding(recomp::GameInput::TOGGLE_MENU, 0, recomp::InputDevice::Controller).to_string() + \
					recomp::get_input_binding(recomp::GameInput::TOGGLE_MENU, 1, recomp::InputDevice::Controller).to_string();
			} else {
				out = PF_KEYBOARD_ESCAPE;
			}
		});

		nav_help_model_handle = constructor.GetModelHandle();
	}

	void make_general_bindings(Rml::Context* context) {
		Rml::DataModelConstructor constructor = context->CreateDataModel("general_model");
		if (!constructor) {
			throw std::runtime_error("Failed to make RmlUi data model for the control options menu");
		}

		bind_config_list_events(constructor);
		
		constructor.Bind("rumble_strength", &control_options_context.rumble_strength);
		constructor.Bind("gyro_sensitivity", &control_options_context.gyro_sensitivity);
		constructor.Bind("mouse_sensitivity", &control_options_context.mouse_sensitivity);
		constructor.Bind("joystick_deadzone", &control_options_context.joystick_deadzone);
		bind_option(constructor, "targeting_mode", &control_options_context.targeting_mode);
		bind_option(constructor, "background_input_mode", &control_options_context.background_input_mode);
		bind_option(constructor, "autosave_mode", &control_options_context.autosave_mode);
		bind_option(constructor, "camera_invert_mode", &control_options_context.camera_invert_mode);
		bind_option(constructor, "analog_cam_mode", &control_options_context.analog_cam_mode);
		bind_option(constructor, "analog_camera_invert_mode", &control_options_context.analog_camera_invert_mode);

		general_model_handle = constructor.GetModelHandle();
	}
	
	void make_sound_options_bindings(Rml::Context* context) {
		Rml::DataModelConstructor constructor = context->CreateDataModel("sound_options_model");
		if (!constructor) {
			throw std::runtime_error("Failed to make RmlUi data model for the sound options menu");
		}

		bind_config_list_events(constructor);
		
		sound_options_model_handle = constructor.GetModelHandle();

		bind_atomic(constructor, sound_options_model_handle, "main_volume", &sound_options_context.main_volume);
		bind_atomic(constructor, sound_options_model_handle, "bgm_volume", &sound_options_context.bgm_volume);
		bind_atomic(constructor, sound_options_model_handle, "low_health_beeps_enabled", &sound_options_context.low_health_beeps_enabled);
	}

	void make_debug_bindings(Rml::Context* context) {
		Rml::DataModelConstructor constructor = context->CreateDataModel("debug_model");
		if (!constructor) {
			throw std::runtime_error("Failed to make RmlUi data model for the debug menu");
		}

		bind_config_list_events(constructor);

		// Bind the debug mode enabled flag.
		constructor.Bind("debug_enabled", &debug_context.debug_enabled);
		
		// Register the array type for string vectors.
		constructor.RegisterArray<std::vector<std::string>>();
		
		// Bind the warp parameter indices
		constructor.Bind("area_index", &debug_context.area_index);
		constructor.Bind("scene_index", &debug_context.scene_index);
		constructor.Bind("entrance_index", &debug_context.entrance_index);

		// Bind the vectors for warp names
		constructor.Bind("area_names", &debug_context.area_names);
		constructor.Bind("scene_names", &debug_context.scene_names);
		constructor.Bind("entrance_names", &debug_context.entrance_names);

		constructor.Bind("debug_time_day", &debug_context.set_time_day);
		constructor.Bind("debug_time_hour", &debug_context.set_time_hour);
		constructor.Bind("debug_time_minute", &debug_context.set_time_minute);

		debug_context.model_handle = constructor.GetModelHandle();
	}

	void make_prompt_bindings(Rml::Context* context) {
		Rml::DataModelConstructor constructor = context->CreateDataModel("prompt_model");
		if (!constructor) {
			throw std::runtime_error("Failed to make RmlUi data model for the prompt");
		}

		// Bind the debug mode enabled flag.
		constructor.Bind("prompt__open", &prompt_context.open);
		constructor.Bind("prompt__header", &prompt_context.header);
		constructor.Bind("prompt__content", &prompt_context.content);
		constructor.Bind("prompt__confirmLabel", &prompt_context.confirmLabel);
		constructor.Bind("prompt__cancelLabel", &prompt_context.cancelLabel);

		constructor.BindEventCallback("prompt__on_click", &recompui::PromptContext::on_click, &prompt_context);

		prompt_context.model_handle = constructor.GetModelHandle();
	}

	void make_bindings(Rml::Context* context) override {
		// initially set cont state for ui help
		recomp::config_menu_set_cont_or_kb(recompui::get_cont_active());
		make_nav_help_bindings(context);
		make_general_bindings(context);
		make_controls_bindings(context);
		make_graphics_bindings(context);
		make_sound_options_bindings(context);
		make_debug_bindings(context);
		make_prompt_bindings(context);
	}
};

std::unique_ptr<recompui::MenuController> recompui::create_config_menu() {
	return std::make_unique<ConfigMenu>();
}

bool zelda64::get_debug_mode_enabled() {
	return debug_context.debug_enabled;
}

void zelda64::set_debug_mode_enabled(bool enabled) {
	debug_context.debug_enabled = enabled;
	if (debug_context.model_handle) {
		debug_context.model_handle.DirtyVariable("debug_enabled");
	}
}

void recompui::update_supported_options() {
	msaa2x_supported = zelda64::renderer::RT64MaxMSAA() >= RT64::UserConfiguration::Antialiasing::MSAA2X;
	msaa4x_supported = zelda64::renderer::RT64MaxMSAA() >= RT64::UserConfiguration::Antialiasing::MSAA4X;
	msaa8x_supported = zelda64::renderer::RT64MaxMSAA() >= RT64::UserConfiguration::Antialiasing::MSAA8X;
	sample_positions_supported = zelda64::renderer::RT64SamplePositionsSupported();
	
	new_options = ultramodern::renderer::get_graphics_config();

	graphics_model_handle.DirtyAllVariables();
}

void recompui::toggle_fullscreen() {
	new_options.wm_option = (new_options.wm_option == ultramodern::renderer::WindowMode::Windowed) ? ultramodern::renderer::WindowMode::Fullscreen : ultramodern::renderer::WindowMode::Windowed;
	apply_graphics_config();
	graphics_model_handle.DirtyVariable("wm_option");
}
