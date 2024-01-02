#include "recomp_ui.h"
#include "../../ultramodern/config.hpp"
#include "RmlUi/Core.h"

ultramodern::GraphicsConfig cur_options;
ultramodern::GraphicsConfig new_options;
Rml::DataModelHandle options_handle;

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
		[option](const Rml::Variant& in) { set_option(*option, in); options_handle.DirtyVariable("options_changed"); }
	);
};

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
			[](Rml::Event& event) {
				cur_options = new_options;
				options_handle.DirtyVariable("options_changed");
				update_graphics_config(new_options);
			});
	}
	void make_bindings(Rml::Context* context) override {
		Rml::DataModelConstructor constructor = context->CreateDataModel("graphics_model");
		if (!constructor) {
			throw std::runtime_error("Failed to make RmlUi data model for the config menu");
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
				options_handle.DirtyVariable("options_changed");
			});

		constructor.BindFunc("options_changed",
			[](Rml::Variant& out) {
				out = (cur_options != new_options);
			});

		options_handle = constructor.GetModelHandle();
	}
};

std::unique_ptr<recomp::MenuController> recomp::create_config_menu() {
	return std::make_unique<ConfigMenu>();
}
