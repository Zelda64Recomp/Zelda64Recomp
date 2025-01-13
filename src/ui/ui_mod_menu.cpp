#include "ui_mod_menu.h"

#include "elements/presets.h"
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

    {
        thumbnail_image = new Image(this);
        thumbnail_image->set_width(100.0f);
        thumbnail_image->set_height(100.0f);
        thumbnail_image->set_min_width(100.0f);
        thumbnail_image->set_min_height(100.0f);
        thumbnail_image->set_background_color(Color{ 190, 184, 219, 25 });

        body_container = new Container(FlexDirection::Column, JustifyContent::FlexStart, this);
        body_container->set_width_auto();
        body_container->set_height(100.0f);
        body_container->set_margin_left(16.0f);
        body_container->set_overflow(Overflow::Hidden);

        {
            name_label = new Label(details.mod_id, LabelStyle::Normal, body_container);
            description_label = new Label("Short description of mod here.", LabelStyle::Small, body_container);
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

void ModMenu::set_active_mod(uint32_t mod_index) {
    mod_details_panel->set_mod_details(mod_details[mod_index]);
}

void ModMenu::refresh_mods() {
    recomp::mods::scan_mods();
    mod_details = recomp::mods::get_mod_details(game_mod_id);
    create_mod_list();
}

void ModMenu::create_mod_list() {
    // Clear the contents of the list scroll.
    list_scroll_container->clear_children();
    mod_entries.clear();

    // Create the child elements for the list scroll.
    for (size_t mod_index = 0; mod_index < mod_details.size(); mod_index++) {
        mod_entries.emplace_back(new ModEntry(list_scroll_container, mod_details[mod_index], mod_index, this));
    }

    set_active_mod(0);
}

ModMenu::ModMenu(Element *parent) : Element(parent) {
    game_mod_id = "mm";

    set_display(Display::Flex);
    set_flex(1.0f, 1.0f, 100.0f);
    set_flex_direction(FlexDirection::Column);
    set_align_items(AlignItems::Center);
    set_justify_content(JustifyContent::FlexStart);
    set_width(100.0f, Unit::Percent);
    set_height(100.0f, Unit::Percent);
    
    {
        body_container = new Container(FlexDirection::Row, JustifyContent::FlexStart, this);
        body_container->set_flex(1.0f, 1.0f, 100.0f);
        body_container->set_width(100.0f, Unit::Percent);
        body_container->set_height(100.0f, Unit::Percent);
        {
            list_container = new Container(FlexDirection::Column, JustifyContent::Center, body_container);
            list_container->set_display(Display::Block);
            list_container->set_flex_basis_percentage(100.0f);
            list_container->set_align_items(AlignItems::Center);
            list_container->set_height(100.0f, Unit::Percent);
            list_container->set_background_color(Color{ 0, 0, 0, 89 });
            list_container->set_border_bottom_left_radius(16.0f);
            {
                list_scroll_container = new ScrollContainer(ScrollDirection::Vertical, list_container);
            } // list_container

            mod_details_panel = new ModDetailsPanel(body_container);
        } // body_container
        

        footer_container = new Container(FlexDirection::Row, JustifyContent::SpaceBetween, this);
        footer_container->set_width(100.0f, recompui::Unit::Percent);
        footer_container->set_align_items(recompui::AlignItems::Center);
        footer_container->set_background_color(Color{ 0, 0, 0, 89 });
        footer_container->set_border_top_width(1.1f);
        footer_container->set_border_top_color(Color{ 255, 255, 255, 25 });
        footer_container->set_padding(20.0f);
        footer_container->set_border_bottom_left_radius(16.0f);
        footer_container->set_border_bottom_right_radius(16.0f);
        {
            refresh_button = new Button("Refresh", recompui::ButtonStyle::Primary, footer_container);
            refresh_button->add_pressed_callback(std::bind(&ModMenu::refresh_mods, this));
        } // footer_container
    } // this

    refresh_mods();
}

ModMenu::~ModMenu() {
}

// Placeholder class until the rest of the UI refactor is finished.

ElementModMenu::ElementModMenu(const Rml::String &tag) : Rml::Element(tag) {
    SetProperty("width", "100%");
    SetProperty("height", "100%");

    recompui::Element this_compat(this);
    mod_menu = std::make_unique<ModMenu>(&this_compat);
}

ElementModMenu::~ElementModMenu() {

}

} // namespace recompui
