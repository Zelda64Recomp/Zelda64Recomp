#ifndef __RECOMP_UI__
#define __RECOMP_UI__

#include <memory>
#include <string>

#include "SDL.h"
#include "RmlUi/Core.h"

namespace Rml {
	class ElementDocument;
	class EventListenerInstancer;
	class Context;
	class Event;
}

namespace recompui {
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

	using event_handler_t = void(const std::string& param, Rml::Event&);

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
	Menu get_current_menu();

	enum class ConfigSubmenu {
		General,
		Controls,
		Graphics,
		Audio,
		Debug,
		Count
	};

	enum class ButtonVariant {
		Primary,
		Secondary,
		Tertiary,
		Success,
		Error,
		Warning,
		NumVariants,
	};

	void set_config_submenu(ConfigSubmenu submenu);

	void destroy_ui();
	void apply_color_hack();
	void get_window_size(int& width, int& height);
	void set_cursor_visible(bool visible);
	void update_supported_options();
	void toggle_fullscreen();
	void update_rml_display_refresh_rate();

	extern const std::unordered_map<ButtonVariant, std::string> button_variants;

	struct PromptContext {
		Rml::DataModelHandle model_handle;
		std::string header = "";
		std::string content = "";
		std::string confirmLabel = "Confirm";
		std::string cancelLabel = "Cancel";
		ButtonVariant confirmVariant = ButtonVariant::Success;
		ButtonVariant cancelVariant = ButtonVariant::Error;
		std::function<void()> onConfirm;
		std::function<void()> onCancel;

		std::string returnElementId = "";

		bool open = false;
		bool shouldFocus = false;
		bool focusOnCancel = true;

		PromptContext() = default;

		void close_prompt();
		void open_prompt(
			const std::string& headerText,
			const std::string& contentText,
			const std::string& confirmLabelText,
			const std::string& cancelLabelText,
			std::function<void()> confirmCb,
			std::function<void()> cancelCb,
			ButtonVariant _confirmVariant = ButtonVariant::Success,
			ButtonVariant _cancelVariant = ButtonVariant::Error,
			bool _focusOnCancel = true,
			const std::string& _returnElementId = ""
		);
		void on_confirm(void);
		void on_cancel(void);
		void on_click(Rml::DataModelHandle model_handle, Rml::Event& event, const Rml::VariantList& inputs);
	};

	PromptContext *get_prompt_context(void);

	bool get_cont_active(void);
	void set_cont_active(bool active);
	void activate_mouse();

	void message_box(const char* msg);

	void set_render_hooks();
}

#endif
