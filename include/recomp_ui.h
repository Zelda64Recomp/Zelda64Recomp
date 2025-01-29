#ifndef __RECOMP_UI__
#define __RECOMP_UI__

#include <memory>
#include <string>
#include <string_view>

// TODO move this file into src/ui

#include "SDL.h"
#include "RmlUi/Core.h"

#include "../src/ui/util/hsv.h"
#include "../src/ui/util/bem.h"

#include "../src/ui/core/ui_context.h"

namespace Rml {
    class ElementDocument;
    class EventListenerInstancer;
    class Context;
    class Event;
}

namespace recompui {
    class UiEventListenerInstancer;

    // TODO remove this once the UI has been ported over to the new system.
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

    void show_context(ContextId context, std::string_view param);
    void hide_context(ContextId context);
    void hide_all_contexts();
    bool is_context_shown(ContextId context);
    bool is_context_taking_input();
    bool is_any_context_shown();

    ContextId get_launcher_context_id();
    ContextId get_config_context_id();
    ContextId get_config_sub_menu_context_id();
    ContextId get_close_prompt_context_id();

    enum class ConfigTab {
        General,
        Controls,
        Graphics,
        Sound,
        Mods,
        Debug,
    };

    void set_config_tab(ConfigTab tab);

    enum class ButtonVariant {
        Primary,
        Secondary,
        Tertiary,
        Success,
        Error,
        Warning,
        NumVariants,
    };

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
    bool is_prompt_open();

    void apply_color_hack();
    void get_window_size(int& width, int& height);
    void set_cursor_visible(bool visible);
    void update_supported_options();
    void toggle_fullscreen();

    bool get_cont_active(void);
    void set_cont_active(bool active);
    void activate_mouse();

    void message_box(const char* msg);

    void set_render_hooks();

    Rml::ElementPtr create_custom_element(Rml::Element* parent, std::string tag);
    Rml::ElementDocument* load_document(const std::filesystem::path& path);
    Rml::ElementDocument* create_empty_document();
    void queue_image_from_bytes(const std::string &src, const std::vector<char> &bytes);
    void release_image(const std::string &src);
}

#endif
