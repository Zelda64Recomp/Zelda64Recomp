#include "recompui/recompui.h"
#include "zelda_game.h"
#include "zelda_launcher.h"

// UI Components
namespace {
    using namespace recompui;

    class SubtitleTitleButton : public Element {
    protected:
        Style selected_style;
        Style hover_style;
        Style focus_style;
        std::function<void()> pressed_callback;
        bool selected = false;
        bool right = false;
        bool soon = false;
        Element *exposed_contents = nullptr;
        
        Label *title = nullptr;
        Label *subtitle = nullptr;
        Element *soon_label_wrap = nullptr;
        Label *soon_label = nullptr;
        Label *soon_label_tm = nullptr;
        int soon_opacity = 0;
        int soon_opacity_goal = 0;
    
        // Element overrides.
        virtual void process_event(const Event &e) override {
            switch (e.type) {
            case EventType::Click:
                if (!soon && !selected && pressed_callback) {
                    pressed_callback();
                }
                break;
            case EventType::Hover: 
                if (soon) {
                    soon_opacity_goal = std::get<EventHover>(e.variant).active ? 255 : 0;
                    queue_update();
                } else {
                    set_style_enabled(hover_state, std::get<EventHover>(e.variant).active && !selected);
                }

                break;
            case EventType::Focus:
                if (!soon) {
                    set_style_enabled(focus_state, std::get<EventFocus>(e.variant).active);
                }
                break;
            case EventType::Update:
                if (soon && soon_opacity_goal != soon_opacity) {
                    int rate = soon_opacity_goal < soon_opacity ? -1 : 1;
                    soon_opacity += rate;
                    soon_label_tm->set_color(theme::color::TextDim, soon_opacity);
                    queue_update();
                }
                break;
            default:
                assert(false && "Unknown event type.");
                break;
            }
        };
        std::string_view get_type_name() override { return "SubtitleTitleButton"; }
    public:
        SubtitleTitleButton(ResourceId rid, Element *parent, const std::string &text, bool right_align = false, bool soon = false) : Element(rid, parent, Events(EventType::Click, EventType::Hover, EventType::Enable, EventType::Focus, EventType::Update), "button", false) {
            auto context = recompui::get_current_context();
            this->soon = soon;
            right = right_align;
            set_display(Display::Block);
            set_position(Position::Relative);
            set_width_auto();
            set_height_auto();
            set_padding(0);
            set_background_color(theme::color::Transparent);
            set_color(theme::color::TextDim);
            set_text_align(right ? TextAlign::Right : TextAlign::Left);
            
            set_opacity(0.5);

            selected_style.set_color(theme::color::Text);
            selected_style.set_cursor(Cursor::None);
            selected_style.set_opacity(1);
            hover_style.set_color(theme::color::Primary);
            hover_style.set_opacity(1);
            focus_style.set_color(theme::color::Primary);
            focus_style.set_opacity(1);

            if (!soon) {
                enable_focus();
                set_cursor(Cursor::Pointer);
            }
            add_style(&selected_style, checked_state);
            add_style(&hover_style, hover_state);
            add_style(&focus_style, focus_state);
            title = context.create_element<Label>(this, "Zelda 64: Recompiled", theme::Typography::Header3);
            title->set_margin_bottom(12);
            subtitle = context.create_element<Label>(this, text, theme::Typography::Header1);

            if (soon) {
                soon_label_wrap = context.create_element<Element>(this, 0, "div", false);
                soon_label_wrap->set_color(theme::color::TextDim, 128);
                soon_label_wrap->set_margin_top(16);
                {
                    soon_label = context.create_element<Label>(soon_label_wrap, "COMING SOON™", theme::Typography::LabelSM);
                    soon_label->set_display(Display::Inline);
                    soon_label_tm = context.create_element<Label>(soon_label_wrap, "™", theme::Typography::LabelSM);
                    soon_label_tm->set_display(Display::Inline);
                    soon_label_tm->set_color(theme::color::TextDim, 0);
                }
            }
        };
        void set_pressed_callback(std::function<void()> callback) { pressed_callback = callback; };
        Style* get_selected_style() { return &selected_style; }
        Style* get_hover_style() { return &hover_style; }
        Style* get_focus_style() { return &focus_style; }
        void set_selected(bool sel) {
            selected = sel;
            set_style_enabled(checked_state, selected);
            if (selected) exposed_contents->display_show();
            else exposed_contents->display_hide();
        }
        void set_exposed_contents(Element *el) { exposed_contents = el; }
    };

    class GameHalf {
    private:
        Element *root;
        Element *title_wrapper;
        Element *content_wrapper;
        SubtitleTitleButton *title_button;
        zelda64::Game game;
        std::string name;
        std::function<void()> on_change_game;
        bool right = false;
        bool soon = false;
    
