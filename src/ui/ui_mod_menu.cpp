#include "ui_mod_menu.h"
#include "recomp_ui.h"

#include "librecomp/mods.hpp"

#include <string>

// TODO:
// - Set up navigation.
// - Add hover and active state for mod entries.

namespace recompui {

ModEntry::ModEntry(Element *parent, const recomp::mods::ModDetails &details, uint32_t mod_index, ModMenu *mod_menu) : Element(parent, Events(EventType::Click, EventType::Hover, EventType::Focus)) {
    assert(mod_menu != nullptr);

    this->mod_index = mod_index;
    this->mod_menu = mod_menu;

    set_display(Display::Flex);
    set_flex_direction(FlexDirection::Row);
    set_width(100.0f, Unit::Percent);
    set_height_auto();
    set_padding_top(4.0f);
    set_padding_right(8.0f);
    set_padding_bottom(4.0f);
    set_padding_left(8.0f);
    set_border_width(1.1f);
    set_border_color(Color{ 242, 242, 242, 204 });
    set_background_color(Color{ 242, 242, 242, 12 });
    set_cursor(Cursor::Pointer);

    ContextId context = get_current_context();

    {
        thumbnail_image = context.create_element<Image>(this);
        thumbnail_image->set_width(100.0f);
        thumbnail_image->set_height(100.0f);
        thumbnail_image->set_min_width(100.0f);
        thumbnail_image->set_min_height(100.0f);
        thumbnail_image->set_background_color(Color{ 190, 184, 219, 25 });

        body_container = context.create_element<Container>(this, FlexDirection::Column, JustifyContent::FlexStart);
        body_container->set_width_auto();
        body_container->set_height(100.0f);
        body_container->set_margin_left(16.0f);
        body_container->set_overflow(Overflow::Hidden);

        {
            name_label = context.create_element<Label>(body_container, details.mod_id, LabelStyle::Normal);
            description_label = context.create_element<Label>(body_container, "Short description of mod here.", LabelStyle::Small);
        } // body_container
    } // this
}

ModEntry::~ModEntry() {

}

void ModEntry::process_event(const Event& e) {
    switch (e.type) {
    case EventType::Click:
        mod_menu->set_active_mod(mod_index);
        break;
    case EventType::Hover:
        break;
    case EventType::Focus:
        break;
    default:
        break;
    }
}

void ModMenu::set_active_mod(int32_t mod_index) {
    active_mod_index = mod_index;
    if (active_mod_index >= 0) {
        bool mod_enabled = recomp::mods::is_mod_enabled(mod_details[mod_index].mod_id);
        bool auto_enabled = recomp::mods::is_mod_auto_enabled(mod_details[mod_index].mod_id);
        bool toggle_enabled = !auto_enabled && (mod_details[mod_index].runtime_toggleable || !ultramodern::is_game_started());
        mod_details_panel->set_mod_details(mod_details[mod_index], mod_enabled, toggle_enabled);
    }
}

void ModMenu::refresh_mods() {
    recomp::mods::scan_mods();
    mod_details = recomp::mods::get_mod_details(game_mod_id);
    create_mod_list();
}

void ModMenu::mod_toggled(bool enabled) {
    if (active_mod_index >= 0) {
        recomp::mods::enable_mod(mod_details[active_mod_index].mod_id, enabled);
    }
}

// TODO remove this once this is migrated to the new system.
ContextId sub_menu_context;

ContextId get_config_sub_menu_context_id() {
    return sub_menu_context;
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
            switch (option.type) {
            case recomp::mods::ConfigOptionType::Enum: {
                const recomp::mods::ConfigOptionEnum &option_enum = std::get<recomp::mods::ConfigOptionEnum>(option.variant);
                config_sub_menu->add_radio_option(option.name, option.description, option_enum.options);
                break;
            }
            case recomp::mods::ConfigOptionType::Number: {
                const recomp::mods::ConfigOptionNumber &option_number = std::get<recomp::mods::ConfigOptionNumber>(option.variant);
                config_sub_menu->add_slider_option(option.name, option.description, option_number.min, option_number.max, option_number.step, option_number.percent);
                break;
            }
            case recomp::mods::ConfigOptionType::String: {
                config_sub_menu->add_text_option(option.name, option.description);
                break;
            }
            default:
                assert(false && "Unknown config option type.");
                break;
            }
        }

        config_sub_menu->enter(mod_details[active_mod_index].mod_id);
        sub_menu_context.close();

        // Reopen the context that was open when this function was called.
        prev_context.open();
        
        // Hide the config menu and show the sub menu.
        recompui::hide_context(recompui::get_config_context_id());
        recompui::show_context(sub_menu_context, "");
    }
}

void ModMenu::create_mod_list() {
    ContextId context = get_current_context();

    // Clear the contents of the list scroll.
    list_scroll_container->clear_children();
    mod_entries.clear();

    // Create the child elements for the list scroll.
    for (size_t mod_index = 0; mod_index < mod_details.size(); mod_index++) {
        mod_entries.emplace_back(context.create_element<ModEntry>(list_scroll_container, mod_details[mod_index], mod_index, this));
    }

    set_active_mod(0);
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
            mod_details_panel->set_mod_toggled_callback(std::bind(&ModMenu::mod_toggled, this, std::placeholders::_1));
            mod_details_panel->set_mod_configure_pressed_callback(std::bind(&ModMenu::mod_configure_requested, this));
        } // body_container
        

        footer_container = context.create_element<Container>(this, FlexDirection::Row, JustifyContent::SpaceBetween);
        footer_container->set_width(100.0f, recompui::Unit::Percent);
        footer_container->set_align_items(recompui::AlignItems::Center);
        footer_container->set_background_color(Color{ 0, 0, 0, 89 });
        footer_container->set_border_top_width(1.1f);
        footer_container->set_border_top_color(Color{ 255, 255, 255, 25 });
        footer_container->set_padding(20.0f);
        footer_container->set_border_bottom_left_radius(16.0f);
        footer_container->set_border_bottom_right_radius(16.0f);
        {
            refresh_button = context.create_element<Button>(footer_container, "Refresh", recompui::ButtonStyle::Primary);
            refresh_button->add_pressed_callback(std::bind(&ModMenu::refresh_mods, this));
        } // footer_container
    } // this

    refresh_mods();

    context.close();

    sub_menu_context = recompui::create_context("assets/config_sub_menu.rml");
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
