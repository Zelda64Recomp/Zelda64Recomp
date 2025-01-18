#include "ui_mod_details_panel.h"

#include "librecomp/mods.hpp"

namespace recompui {

ModDetailsPanel::ModDetailsPanel(Element *parent) : Element(parent) {
    set_flex(1.0f, 1.0f, 200.0f);
    set_height(100.0f, Unit::Percent);
    set_display(Display::Flex);
    set_flex_direction(FlexDirection::Column);
    set_border_bottom_right_radius(16.0f);
    set_background_color(Color{ 190, 184, 219, 25 });

    ContextId context = get_current_context();

    header_container = context.create_element<Container>(FlexDirection::Row, JustifyContent::FlexStart, this);
    header_container->set_flex(0.0f, 0.0f);
    header_container->set_padding(16.0f);
    header_container->set_gap(16.0f);
    header_container->set_background_color(Color{ 0, 0, 0, 89 });
    {
        thumbnail_container = context.create_element<Container>(FlexDirection::Column, JustifyContent::SpaceEvenly, header_container);
        thumbnail_container->set_flex(0.0f, 0.0f);
        {
            thumbnail_image = context.create_element<Image>(thumbnail_container);
            thumbnail_image->set_width(100.0f);
            thumbnail_image->set_height(100.0f);
            thumbnail_image->set_background_color(Color{ 190, 184, 219, 25 });
        }

        header_details_container = context.create_element<Container>(FlexDirection::Column, JustifyContent::SpaceEvenly, header_container);
        header_details_container->set_flex(1.0f, 1.0f);
        header_details_container->set_flex_basis(100.0f, Unit::Percent);
        header_details_container->set_text_align(TextAlign::Left);
        {
            title_label = context.create_element<Label>(LabelStyle::Large, header_details_container);
            version_label = context.create_element<Label>(LabelStyle::Normal, header_details_container);
        }
    }

    body_container = context.create_element<Container>(FlexDirection::Column, JustifyContent::FlexStart, this);
    body_container->set_flex(0.0f, 0.0f);
    body_container->set_text_align(TextAlign::Left);
    body_container->set_padding(16.0f);
    body_container->set_gap(16.0f);
    {
        description_label = context.create_element<Label>(LabelStyle::Normal, body_container);
        authors_label = context.create_element<Label>(LabelStyle::Normal, body_container);
    }
    
    spacer_element = context.create_element<Element>(this);
    spacer_element->set_flex(1.0f, 0.0f);
    
    buttons_container = context.create_element<Container>(FlexDirection::Row, JustifyContent::SpaceAround, this);
    buttons_container->set_flex(0.0f, 0.0f);
    buttons_container->set_padding(16.0f);
    {
        enable_toggle = context.create_element<Toggle>(buttons_container);
        enable_toggle->add_checked_callback(std::bind(&ModDetailsPanel::enable_toggle_checked, this, std::placeholders::_1));
        configure_button = context.create_element<Button>("Configure", recompui::ButtonStyle::Secondary, buttons_container);
        configure_button->add_pressed_callback(std::bind(&ModDetailsPanel::configure_button_pressed, this));
        erase_button = context.create_element<Button>("Erase", recompui::ButtonStyle::Secondary, buttons_container);
    }
}

ModDetailsPanel::~ModDetailsPanel() {
}

void ModDetailsPanel::set_mod_details(const recomp::mods::ModDetails& details, bool mod_enabled, bool toggle_enabled) {
    cur_details = details;

    title_label->set_text(cur_details.mod_id);
    version_label->set_text(cur_details.version.to_string());

    std::string authors_str = "<i>Authors</i>:";
    bool first = true;
    for (const std::string& author : details.authors) {
        authors_str += (first ? " " : ", ") + author;
        first = false;
    }

    authors_label->set_text(authors_str);
    description_label->set_text("Placeholder description. Some long text to make sure that wrapping is working correctly. Yet more text and so on.");
    enable_toggle->set_checked(mod_enabled);
    enable_toggle->set_enabled(toggle_enabled);
}

void ModDetailsPanel::set_mod_toggled_callback(std::function<void(bool)> callback) {
    mod_toggled_callback = callback;
}

void ModDetailsPanel::set_mod_configure_pressed_callback(std::function<void()> callback) {
    mod_configure_pressed_callback = callback;
}

void ModDetailsPanel::enable_toggle_checked(bool checked) {
    if (mod_toggled_callback != nullptr) {
        mod_toggled_callback(checked);
    }
}

void ModDetailsPanel::configure_button_pressed() {
    if (mod_configure_pressed_callback != nullptr) {
        mod_configure_pressed_callback();
    }
}

} // namespace recompui
