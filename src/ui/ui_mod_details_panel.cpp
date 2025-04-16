#include "ui_mod_details_panel.h"

#include "librecomp/mods.hpp"

namespace recompui {

extern const std::string mod_tab_id;

ModDetailsPanel::ModDetailsPanel(Element *parent) : Element(parent) {
    set_flex(1.0f, 1.0f, 200.0f);
    set_height(100.0f, Unit::Percent);
    set_display(Display::Flex);
    set_flex_direction(FlexDirection::Column);
    set_background_color(Color{ 190, 184, 219, 25 });

    ContextId context = get_current_context();

    header_container = context.create_element<Container>(this, FlexDirection::Row, JustifyContent::FlexStart);
    header_container->set_flex(0.0f, 0.0f);
    header_container->set_padding(16.0f);
    header_container->set_gap(16.0f);
    header_container->set_background_color(Color{ 0, 0, 0, 89 });
    header_container->set_border_bottom_width(1.1f);
    header_container->set_border_bottom_color(Color{ 255, 255, 255, 25 });
    {
        thumbnail_container = context.create_element<Container>(header_container, FlexDirection::Column, JustifyContent::SpaceEvenly);
        thumbnail_container->set_flex(0.0f, 0.0f);
        {
            thumbnail_image = context.create_element<Image>(thumbnail_container, "");
            thumbnail_image->set_width(100.0f);
            thumbnail_image->set_height(100.0f);
            thumbnail_image->set_background_color(Color{ 190, 184, 219, 25 });
        }

        header_details_container = context.create_element<Container>(header_container, FlexDirection::Column, JustifyContent::SpaceEvenly);
        header_details_container->set_flex(1.0f, 1.0f);
        header_details_container->set_flex_basis(100.0f, Unit::Percent);
        header_details_container->set_text_align(TextAlign::Left);
        {
            title_label = context.create_element<Label>(header_details_container, LabelStyle::Large);
            version_label = context.create_element<Label>(header_details_container, LabelStyle::Normal);
        }
    }

    body_container = context.create_element<ScrollContainer>(this, ScrollDirection::Vertical);
    body_container->set_text_align(TextAlign::Left);
    body_container->set_padding(16.0f);
    {
        authors_label = context.create_element<Label>(body_container, LabelStyle::Normal);
        authors_label->set_margin_bottom(16.0f);
        description_label = context.create_element<Label>(body_container, LabelStyle::Normal);
    }
    
    buttons_container = context.create_element<Container>(this, FlexDirection::Row, JustifyContent::SpaceAround);
    buttons_container->set_flex(0.0f, 0.0f);
    buttons_container->set_padding(16.0f);
    buttons_container->set_justify_content(JustifyContent::SpaceBetween);
    buttons_container->set_border_top_width(1.1f);
    buttons_container->set_border_top_color(Color{ 255, 255, 255, 25 });
    buttons_container->set_background_color(Color{ 0, 0, 0, 89 });
    {
        enable_container = context.create_element<Container>(buttons_container, FlexDirection::Row, JustifyContent::FlexStart);
        enable_container->set_align_items(AlignItems::Center);
        enable_container->set_gap(16.0f);
        {
            enable_toggle = context.create_element<Toggle>(enable_container);
            enable_toggle->add_checked_callback([this](bool checked){ enable_toggle_checked(checked); });
            enable_toggle->set_nav_manual(NavDirection::Up, mod_tab_id);

            enable_label = context.create_element<Label>(enable_container, "A currently enabled mod requires this mod", LabelStyle::Annotation);
        }

        configure_button = context.create_element<Button>(buttons_container, "Configure", recompui::ButtonStyle::Secondary);
        configure_button->add_pressed_callback([this](){ configure_button_pressed(); });
        configure_button->set_nav_manual(NavDirection::Up, mod_tab_id);
    }
    clear_mod_navigation();
}

ModDetailsPanel::~ModDetailsPanel() {
}

void ModDetailsPanel::disable_toggle() {
    enable_toggle->set_enabled(false);
}

void ModDetailsPanel::set_mod_details(const recomp::mods::ModDetails& details, const std::string &thumbnail, bool toggle_checked, bool toggle_enabled, bool toggle_label_visible, bool configure_enabled) {
    cur_details = details;

    thumbnail_image->set_src(thumbnail);

    title_label->set_text(cur_details.display_name);
    version_label->set_text(cur_details.version.to_string());

    std::string authors_str = "Authors:";
    bool first = true;
    for (const std::string& author : details.authors) {
        authors_str += (first ? " " : ", ") + author;
        first = false;
    }

    authors_label->set_text(authors_str);
    description_label->set_text(cur_details.description);
    enable_toggle->set_checked(toggle_checked);
    enable_toggle->set_enabled(toggle_enabled);
    configure_button->set_enabled(configure_enabled);
    enable_label->set_display(toggle_label_visible ? Display::Block : Display::None);

    if (configure_enabled) {
        enable_toggle->set_nav(NavDirection::Right, configure_button);
    }
    else {
        enable_toggle->set_nav_none(NavDirection::Right);
    }
}

void ModDetailsPanel::set_mod_toggled_callback(std::function<void(bool)> callback) {
    mod_toggled_callback = callback;
}

void ModDetailsPanel::set_mod_configure_pressed_callback(std::function<void()> callback) {
    mod_configure_pressed_callback = callback;
}

void ModDetailsPanel::setup_mod_navigation(Element* nav_target) {
    enable_toggle->set_nav(NavDirection::Left, nav_target);

    if (enable_toggle->is_enabled()) {
        configure_button->set_nav(NavDirection::Left, enable_toggle);
    }
    else {
        configure_button->set_nav(NavDirection::Left, nav_target);
    }
}

void ModDetailsPanel::clear_mod_navigation() {
    enable_toggle->set_nav_none(NavDirection::Left);
    configure_button->set_nav_none(NavDirection::Left);
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
