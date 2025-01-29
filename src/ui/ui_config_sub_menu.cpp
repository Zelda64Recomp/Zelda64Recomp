#include "ui_config_sub_menu.h"

#include <cassert>
#include <string_view>

#include "recomp_ui.h"

namespace recompui {

// ConfigOptionElement


void ConfigOptionElement::process_event(const Event &e) {
    switch (e.type) {
    case EventType::Hover:
        hover_callback(this, std::get<EventHover>(e.variant).active);
        break;
    case EventType::Update:
        break;
    default:
        assert(false && "Unknown event type.");
        break;
    }
}

ConfigOptionElement::ConfigOptionElement(Element *parent) : Element(parent, Events(EventType::Hover)) {
    set_display(Display::Flex);
    set_flex_direction(FlexDirection::Column);
    set_gap(16.0f);
    set_height(100.0f);

    name_label = get_current_context().create_element<Label>(this, LabelStyle::Normal);
}

ConfigOptionElement::~ConfigOptionElement() {

}

void ConfigOptionElement::set_id(std::string_view id) {
    this->id = id;
}

void ConfigOptionElement::set_name(std::string_view name) {
    this->name = name;
    name_label->set_text(name);
}

void ConfigOptionElement::set_description(std::string_view description) {
    this->description = description;
}

void ConfigOptionElement::set_hover_callback(std::function<void(ConfigOptionElement *, bool)> callback) {
    hover_callback = callback;
}

const std::string &ConfigOptionElement::get_description() const {
    return description;
}

// ConfigOptionSlider

void ConfigOptionSlider::slider_value_changed(double v) {
    callback(id, v);
}

ConfigOptionSlider::ConfigOptionSlider(Element *parent, double value, double min_value, double max_value, double step_value, bool percent, std::function<void(const std::string &, double)> callback) : ConfigOptionElement(parent) {
    this->callback = callback;

    slider = get_current_context().create_element<Slider>(this, percent ? SliderType::Percent : SliderType::Double);
    slider->set_min_value(min_value);
    slider->set_max_value(max_value);
    slider->set_step_value(step_value);
    slider->set_value(value);
    slider->add_value_changed_callback(std::bind(&ConfigOptionSlider::slider_value_changed, this, std::placeholders::_1));
}

// ConfigOptionTextInput

void ConfigOptionTextInput::text_changed(const std::string &text) {
    callback(id, text);
}

ConfigOptionTextInput::ConfigOptionTextInput(Element *parent, std::string_view value, std::function<void(const std::string &, const std::string &)> callback) : ConfigOptionElement(parent) {
    this->callback = callback;

    text_input = get_current_context().create_element<TextInput>(this);
    text_input->set_text(value);
    text_input->add_text_changed_callback(std::bind(&ConfigOptionTextInput::text_changed, this, std::placeholders::_1));
}

// ConfigOptionRadio

void ConfigOptionRadio::index_changed(uint32_t index) {
    callback(id, index);
}

ConfigOptionRadio::ConfigOptionRadio(Element *parent, uint32_t value, const std::vector<std::string> &options, std::function<void(const std::string &, uint32_t)> callback) : ConfigOptionElement(parent) {
    this->callback = callback;

    radio = get_current_context().create_element<Radio>(this);
    radio->add_index_changed_callback(std::bind(&ConfigOptionRadio::index_changed, this, std::placeholders::_1));
    for (std::string_view option : options) {
        radio->add_option(option);
    }

    if (value < options.size()) {
        radio->set_index(value);
    }
}

// ConfigSubMenu

void ConfigSubMenu::back_button_pressed() {
    // Hide the config sub menu and show the config menu.
    ContextId config_context = recompui::get_config_context_id();
    ContextId sub_menu_context = recompui::get_config_sub_menu_context_id();

    recompui::hide_context(sub_menu_context);
    recompui::show_context(config_context, "");
}

void ConfigSubMenu::option_hovered(ConfigOptionElement *option, bool active) {
    if (active) {
        hover_option_elements.emplace(option);
    }
    else {
        hover_option_elements.erase(option);
    }

    if (hover_option_elements.empty()) {
        description_label->set_text("");
    }
    else {
        description_label->set_text((*hover_option_elements.begin())->get_description());
    }
}

ConfigSubMenu::ConfigSubMenu(Element *parent) : Element(parent) {
    using namespace std::string_view_literals;

    set_display(Display::Flex);
    set_flex(1, 1, 100.0f, Unit::Percent);
    set_flex_direction(FlexDirection::Column);
    set_height(100.0f, Unit::Percent);

    recompui::ContextId context = get_current_context();
    header_container = context.create_element<Container>(this, FlexDirection::Row, JustifyContent::FlexStart);
    header_container->set_flex_grow(0.0f);
    header_container->set_align_items(AlignItems::Center);
    header_container->set_padding_left(12.0f);
    header_container->set_gap(24.0f);

    {
        back_button = context.create_element<Button>(header_container, "Back", ButtonStyle::Secondary);
        back_button->add_pressed_callback(std::bind(&ConfigSubMenu::back_button_pressed, this));
        title_label = context.create_element<Label>(header_container, "Title", LabelStyle::Large);
    }

    body_container = context.create_element<Container>(this, FlexDirection::Row, JustifyContent::SpaceEvenly);
    body_container->set_padding(32.0f);
    {
        config_container = context.create_element<Container>(body_container, FlexDirection::Column, JustifyContent::Center);
        config_container->set_display(Display::Block);
        config_container->set_flex_basis(100.0f);
        config_container->set_align_items(AlignItems::Center);
        {
            config_scroll_container = context.create_element<ScrollContainer>(config_container, ScrollDirection::Vertical);
        }

        description_label = context.create_element<Label>(body_container, "Description", LabelStyle::Small);
        description_label->set_min_width(800.0f);
    }
}

ConfigSubMenu::~ConfigSubMenu() {

}

void ConfigSubMenu::enter(std::string_view title) {
    title_label->set_text(title);
}

void ConfigSubMenu::clear_options() {
    config_scroll_container->clear_children();
    config_option_elements.clear();
    hover_option_elements.clear();
}

void ConfigSubMenu::add_option(ConfigOptionElement *option, std::string_view id, std::string_view name, std::string_view description) {
    option->set_id(id);
    option->set_name(name);
    option->set_description(description);
    option->set_hover_callback(std::bind(&ConfigSubMenu::option_hovered, this, std::placeholders::_1, std::placeholders::_2));
    config_option_elements.emplace_back(option);
}

void ConfigSubMenu::add_slider_option(std::string_view id, std::string_view name, std::string_view description, double value, double min, double max, double step, bool percent, std::function<void(const std::string &, double)> callback) {
    ConfigOptionSlider *option_slider = get_current_context().create_element<ConfigOptionSlider>(config_scroll_container, value, min, max, step, percent, callback);
    add_option(option_slider, id, name, description);
}

void ConfigSubMenu::add_text_option(std::string_view id, std::string_view name, std::string_view description, std::string_view value, std::function<void(const std::string &, const std::string &)> callback) {
    ConfigOptionTextInput *option_text_input = get_current_context().create_element<ConfigOptionTextInput>(config_scroll_container, value, callback);
    add_option(option_text_input, id, name, description);
}

void ConfigSubMenu::add_radio_option(std::string_view id, std::string_view name, std::string_view description, uint32_t value, const std::vector<std::string> &options, std::function<void(const std::string &, uint32_t)> callback) {
    ConfigOptionRadio *option_radio = get_current_context().create_element<ConfigOptionRadio>(config_scroll_container, value, options, callback);
    add_option(option_radio, id, name, description);
}

// ElementConfigSubMenu

ElementConfigSubMenu::ElementConfigSubMenu(const Rml::String &tag) : Rml::Element(tag) {
    SetProperty(Rml::PropertyId::Display, Rml::Style::Display::Flex);
    SetProperty("width", "100%");
    SetProperty("height", "100%");

    recompui::Element this_compat(this);
    recompui::ContextId context = get_current_context();
    config_sub_menu = context.create_element<ConfigSubMenu>(&this_compat);
}

ElementConfigSubMenu::~ElementConfigSubMenu() {

}

void ElementConfigSubMenu::set_display(bool display) {
    SetProperty(Rml::PropertyId::Display, display ? Rml::Style::Display::Block : Rml::Style::Display::None);
}

ConfigSubMenu *ElementConfigSubMenu::get_config_sub_menu_element() const {
    return config_sub_menu;
}

}