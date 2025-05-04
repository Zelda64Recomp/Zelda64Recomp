#ifndef __RECOMP_UI__
#define __RECOMP_UI__

#include <memory>
#include <string>
#include <string_view>
#include <list>

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
        virtual void load_document() = 0;
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
    bool is_context_capturing_input();
    bool is_context_capturing_mouse();
    bool is_any_context_shown();
    ContextId try_close_current_context();

    ContextId get_launcher_context_id();
    ContextId get_config_context_id();
    ContextId get_config_sub_menu_context_id();

    enum class ConfigTab {
        General,
        Controls,
        Graphics,
        Sound,
        Mods,
        Debug,
    };

    void set_config_tab(ConfigTab tab);
    int config_tab_to_index(ConfigTab tab);
    Rml::ElementTabSet* get_config_tabset();
    Rml::Element* get_mod_tab();
    void set_config_tabset_mod_nav();
    void focus_mod_configure_button();

    enum class ButtonVariant {
        Primary,
        Secondary,
        Tertiary,
        Success,
        Error,
        Warning,
        NumVariants,
    };

    void init_styling(const std::filesystem::path& rcss_file);
    void init_prompt_context();
    void open_choice_prompt(
        const std::string& header_text,
        const std::string& content_text,
        const std::string& confirm_label_text,
        const std::string& cancel_label_text,
        std::function<void()> confirm_action,
        std::function<void()> cancel_action,
        ButtonVariant confirm_variant = ButtonVariant::Success,
        ButtonVariant cancel_variant = ButtonVariant::Error,
        bool focus_on_cancel = true,
        const std::string& return_element_id = ""
    );
    void open_info_prompt(
        const std::string& header_text,
        const std::string& content_text,
        const std::string& okay_label_text,
        std::function<void()> okay_action,
        ButtonVariant okay_variant = ButtonVariant::Error,
        const std::string& return_element_id = ""
    );
    void open_notification(
        const std::string& header_text,
        const std::string& content_text,
        const std::string& return_element_id = ""
    );
    void close_prompt();
    bool is_prompt_open();
    void update_mod_list(bool scan_mods = true);
    void process_game_started();

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
    Rml::Element* get_child_by_tag(Rml::Element* parent, const std::string& tag);

    void queue_image_from_bytes_rgba32(const std::string &src, const std::vector<char> &bytes, uint32_t width, uint32_t height);
    void queue_image_from_bytes_file(const std::string &src, const std::vector<char> &bytes);
    void release_image(const std::string &src);

    void drop_files(const std::list<std::filesystem::path> &file_list);
}

#endif
