#include "recomp_ui.h"
#include "recomp_input.h"
#include "../../ultramodern/config.hpp"
#include "../../ultramodern/ultramodern.hpp"
#include "RmlUi/Core.h"

ultramodern::GraphicsConfig cur_options;
ultramodern::GraphicsConfig new_options;
Rml::DataModelHandle graphics_model_handle;
Rml::DataModelHandle controls_model_handle;
// True if controller config menu is open, false if keyboard config menu is open, undefined otherwise
bool configuring_controller = false; 

NLOHMANN_JSON_SERIALIZE_ENUM(ultramodern::Resolution, {
	{ultramodern::Resolution::Original, "Original"},
	{ultramodern::Resolution::Original2x, "Original2x"},
	{ultramodern::Resolution::Auto, "Auto"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ultramodern::WindowMode, {
	{ultramodern::WindowMode::Windowed, "Windowed"},
	{ultramodern::WindowMode::Fullscreen, "Fullscreen"}
});

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

static size_t scanned_binding_index;
static size_t scanned_input_index;

constexpr recomp::InputDevice cur_device = recomp::InputDevice::Controller;

void recomp::finish_scanning_input(recomp::InputField scanned_field) {
	recomp::set_input_binding(scanned_input_index, scanned_binding_index, cur_device, scanned_field);
	controls_model_handle.DirtyVariable("inputs");
}

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
				cur_options = new_options;
				graphics_model_handle.DirtyVariable("options_changed");
				update_graphics_config(new_options);
			});
		recomp::register_event(listener, "rebind_input_bindings",
			[](const std::string& param, Rml::Event& event) {
			});
		recomp::register_event(listener, "clear_input_bindings",
			[](const std::string& param, Rml::Event& event) {
			});
		recomp::register_event(listener, "add_input_binding",
			[](const std::string& param, Rml::Event& event) {
			});
		recomp::register_event(listener, "config_keydown",
			[](const std::string& param, Rml::Event& event) {
				if (event.GetId() == Rml::EventId::Keydown) {
					if (event.GetParameter<Rml::Input::KeyIdentifier>("key_identifier", Rml::Input::KeyIdentifier::KI_UNKNOWN) == Rml::Input::KeyIdentifier::KI_ESCAPE) {
						if (ultramodern::is_game_started()) {
							recomp::set_current_menu(recomp::Menu::None);
						}
						else {
							recomp::set_current_menu(recomp::Menu::Launcher);
						}
					}
				}
			});
	}
	void make_graphics_bindings(Rml::Context* context) {
		Rml::DataModelConstructor constructor = context->CreateDataModel("graphics_model");
		if (!constructor) {
			throw std::runtime_error("Failed to make RmlUi data model for the graphics config menu");
		}

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
				out = (cur_options != new_options);
			});

		graphics_model_handle = constructor.GetModelHandle();
	}

	void make_controls_bindings(Rml::Context* context) {
		Rml::DataModelConstructor constructor = context->CreateDataModel("controls_model");
		if (!constructor) {
			throw std::runtime_error("Failed to make RmlUi data model for the controls config menu");
		}

		constructor.BindFunc("input_count", [](Rml::Variant& out) { out = recomp::get_num_inputs(); } );
		
		constructor.RegisterTransformFunc("get_input_name", [](const Rml::VariantList& inputs) {
			return Rml::Variant{recomp::get_input_names().at(inputs.at(0).Get<size_t>())};
		});

		constructor.BindEventCallback("set_input_binding",
			[](Rml::DataModelHandle model_handle, Rml::Event& event, const Rml::VariantList& inputs) {
				scanned_input_index = inputs.at(0).Get<size_t>();
				scanned_binding_index = inputs.at(1).Get<size_t>();
				recomp::start_scanning_input(cur_device);
			});

		constructor.BindEventCallback("clear_input_bindings",
			[](Rml::DataModelHandle model_handle, Rml::Event& event, const Rml::VariantList& inputs) {
				size_t input_index = inputs.at(0).Get<size_t>();
				for (size_t binding_index = 0; binding_index < recomp::bindings_per_input; binding_index++) {
					recomp::set_input_binding(input_index, binding_index, cur_device, recomp::InputField{});
				}
				model_handle.DirtyVariable("inputs");
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
				uintptr_t input_index = (uintptr_t)ptr;
				return Rml::DataVariable{&input_field_definition_instance, & recomp::get_input_binding(input_index, address.index, recomp::InputDevice::Controller)};
			}
		};
		// Static instance of the InputField array variable definition to a fixed pointer to return to RmlUi.
		static BindingContainerVariableDefinition binding_container_var_instance{};

		// Rml variable definition for an array of an array of InputField values (e.g. all the bindings for all inputs).
		struct InputContainerVariableDefinition : public Rml::VariableDefinition {
			InputContainerVariableDefinition() : Rml::VariableDefinition(Rml::DataVariableType::Array) {}

			virtual bool Get(void* ptr, Rml::Variant& variant) override { return false; }
			virtual bool Set(void* ptr, const Rml::Variant& variant) override { return false; }

			virtual int Size(void* ptr) override { return recomp::get_num_inputs(); }
			virtual Rml::DataVariable Child(void* ptr, const Rml::DataAddressEntry& address) override {
				// Encode the input index as the pointer to avoid needing to do any allocations.
				return Rml::DataVariable(&binding_container_var_instance, (void*)(uintptr_t)address.index);
			}
		};

		// Dummy type to associate with the variable definition.
		struct InputContainer {};
		constructor.RegisterCustomDataVariableDefinition<InputContainer>(Rml::MakeUnique<InputContainerVariableDefinition>());

		// Dummy instance of the dummy type to bind to the variable.
		static InputContainer dummy_container;
		constructor.Bind("inputs", &dummy_container);

		controls_model_handle = constructor.GetModelHandle();
	}

	void make_bindings(Rml::Context* context) override {
		make_graphics_bindings(context);
		make_controls_bindings(context);
	}
};

std::unique_ptr<recomp::MenuController> recomp::create_config_menu() {
	return std::make_unique<ConfigMenu>();
}