        void style_vertical_split() {
            root->set_display(Display::Flex);
            root->set_position(Position::Absolute);
            root->set_top(0);
            root->set_bottom(0);
            root->set_flex_direction(FlexDirection::Column);
            root->set_justify_content(JustifyContent::SpaceBetween);
            // Reset styles set in LauncherMenu's constructor
            root->set_height_auto();
            root->set_width_auto();
            root->set_as_navigation_container(NavigationType::Vertical);
    
            if (right) {
                root->set_right(0);
                root->set_left(50, Unit::Percent);
                root->set_align_items(AlignItems::FlexEnd);
            } else {
                root->set_right(50, Unit::Percent);
                root->set_left(0);
                root->set_align_items(AlignItems::FlexStart);
            }
        }

        void add_title_wrapper() {
            auto context = get_current_context();
            title_wrapper = context.create_element<Element>(root, 0, "div", false);

            title_wrapper->set_flex(1, 1);
            title_wrapper->set_flex_basis_auto();
            title_wrapper->set_height_auto();
            title_wrapper->set_width_auto();
            title_wrapper->set_padding_top(96);

            if (right) {
                title_wrapper->set_padding_right(96);
                title_wrapper->set_padding_left(0);
            } else {
                title_wrapper->set_padding_right(0);
                title_wrapper->set_padding_left(96);
            }
        }

        void add_content_wrapper() {
            auto context = get_current_context();
            content_wrapper = context.create_element<Element>(root, 0, "div", false);

            content_wrapper->set_display(Display::Flex);
            content_wrapper->set_position(Position::Relative);
            content_wrapper->set_flex(1, 1);
            content_wrapper->set_flex_basis(100, Unit::Percent);
            content_wrapper->set_height_auto();
            content_wrapper->set_width(100, Unit::Percent);
            content_wrapper->set_flex_direction(FlexDirection::Column);
            content_wrapper->set_padding(32);
            content_wrapper->set_justify_content(JustifyContent::FlexEnd);

            if (right) {
                content_wrapper->set_align_items(AlignItems::FlexEnd);
            } else {
                content_wrapper->set_align_items(AlignItems::FlexStart);
            }
        }

    public:
        GameHalf(zelda64::Game game, const std::string &name, bool right = false, bool soon = false) : game(game), name(name), right(right), soon(soon) {};

        void init(Element *root_element) {
            root = root_element;
            root->clear_children();
            style_vertical_split(); 
            auto context = recompui::get_current_context();

            add_title_wrapper();
            title_button = context.create_element<SubtitleTitleButton>(title_wrapper, name, right, soon);
            add_content_wrapper();

            const recomp::GameEntry &game_entry = zelda64::get_game_entry(game);
            auto game_options_menu = context.create_element<GameOptionsMenu>(content_wrapper,
                game_entry.game_id,
                game_entry.mod_game_id,
                game_entry.display_name,
                game_entry.thumbnail_bytes,
                right ? recompui::GameOptionsMenuLayout::Right : recompui::GameOptionsMenuLayout::Left
            );
            game_options_menu->set_as_navigation_container(recompui::NavigationType::Vertical);
            game_options_menu->add_default_options();

            title_button->set_exposed_contents(game_options_menu);
        }

        void derive_selected() {
            title_button->set_selected(zelda64::get_current_game() == game);
        }

        void set_on_changed_game(std::function<void()> on_change_game_callback) {
            on_change_game = on_change_game_callback;
            title_button->set_pressed_callback([this]() {
                on_change_game();
            });
        }
    };
};

static GameHalf oot_half{zelda64::Game::OoT, "Ocarina of Time", false, true};
static GameHalf mm_half{zelda64::Game::MM, "Majora's Mask", true};


void zelda64::on_launcher_init(recompui::LauncherMenu *menu) {
    menu->remove_default_title();
    menu->set_as_navigation_container(recompui::NavigationType::Horizontal);

    // changing the way these are used to be split into vertical sections (each game)

    oot_half.init(menu->get_background_container());
    mm_half.init(menu->get_menu_container());

    recompui::update_game_mod_id(get_game_entry(get_current_game()).mod_game_id);

    oot_half.set_on_changed_game([]() {
        swap_current_game();
        oot_half.derive_selected();
        mm_half.derive_selected();
    });
    mm_half.set_on_changed_game([]() {
        swap_current_game();
        oot_half.derive_selected();
        mm_half.derive_selected();
    });

    oot_half.derive_selected();
    mm_half.derive_selected();

    // zelda64::launcher_animation_setup(menu);
}
