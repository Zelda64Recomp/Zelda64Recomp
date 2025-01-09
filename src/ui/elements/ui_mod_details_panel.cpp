#include "ui_mod_details_panel.h"

#include "presets.h"
#include "librecomp/mods.hpp"

namespace recompui {

ModDetailsPanel::ModDetailsPanel(Element *parent) : Element(parent) {
    set_flex(1.0f, 1.0f, 200.0f);
    set_height(100.0f, Unit::Percent);
    set_flex_direction(FlexDirection::Column);
    set_border_bottom_right_radius(16.0f);
    set_background_color(Color{ 190, 184, 219, 25 });

    header_container = std::make_unique<recompui::Container>(FlexDirection::Row, JustifyContent::FlexStart, this);
    header_container->set_padding(16.0f);
    header_container->set_background_color(Color{ 0, 0, 0, 89 });
    {
        thumbnail_container = std::make_unique<recompui::Container>(FlexDirection::Column, JustifyContent::SpaceEvenly, header_container.get());
        {
            thumbnail_image = std::make_unique<recompui::Image>(thumbnail_container.get());
            thumbnail_image->set_width(100.0f);
            thumbnail_image->set_height(100.0f);
            thumbnail_image->set_background_color(Color{ 190, 184, 219, 25 });
        }

        header_details_container = std::make_unique<recompui::Container>(FlexDirection::Column, JustifyContent::SpaceEvenly, header_container.get());
        header_details_container->set_width(100.0f, Unit::Percent);
        header_details_container->set_margin_left(32.0f);
        header_details_container->set_overflow(Overflow::Hidden);

        {
            title_label = std::make_unique<recompui::Label>(LabelStyle::Large, header_details_container.get());
            version_label = std::make_unique<recompui::Label>(LabelStyle::Normal, header_details_container.get());
        }
    }

    body_container = std::make_unique<recompui::Container>(FlexDirection::Column, JustifyContent::FlexStart, this);
    body_container->set_padding_left(16.0f);
    {
        description_label = std::make_unique<recompui::Label>(LabelStyle::Normal, body_container.get());
        authors_label = std::make_unique<recompui::Label>(LabelStyle::Normal, body_container.get());
        buttons_container = std::make_unique<recompui::Container>(FlexDirection::Row, JustifyContent::SpaceAround, body_container.get());
        buttons_container->set_padding_left(16.0f);
        {
            enable_toggle = std::make_unique<recompui::Toggle>(buttons_container.get());
            configure_button = std::make_unique<recompui::Button>("Configure", recompui::ButtonStyle::Secondary, buttons_container.get());
            erase_button = std::make_unique<recompui::Button>("Erase", recompui::ButtonStyle::Secondary, buttons_container.get());
        }
    }
}

ModDetailsPanel::~ModDetailsPanel() {
}

void ModDetailsPanel::set_mod_details(const recomp::mods::ModDetails& details) {
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
}

} // namespace recompui
