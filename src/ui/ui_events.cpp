#define _CRT_SECURE_NO_WARNINGS

#include "recomp_ui.h"
#include "../../ultramodern/ultramodern.hpp"
#include "../../ultramodern/config.hpp"
#include "common/rt64_user_configuration.h"

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

NLOHMANN_JSON_SERIALIZE_ENUM(ultramodern::Resolution, {
	{ultramodern::Resolution::Original, "Original"},
	{ultramodern::Resolution::Original2x, "Original2x"},
	{ultramodern::Resolution::Auto, "Auto"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ultramodern::WindowMode, {
	{ultramodern::WindowMode::Windowed, "Windowed"},
	{ultramodern::WindowMode::Fullscreen, "Fullscreen"}
});

ultramodern::GraphicsConfig cur_options;
ultramodern::GraphicsConfig new_options;
Rml::DataModelHandle options_handle;

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

void recomp::make_ui_bindings(Rml::Context* context) {
	Rml::DataModelConstructor constructor = context->CreateDataModel("graphics_model");
	if (!constructor) {
		throw std::runtime_error("Failed to make RmlUi data model constructor");
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

std::unique_ptr<Rml::EventListenerInstancer> recomp::make_event_listener_instancer() {
	std::unique_ptr<UiEventListenerInstancer> ret = std::make_unique<UiEventListenerInstancer>();

	ret->register_event("start_game",
		[](Rml::Event& event) {
			ultramodern::start_game(0);
			set_current_menu(Menu::Config);
		}
	);

	ret->register_event("apply_options",
		[](Rml::Event& event) {
			cur_options = new_options;
			options_handle.DirtyVariable("options_changed");
			update_graphics_config(new_options);
		});

	return ret;
}
