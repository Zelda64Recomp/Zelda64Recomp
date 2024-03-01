#include "recomp_ui.h"
#include "recomp_input.h"
#include "recomp_config.h"
#include "recomp_debug.h"
#include "../../ultramodern/config.hpp"
#include "../../ultramodern/ultramodern.hpp"
#include "RmlUi/Core.h"

ultramodern::GraphicsConfig new_options;
Rml::DataModelHandle graphics_model_handle;
Rml::DataModelHandle controls_model_handle;
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
		[option](const Rml::Variant& in) { set_option(*option, in); graphics_model_handle.DirtyVariable("options_changed"); }
	);
};

static int scanned_binding_index = -1;
static int scanned_input_index = -1;
static int focused_input_index = -1;

static recomp::InputDevice cur_device = recomp::InputDevice::Controller;

void recomp::finish_scanning_input(recomp::InputField scanned_field) {
	recomp::set_input_binding(static_cast<recomp::GameInput>(scanned_input_index), scanned_binding_index, cur_device, scanned_field);
	scanned_input_index = -1;
	scanned_binding_index = -1;
	controls_model_handle.DirtyVariable("inputs");
	controls_model_handle.DirtyVariable("active_binding_input");
	controls_model_handle.DirtyVariable("active_binding_slot");
}

void close_config_menu() {
	recomp::save_config();

	if (ultramodern::is_game_started()) {
		recomp::set_current_menu(recomp::Menu::None);
	}
	else {
		recomp::set_current_menu(recomp::Menu::Launcher);
	}
}

struct DebugContext {
	Rml::DataModelHandle model_handle;
	std::vector<std::string> area_names;
	std::vector<std::string> scene_names;
	std::vector<std::string> entrance_names; 
	int area_index = 0;
	int scene_index = 0;
	int entrance_index = 0;

	DebugContext() {
		for (const auto& area : recomp::game_warps) {
			area_names.emplace_back(area.name);
		}
		update_warp_names();
	}

	void update_warp_names() {
		scene_names.clear();
		for (const auto& scene : recomp::game_warps[area_index].scenes) {
			scene_names.emplace_back(scene.name);
		}
		
		entrance_names = recomp::game_warps[area_index].scenes[scene_index].entrances;
	}
};

DebugContext debug_context;

class ConfigMenu : public recomp::MenuController {
public:
	ConfigMenu() {

	}
	~ConfigMenu() override {

	}
	Rml::ElementDocument* load_document(Rml::Context* context) override {
        return context->LoadDocument("assets/config_menu.rml");
	}
	void register_events(recomp::UiEventListenerInstancer& listener) override {
		recomp::register_event(listener, "apply_options",
			[](const std::string& param, Rml::Event& event) {
				graphics_model_handle.DirtyVariable("options_changed");
				ultramodern::set_graphics_config(new_options);
			});
		recomp::register_event(listener, "config_keydown",
			[](const std::string& param, Rml::Event& event) {
				if (event.GetId() == Rml::EventId::Keydown) {
					if (event.GetParameter<Rml::Input::KeyIdentifier>("key_identifier", Rml::Input::KeyIdentifier::KI_UNKNOWN) == Rml::Input::KeyIdentifier::KI_ESCAPE) {
						close_config_menu();
					}
				}
			});
		// This needs to be separate from `close_config_menu` so it ensures that the event is only on the target
		recomp::register_event(listener, "close_config_menu_backdrop",
			[](const std::string& param, Rml::Event& event) {
				if (event.GetPhase() == Rml::EventPhase::Target) {
					close_config_menu();
				}
			});
		recomp::register_event(listener, "close_config_menu",
			[](const std::string& param, Rml::Event& event) {
				close_config_menu();
			});

		recomp::register_event(listener, "toggle_input_device",
			[](const std::string& param, Rml::Event& event) {
				cur_device = cur_device == recomp::InputDevice::Controller
					? recomp::InputDevice::Keyboard
					: recomp::InputDevice::Controller;
				controls_model_handle.DirtyVariable("input_device_is_keyboard");
				controls_model_handle.DirtyVariable("inputs");
			});
			
		recomp::register_event(listener, "area_index_changed",
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
			
		recomp::register_event(listener, "scene_index_changed",
			[](const std::string& param, Rml::Event& event) {
				debug_context.scene_index = event.GetParameter<int>("value", 0);
				debug_context.entrance_index = 0;
				debug_context.update_warp_names();
				debug_context.model_handle.DirtyVariable("entrance_index");
				debug_context.model_handle.DirtyVariable("entrance_names");
			});

		recomp::register_event(listener, "do_warp",
			[](const std::string& param, Rml::Event& event) {
				recomp::do_warp(debug_context.area_index, debug_context.scene_index, debug_context.entrance_index);
			});
	}
	void make_graphics_bindings(Rml::Context* context) {
		Rml::DataModelConstructor constructor = context->CreateDataModel("graphics_model");
		if (!constructor) {
			throw std::runtime_error("Failed to make RmlUi data model for the graphics config menu");
		}

		{
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(50ms);
		}
		new_options = ultramodern::get_graphics_config();

		bind_option(constructor, "res_option", &new_options.res_option);
		bind_option(constructor, "wm_option", &new_options.wm_option);
		bind_option(constructor, "ar_option", &new_options.ar_option);
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

		constructor.BindFunc("options_changed",
			[](Rml::Variant& out) {
				out = (ultramodern::get_graphics_config() != new_options);
			});

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

		constructor.BindEventCallback("clear_input_bindings",
			[](Rml::DataModelHandle model_handle, Rml::Event& event, const Rml::VariantList& inputs) {
				recomp::GameInput input = static_cast<recomp::GameInput>(inputs.at(0).Get<size_t>());
				for (size_t binding_index = 0; binding_index < recomp::bindings_per_input; binding_index++) {
					recomp::set_input_binding(input, binding_index, cur_device, recomp::InputField{});
				}
				model_handle.DirtyVariable("inputs");
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

	void make_debug_bindings(Rml::Context* context) {
		Rml::DataModelConstructor constructor = context->CreateDataModel("debug_model");
		if (!constructor) {
			throw std::runtime_error("Failed to make RmlUi data model for the debug menu");
		}
		
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

		debug_context.model_handle = constructor.GetModelHandle();
	}

	void make_bindings(Rml::Context* context) override {
		make_graphics_bindings(context);
		make_controls_bindings(context);
		make_debug_bindings(context);
	}
};

std::unique_ptr<recomp::MenuController> recomp::create_config_menu() {
	return std::make_unique<ConfigMenu>();
}
