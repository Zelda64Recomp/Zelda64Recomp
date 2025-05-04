#include "ui_mod_menu.h"
#include "ui_utils.h"
#include "recomp_ui.h"
#include "zelda_support.h"
#include "zelda_render.h"

#include "librecomp/mods.hpp"

#include <string>

#ifdef WIN32
#include <shellapi.h>
#endif

// TODO:
// - Set up navigation.
// - Add hover and active state for mod entries.

namespace recompui {

static std::string generate_thumbnail_src_for_mod(const std::string &mod_id) {
    return "?/mods/" + mod_id + "/thumb";
}

static bool is_mod_enabled_or_auto(const std::string &mod_id) {
    return recomp::mods::is_mod_enabled(mod_id) || recomp::mods::is_mod_auto_enabled(mod_id);
}

// ModEntryView
#define COL_TEXT_DEFAULT 242, 242, 242
#define COL_TEXT_DIM 204, 204, 204
#define COL_SECONDARY 23, 214, 232
constexpr float modEntryHeight = 120.0f;
constexpr float modEntryPadding = 4.0f;

extern const std::string mod_tab_id;
const std::string mod_tab_id = "#tab_mods";

ModEntryView::ModEntryView(Element *parent) : Element(parent, Events(EventType::Update)) {
    ContextId context = get_current_context();

    set_display(Display::Flex);
    set_flex_direction(FlexDirection::Row);
    set_width(100.0f, Unit::Percent);
    set_height_auto();
    set_padding(modEntryPadding);
    set_border_left_width(2.0f);
    set_border_color(Color{ COL_TEXT_DEFAULT, 12 });
    set_background_color(Color{ COL_TEXT_DEFAULT, 12 });
    set_cursor(Cursor::Pointer);
    set_color(Color{ COL_TEXT_DEFAULT, 255 });

    checked_style.set_border_color(Color{ COL_TEXT_DEFAULT, 160 });
    checked_style.set_color(Color{ 255, 255, 255, 255 });
    checked_style.set_background_color(Color{ 26, 24, 32, 255 });
    hover_style.set_border_color(Color{ COL_TEXT_DEFAULT, 64 });
    checked_hover_style.set_border_color(Color{ COL_TEXT_DEFAULT, 255 });
    pulsing_style.set_border_color(Color{ 23, 214, 232, 244 });

    {
        thumbnail_image = context.create_element<Image>(this, "");
        thumbnail_image->set_width(modEntryHeight);
        thumbnail_image->set_height(modEntryHeight);
        thumbnail_image->set_min_width(modEntryHeight);
        thumbnail_image->set_min_height(modEntryHeight);
        thumbnail_image->set_background_color(Color{ 190, 184, 219, 25 });


        body_container = context.create_element<Element>(this);
        body_container->set_width_auto();
        body_container->set_margin_left(16.0f);
        body_container->set_padding_top(8.0f);
        body_container->set_padding_bottom(8.0f);
        body_container->set_max_height(modEntryHeight);
        body_container->set_overflow_y(Overflow::Hidden);

        {
            name_label = context.create_element<Label>(body_container, LabelStyle::Normal);
            description_label = context.create_element<Label>(body_container, LabelStyle::Small);
            description_label->set_margin_top(4.0f);
            description_label->set_color(Color{ COL_TEXT_DIM, 255 });
        } // body_container
    } // this

    add_style(&checked_style, checked_state);
    add_style(&hover_style, hover_state);
    add_style(&checked_hover_style, { checked_state, hover_state });
    add_style(&pulsing_style, { focus_state });
}

ModEntryView::~ModEntryView() {

}

void ModEntryView::set_mod_details(const recomp::mods::ModDetails &details) {
    name_label->set_text(details.display_name);
    description_label->set_text(details.short_description);
}

void ModEntryView::set_mod_thumbnail(const std::string &thumbnail) {
    thumbnail_image->set_src(thumbnail);
}

void ModEntryView::set_mod_enabled(bool enabled) {
    set_opacity(enabled ? 1.0f : 0.5f);
}

void ModEntryView::set_selected(bool selected) {
    set_style_enabled(checked_state, selected);
}

void ModEntryView::set_focused(bool focused) {
    set_style_enabled(focus_state, focused);
}

void ModEntryView::process_event(const Event &e) {
    switch (e.type) {
    case EventType::Update:
        if (is_style_enabled(focus_state)) {
            pulsing_style.set_color(recompui::get_pulse_color(750));
            apply_styles();
            queue_update();
        }

        break;
    default:
        break;
    }
}

// ModEntryButton

ModEntryButton::ModEntryButton(Element *parent, uint32_t mod_index) : Element(parent, Events(EventType::Click, EventType::Hover, EventType::Focus, EventType::Drag)) {
    this->mod_index = mod_index;

    set_drag(Drag::Drag);
    enable_focus();

    ContextId context = get_current_context();
    view = context.create_element<ModEntryView>(this);
}

ModEntryButton::~ModEntryButton() {

}

void ModEntryButton::set_mod_selected_callback(std::function<void(uint32_t)> callback) {
    selected_callback = callback;
}

void ModEntryButton::set_mod_drag_callback(std::function<void(uint32_t, EventDrag)> callback) {
    drag_callback = callback;
}

void ModEntryButton::set_mod_details(const recomp::mods::ModDetails &details) {
    view->set_mod_details(details);
}

void ModEntryButton::set_mod_thumbnail(const std::string &thumbnail) {
    view->set_mod_thumbnail(thumbnail);
}

void ModEntryButton::set_mod_enabled(bool enabled) {
    view->set_mod_enabled(enabled);
}

void ModEntryButton::set_selected(bool selected) {
    view->set_selected(selected);
}

void ModEntryButton::set_focused(bool focused) {
    view->set_focused(focused);
    view->queue_update();
}

void ModEntryButton::process_event(const Event& e) {
    switch (e.type) {
    case EventType::Focus:
        selected_callback(mod_index);
        set_focused(std::get<EventFocus>(e.variant).active);
        break;
    case EventType::Hover:
        view->set_style_enabled(hover_state, std::get<EventHover>(e.variant).active);
        break;
    case EventType::Drag:
        drag_callback(mod_index, std::get<EventDrag>(e.variant));
        break;
    default:
        break;
    }
}

// ModEntrySpacer

void ModEntrySpacer::check_height_distance() {
    constexpr float tolerance = 0.01f;
    if (abs(target_height - height) < tolerance) {
        height = target_height;
        set_height(height, Unit::Dp);
    }
    else {
        queue_update();
    }
}

void ModEntrySpacer::process_event(const Event &e) {
    switch (e.type) {
    case EventType::Update: {
        std::chrono::high_resolution_clock::duration now = ultramodern::time_since_start();
        float delta_time = std::max(std::chrono::duration<float>(now - last_time).count(), 0.0f);
        constexpr float dp_speed = 1000.0f;
        last_time = now;

        if (target_height < height) {
            height += std::max(-dp_speed * delta_time, target_height - height);
        }
        else {
            height += std::min(dp_speed * delta_time, target_height - height);
        }

        set_height(height, Unit::Dp);
        check_height_distance();
        break;
    }
    default:
        break;
    }
}

ModEntrySpacer::ModEntrySpacer(Element *parent) : Element(parent) {
    // Do nothing.
}

void ModEntrySpacer::set_target_height(float target_height, bool animate_to_target) {
    this->target_height = target_height;

    if (animate_to_target) {
        last_time = ultramodern::time_since_start();
        check_height_distance();
    }
    else {
        height = target_height;
        set_height(target_height, Unit::Dp);
    }
}

// ModMenu

void ModMenu::refresh_mods(bool scan_mods) {
    for (const std::string &thumbnail : loaded_thumbnails) {
        recompui::release_image(thumbnail);
    }

    if (scan_mods) {
        recomp::mods::scan_mods();
    }
    mod_details = recomp::mods::get_all_mod_details(game_mod_id);
    create_mod_list();
}

void ModMenu::open_mods_folder() {
    std::filesystem::path mods_directory = recomp::mods::get_mods_directory();
#if defined(WIN32)
    std::wstring path_wstr = mods_directory.wstring();
    ShellExecuteW(NULL, L"open", path_wstr.c_str(), NULL, NULL, SW_SHOWDEFAULT);
#elif defined(__linux__)
    std::string command = "xdg-open \"" + mods_directory.string() + "\" &";
    std::system(command.c_str());
#elif defined(__APPLE__)
    std::string command = "open \"" + mods_directory.string() + "\"";
    std::system(command.c_str());
#else
    static_assert(false, "Not implemented for this platform.");
#endif
}

void ModMenu::open_install_dialog() {
    zelda64::open_file_dialog_multiple([](bool success, const std::list<std::filesystem::path>& paths) {
        if (success) {
            ContextId old_context = recompui::try_close_current_context();

            recompui::drop_files(paths);

            if (old_context != ContextId::null()) {
                old_context.open();
            }
        }
    });
}

void ModMenu::mod_toggled(bool enabled) {
    if (active_mod_index >= 0) {
        recomp::mods::enable_mod(mod_details[active_mod_index].mod_id, enabled);
        
        // Refresh enabled status for all mods in case one of them got auto-enabled due to being a dependency.
        for (size_t i = 0; i < mod_entry_buttons.size(); i++) {
            mod_entry_buttons[i]->set_mod_enabled(is_mod_enabled_or_auto(mod_details[i].mod_id));
        }
    }
}

void ModMenu::mod_selected(uint32_t mod_index) {
    if (active_mod_index >= 0) {
        mod_entry_buttons[active_mod_index]->set_selected(false);
    }

    active_mod_index = mod_index;

    if (active_mod_index >= 0) {
        std::string thumbnail_src = generate_thumbnail_src_for_mod(mod_details[mod_index].mod_id);
        const recomp::mods::ConfigSchema &config_schema = recomp::mods::get_mod_config_schema(mod_details[active_mod_index].mod_id);
        bool toggle_checked = is_mod_enabled_or_auto(mod_details[mod_index].mod_id);
        bool auto_enabled = recomp::mods::is_mod_auto_enabled(mod_details[mod_index].mod_id);
        bool toggle_enabled = !auto_enabled && (mod_details[mod_index].runtime_toggleable || !ultramodern::is_game_started());
        bool configure_enabled = !config_schema.options.empty();
        mod_details_panel->set_mod_details(mod_details[mod_index], thumbnail_src, toggle_checked, toggle_enabled, auto_enabled, configure_enabled);
        mod_entry_buttons[active_mod_index]->set_selected(true);

        mod_details_panel->setup_mod_navigation(mod_entry_buttons[mod_index]);

        // Navigation from the bottom bar.
        Button *configure_button = mod_details_panel->get_configure_button();
        Toggle *enable_toggle = mod_details_panel->get_enable_toggle();
        if (configure_enabled) {
            refresh_button->set_nav(NavDirection::Up, configure_button);
            mods_folder_button->set_nav(NavDirection::Up, configure_button);
        }
        else if (toggle_enabled) {
            refresh_button->set_nav(NavDirection::Up, enable_toggle);
            mods_folder_button->set_nav(NavDirection::Up, enable_toggle);
        }
        else {
            refresh_button->set_nav_manual(NavDirection::Up, mod_tab_id);
            mods_folder_button->set_nav_manual(NavDirection::Up, mod_tab_id);
        }

        // Navigation from the mod list.
        if (toggle_enabled) {
            mod_entry_buttons[active_mod_index]->set_nav(NavDirection::Right, enable_toggle);
        }
        else if (configure_enabled) {
            mod_entry_buttons[active_mod_index]->set_nav(NavDirection::Right, configure_button);
        }
        else {
            mod_entry_buttons[active_mod_index]->set_nav_none(NavDirection::Right);
        }
    }
}

void ModMenu::mod_dragged(uint32_t mod_index, EventDrag drag) {
    constexpr float spacer_height = modEntryHeight + modEntryPadding * 2.0f;

    switch (drag.phase) {
    case DragPhase::Start: {
        for (size_t i = 0; i < mod_entry_buttons.size(); i++) {
            mod_entry_middles[i] = mod_entry_buttons[i]->get_absolute_top() + mod_entry_buttons[i]->get_client_height() / 2.0f;
        }

        // When the drag phase starts, we make the floating mod details visible and store the relative coordinate of the
        // mouse cursor. Instantly hide the real element and use a spacer in its place that will stay on the same size as
        // long as the cursor is hovering over this slot.
        float width = mod_entry_buttons[mod_index]->get_client_width();
        float height = mod_entry_buttons[mod_index]->get_client_height();
        float left = mod_entry_buttons[mod_index]->get_absolute_left() - get_absolute_left();
        float top = mod_entry_buttons[mod_index]->get_absolute_top() - (height / 2.0f); // TODO: Figure out why this adjustment is even necessary.
        mod_entry_buttons[mod_index]->set_display(Display::None);
        mod_entry_buttons[mod_index]->set_focused(false);
        mod_entry_floating_view->set_display(Display::Flex);
        mod_entry_floating_view->set_mod_details(mod_details[mod_index]);
        mod_entry_floating_view->set_mod_thumbnail(generate_thumbnail_src_for_mod(mod_details[mod_index].mod_id));
        mod_entry_floating_view->set_mod_enabled(is_mod_enabled_or_auto(mod_details[mod_index].mod_id));
        mod_entry_floating_view->set_left(left, Unit::Px);
        mod_entry_floating_view->set_top(top, Unit::Px);
        mod_entry_floating_view->set_width(width, Unit::Px);
        mod_entry_floating_view->set_height(height, Unit::Px);
        mod_drag_start_coordinates[0] = drag.x;
        mod_drag_start_coordinates[1] = drag.y;
        mod_drag_view_coordinates[0] = left;
        mod_drag_view_coordinates[1] = top;
        
        mod_drag_target_index = mod_index;
        mod_entry_spacers[mod_drag_target_index]->set_target_height(spacer_height, false);
        break;
    }
    case DragPhase::Move: {
        // Binary search for the drag area.
        uint32_t low = 0;
        uint32_t high = mod_entry_buttons.size();
        while (low < high) {
            uint32_t mid = low + (high - low) / 2;
            if (drag.y < mod_entry_middles[mid]) {
                high = mid;
            }
            else {
                low = mid + 1;
            }
        }
        
        uint32_t new_index = low;
        float delta_x = drag.x - mod_drag_start_coordinates[0];
        float delta_y = drag.y - mod_drag_start_coordinates[1];
        mod_entry_floating_view->set_left(mod_drag_view_coordinates[0] + delta_x, Unit::Px);
        mod_entry_floating_view->set_top(mod_drag_view_coordinates[1] + delta_y, Unit::Px);
        if (mod_drag_target_index != new_index) {
            mod_entry_spacers[mod_drag_target_index]->set_target_height(0.0f, true);
            mod_entry_spacers[new_index]->set_target_height(spacer_height, true);
            mod_drag_target_index = new_index;
        }

        break;
    }
    case DragPhase::End: {
        // Dragging has ended, hide the floating view.
        mod_entry_buttons[mod_index]->set_display(Display::Block);
        mod_entry_buttons[mod_index]->set_selected(false);
        mod_entry_spacers[mod_drag_target_index]->set_target_height(0.0f, false);
        mod_entry_floating_view->set_display(Display::None);

        // Result needs a small substraction when dragging downwards.
        if (mod_drag_target_index > mod_index) {
            mod_drag_target_index--;
        }

        // Re-order the mods and update all the details on the menu.
        recomp::mods::set_mod_index(game_mod_id, mod_details[mod_index].mod_id, mod_drag_target_index);
        mod_details = recomp::mods::get_all_mod_details(game_mod_id);
        for (size_t i = 0; i < mod_entry_buttons.size(); i++) {
            mod_entry_buttons[i]->set_mod_details(mod_details[i]);
            mod_entry_buttons[i]->set_mod_thumbnail(generate_thumbnail_src_for_mod(mod_details[i].mod_id));
            mod_entry_buttons[i]->set_mod_enabled(is_mod_enabled_or_auto(mod_details[i].mod_id));
        }

        mod_entry_buttons[mod_drag_target_index]->set_selected(true);
        active_mod_index = mod_drag_target_index;

        break;
    }
    default:
        break;
    }
}

// TODO remove this once this is migrated to the new system.
ContextId sub_menu_context;

ContextId get_config_sub_menu_context_id() {
    return sub_menu_context;
}

bool ModMenu::handle_special_config_options(const recomp::mods::ConfigOption& option, const recomp::mods::ConfigValueVariant& config_value) {
    if (zelda64::renderer::is_texture_pack_enable_config_option(option, true)) {
        const recomp::mods::ConfigOptionEnum &option_enum = std::get<recomp::mods::ConfigOptionEnum>(option.variant);

        config_sub_menu->add_radio_option(option.id, option.name, option.description, std::get<uint32_t>(config_value), option_enum.options,
            [this](const std::string &id, uint32_t value) {
                mod_enum_option_changed(id, value);
                mod_hd_textures_enabled_changed(value);
            });

        return true;
    }

    return false;
}

void ModMenu::mod_configure_requested() {
    if (active_mod_index >= 0) {
        // Record the context that was open when this function was called and close it.
        ContextId prev_context = recompui::get_current_context();
        prev_context.close();

        // Open the sub menu context and set up the element.
        sub_menu_context.open();
        config_sub_menu->clear_options();

        const recomp::mods::ConfigSchema &config_schema = recomp::mods::get_mod_config_schema(mod_details[active_mod_index].mod_id);
        for (const recomp::mods::ConfigOption &option : config_schema.options) {
            recomp::mods::ConfigValueVariant config_value = recomp::mods::get_mod_config_value(mod_details[active_mod_index].mod_id, option.id);
            if (std::holds_alternative<std::monostate>(config_value)) {
                continue;
            }

            if (handle_special_config_options(option, config_value)) {
                continue;
            }

            switch (option.type) {
            case recomp::mods::ConfigOptionType::Enum: {
                const recomp::mods::ConfigOptionEnum &option_enum = std::get<recomp::mods::ConfigOptionEnum>(option.variant);
                config_sub_menu->add_radio_option(option.id, option.name, option.description, std::get<uint32_t>(config_value), option_enum.options,
                    [this](const std::string &id, uint32_t value){ mod_enum_option_changed(id, value); });
                break;
            }
            case recomp::mods::ConfigOptionType::Number: {
                const recomp::mods::ConfigOptionNumber &option_number = std::get<recomp::mods::ConfigOptionNumber>(option.variant);
                config_sub_menu->add_slider_option(option.id, option.name, option.description, std::get<double>(config_value), option_number.min, option_number.max, option_number.step, option_number.percent,
                    [this](const std::string &id, double value){ mod_number_option_changed(id, value); });
                break;
            }
            case recomp::mods::ConfigOptionType::String: {
                config_sub_menu->add_text_option(option.id, option.name, option.description, std::get<std::string>(config_value),
                    [this](const std::string &id, const std::string &value){ mod_string_option_changed(id, value); });
                break;
            }
            default:
                assert(false && "Unknown config option type.");
                break;
            }
        }

        config_sub_menu->enter(mod_details[active_mod_index].display_name);
        sub_menu_context.close();

        // Reopen the context that was open when this function was called.
        prev_context.open();
        
        // Hide the config menu and show the sub menu.
        recompui::hide_context(recompui::get_config_context_id());
        recompui::show_context(sub_menu_context, "");
    }
}

void ModMenu::mod_enum_option_changed(const std::string &id, uint32_t value) {
    if (active_mod_index >= 0) {
        recomp::mods::set_mod_config_value(mod_details[active_mod_index].mod_id, id, value);
    }
}

void ModMenu::mod_string_option_changed(const std::string &id, const std::string &value) {
    if (active_mod_index >= 0) {
        recomp::mods::set_mod_config_value(mod_details[active_mod_index].mod_id, id, value);
    }
}

void ModMenu::mod_number_option_changed(const std::string &id, double value) {
    if (active_mod_index >= 0) {
        recomp::mods::set_mod_config_value(mod_details[active_mod_index].mod_id, id, value);
    }
}

void ModMenu::mod_hd_textures_enabled_changed(uint32_t value) {
    if (active_mod_index >= 0) {
        if (value) {
            zelda64::renderer::secondary_enable_texture_pack(mod_details[active_mod_index].mod_id);
        }
        else {
            zelda64::renderer::secondary_disable_texture_pack(mod_details[active_mod_index].mod_id);
        }
    }
}

void ModMenu::create_mod_list() {
    ContextId context = get_current_context();
    
    // Clear the contents of the list scroll.
    list_scroll_container->clear_children();
    mod_entry_buttons.clear();
    mod_entry_spacers.clear();

    Toggle* enable_toggle = mod_details_panel->get_enable_toggle();

    // Create the child elements for the list scroll.
    for (size_t mod_index = 0; mod_index < mod_details.size(); mod_index++) {
        const std::vector<char> &thumbnail = recomp::mods::get_mod_thumbnail(mod_details[mod_index].mod_id);
        std::string thumbnail_name = generate_thumbnail_src_for_mod(mod_details[mod_index].mod_id);
        if (!thumbnail.empty()) {
            recompui::queue_image_from_bytes_file(thumbnail_name, thumbnail);
            loaded_thumbnails.emplace(thumbnail_name);
        }

        ModEntrySpacer *spacer = context.create_element<ModEntrySpacer>(list_scroll_container);
        mod_entry_spacers.emplace_back(spacer);

        ModEntryButton *mod_entry = context.create_element<ModEntryButton>(list_scroll_container, mod_index);
        mod_entry->set_mod_selected_callback([this](uint32_t mod_index){ mod_selected(mod_index); });
        mod_entry->set_mod_drag_callback([this](uint32_t mod_index, recompui::EventDrag drag){ mod_dragged(mod_index, drag); });
        mod_entry->set_mod_details(mod_details[mod_index]);
        mod_entry->set_mod_thumbnail(thumbnail_name);
        mod_entry->set_mod_enabled(is_mod_enabled_or_auto(mod_details[mod_index].mod_id));
        mod_entry_buttons.emplace_back(mod_entry);
    }

    if (!mod_entry_buttons.empty()) {
        mod_entry_buttons.front()->set_nav_manual(NavDirection::Up, mod_tab_id);
        mod_entry_buttons.back()->set_nav(NavDirection::Down, install_mods_button);
        install_mods_button->set_nav(NavDirection::Up, mod_entry_buttons.back());
    }
    else {
        install_mods_button->set_nav_manual(NavDirection::Up, mod_tab_id);
    }

    Rml::ElementTabSet* tabset = recompui::get_config_tabset();
    if (tabset && tabset->GetActiveTab() == recompui::config_tab_to_index(ConfigTab::Mods)) {
        recompui::set_config_tabset_mod_nav();
    }       

    // Add one extra spacer at the bottom.
    ModEntrySpacer *spacer = context.create_element<ModEntrySpacer>(list_scroll_container);
    mod_entry_spacers.emplace_back(spacer);

    mod_entry_middles.resize(mod_entry_buttons.size());

    bool mods_available = !mod_details.empty();
    body_container->set_display(mods_available ? Display::Flex : Display::None);
    body_empty_container->set_display(mods_available ? Display::None : Display::Flex);
    if (mods_available) {
        mod_selected(0);
    }
}

void ModMenu::process_event(const Event &e) {
    if (e.type == EventType::Update) {
        if (mods_dirty) {
            refresh_mods(mod_scan_queued);
            mods_dirty = false;
            mod_scan_queued = false;
        }
        if (ultramodern::is_game_started()) {
            install_mods_button->set_enabled(false);
            refresh_button->set_enabled(false);
        }
        if (active_mod_index != -1) {        
            bool auto_enabled = recomp::mods::is_mod_auto_enabled(mod_details[active_mod_index].mod_id);
            bool toggle_enabled = !auto_enabled && (mod_details[active_mod_index].runtime_toggleable || !ultramodern::is_game_started());
            if (!toggle_enabled) {
                mod_details_panel->disable_toggle();
            }
        }
    }
}

ModMenu::ModMenu(Element *parent) : Element(parent) {
    game_mod_id = "mm";

    ContextId context = get_current_context();

    set_display(Display::Flex);
    set_flex(1.0f, 1.0f, 100.0f);
    set_flex_direction(FlexDirection::Column);
    set_align_items(AlignItems::Center);
    set_justify_content(JustifyContent::FlexStart);
    set_width(100.0f, Unit::Percent);
    set_height(100.0f, Unit::Percent);
    
    {
        body_container = context.create_element<Container>(this, FlexDirection::Row, JustifyContent::FlexStart);
        body_container->set_flex(1.0f, 1.0f, 100.0f);
        body_container->set_width(100.0f, Unit::Percent);
        body_container->set_height(100.0f, Unit::Percent);
        {
            list_container = context.create_element<Container>(body_container, FlexDirection::Column, JustifyContent::Center);
            list_container->set_display(Display::Block);
            list_container->set_flex_basis(100.0f);
            list_container->set_align_items(AlignItems::Center);
            list_container->set_height(100.0f, Unit::Percent);
            list_container->set_background_color(Color{ 0, 0, 0, 89 });
            list_container->set_border_bottom_left_radius(16.0f);
            {
                list_scroll_container = context.create_element<ScrollContainer>(list_container, ScrollDirection::Vertical);
            } // list_container

            mod_details_panel = context.create_element<ModDetailsPanel>(body_container);
            mod_details_panel->set_mod_toggled_callback([this](bool enabled){ mod_toggled(enabled); });
            mod_details_panel->set_mod_configure_pressed_callback([this](){ mod_configure_requested(); });
        } // body_container
        
        body_empty_container = context.create_element<Container>(this, FlexDirection::Column, JustifyContent::SpaceBetween);
        body_empty_container->set_flex(1.0f, 1.0f, 100.0f);
        body_empty_container->set_display(Display::None);
        {
            context.create_element<Element>(body_empty_container);
            context.create_element<Label>(body_empty_container, "You have no mods. Go get some!", LabelStyle::Large);
            context.create_element<Element>(body_empty_container);
        } // body_empty_container

        footer_container = context.create_element<Container>(this, FlexDirection::Row, JustifyContent::FlexStart);
        footer_container->set_width(100.0f, recompui::Unit::Percent);
        footer_container->set_align_items(recompui::AlignItems::Center);
        footer_container->set_background_color(Color{ 0, 0, 0, 89 });
        footer_container->set_border_top_width(1.1f);
        footer_container->set_border_top_color(Color{ 255, 255, 255, 25 });
        footer_container->set_padding(20.0f);
        footer_container->set_gap(20.0f);
        footer_container->set_border_bottom_left_radius(16.0f);
        footer_container->set_border_bottom_right_radius(16.0f);
        {
            Button* configure_button = mod_details_panel->get_configure_button();
            install_mods_button = context.create_element<Button>(footer_container, "Install Mods", recompui::ButtonStyle::Primary);
            install_mods_button->add_pressed_callback([this](){ open_install_dialog(); });

            Element* footer_spacer = context.create_element<Element>(footer_container);
            footer_spacer->set_flex(1.0f, 0.0f);

            refresh_button = context.create_element<Button>(footer_container, "Refresh", recompui::ButtonStyle::Primary);
            refresh_button->add_pressed_callback([this](){ refresh_mods(true); });
            refresh_button->set_nav_manual(NavDirection::Up, mod_tab_id);

            mods_folder_button = context.create_element<Button>(footer_container, "Open Mods Folder", recompui::ButtonStyle::Primary);
            mods_folder_button->add_pressed_callback([this](){ open_mods_folder(); });
            mods_folder_button->set_nav(NavDirection::Up, configure_button);
            mods_folder_button->set_nav_manual(NavDirection::Up, mod_tab_id);
        } // footer_container
    } // this
    
    mod_entry_floating_view = context.create_element<ModEntryView>(this);
    mod_entry_floating_view->set_display(Display::None);
    mod_entry_floating_view->set_position(Position::Absolute);
    mod_entry_floating_view->set_selected(true);

    context.close();

    sub_menu_context = recompui::create_context(zelda64::get_asset_path("config_sub_menu.rml"));
    sub_menu_context.open();
    Rml::ElementDocument* sub_menu_doc = sub_menu_context.get_document();
    Rml::Element* config_sub_menu_generic = sub_menu_doc->GetElementById("config_sub_menu");
    ElementConfigSubMenu* config_sub_menu_element = rmlui_dynamic_cast<ElementConfigSubMenu*>(config_sub_menu_generic);
    config_sub_menu = config_sub_menu_element->get_config_sub_menu_element();
    sub_menu_context.close();

    context.open();
}

ModMenu::~ModMenu() {
}

// Placeholder class until the rest of the UI refactor is finished.

recompui::ModMenu* mod_menu;

void update_mod_list(bool scan_mods) {
    if (mod_menu) {
        recompui::ContextId ui_context = recompui::get_config_context_id();
        bool opened = ui_context.open_if_not_already();

        mod_menu->set_mods_dirty(scan_mods);
        mod_menu->queue_update();

        if (opened) {
            ui_context.close();
        }
    }
}

void process_game_started() {
    if (mod_menu) {
        recompui::ContextId ui_context = recompui::get_config_context_id();
        bool opened = ui_context.open_if_not_already();

        mod_menu->queue_update();

        if (opened) {
            ui_context.close();
        }
    }
}

void set_config_tabset_mod_nav() {
    if (mod_menu) {
        Rml::ElementTabSet* tabset = recompui::get_config_tabset();
        Rml::Element* tabs = recompui::get_child_by_tag(tabset, "tabs");
        if (tabs != nullptr) {
            size_t num_children = tabs->GetNumChildren();
            Element* first_mod_entry = mod_menu->get_first_mod_entry();
            if (first_mod_entry != nullptr) {
                std::string id = "#" + first_mod_entry->get_id();
                for (size_t i = 0; i < num_children; i++) {
                    tabs->GetChild(i)->SetProperty(Rml::PropertyId::NavDown, Rml::Property{ id, Rml::Unit::STRING });
                }
            }
            else {
                for (size_t i = 0; i < num_children; i++) {
                    tabs->GetChild(i)->SetProperty(Rml::PropertyId::NavDown, Rml::Style::Nav::Auto);
                }
            }
        }
    }
}

void focus_mod_configure_button() {
    Element* configure_button = mod_menu->get_mod_configure_button();
    if (configure_button) {
        configure_button->focus();
    }
}

ElementModMenu::ElementModMenu(const Rml::String &tag) : Rml::Element(tag) {
    SetProperty("width", "100%");
    SetProperty("height", "100%");

    recompui::Element this_compat(this);
    recompui::ContextId context = get_current_context();
    mod_menu = context.create_element<ModMenu>(&this_compat);
}

ElementModMenu::~ElementModMenu() {

}

} // namespace recompui
