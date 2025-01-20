#include "ui_config_sub_menu.h"

#include <cassert>

namespace recompui {

// ConfigOptionElement


void ConfigOptionElement::process_event(const Event &e) {
    switch (e.type) {
    case EventType::Hover:
        hover_callback(this, std::get<EventHover>(e.variant).active);
        break;
    default:
        assert(false && "Unknown event type.");
        break;
    }
}

ConfigOptionElement::ConfigOptionElement(Element *parent) : Element(parent, Events(EventType::Hover)) {
    set_gap(8.0f);
    set_min_height(100.0f);

    name_label = get_current_context().create_element<Label>(LabelStyle::Normal, this);
}

ConfigOptionElement::~ConfigOptionElement() {

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
    // TODO: Hook up to whatever API Recomp exposes to set the value of the persisent configuration in mods.
    printf("%s changed to %f.\n", name.c_str(), v);
}

ConfigOptionSlider::ConfigOptionSlider(double value, double min_value, double max_value, double step_value, bool percent, Element *parent) : ConfigOptionElement(parent) {
    slider = get_current_context().create_element<Slider>(percent ? SliderType::Percent : SliderType::Double, this);
    slider->set_value(value);
    slider->set_min_value(min_value);
    slider->set_max_value(max_value);
    slider->set_step_value(step_value);
    slider->add_value_changed_callback(std::bind(&ConfigOptionSlider::slider_value_changed, this, std::placeholders::_1));
}

// ConfigOptionTextInput

void ConfigOptionTextInput::text_changed(const std::string &text) {
    // TODO: Hook up to whatever API Recomp exposes to set the value of the persisent configuration in mods.
    printf("%s changed to %s.\n", name.c_str(), text.c_str());
}

ConfigOptionTextInput::ConfigOptionTextInput(Element *parent) : ConfigOptionElement(parent) {
    text_input = get_current_context().create_element<TextInput>(this);
    text_input->add_text_changed_callback(std::bind(&ConfigOptionTextInput::text_changed, this, std::placeholders::_1));
}

// ConfigOptionRadio

void ConfigOptionRadio::index_changed(uint32_t index) {
    printf("%s changed to %d.\n", name.c_str(), index);
}

ConfigOptionRadio::ConfigOptionRadio(const std::vector<std::string> &options, Element *parent) : ConfigOptionElement(parent) {
    radio = get_current_context().create_element<Radio>(this);
    radio->add_index_changed_callback(std::bind(&ConfigOptionRadio::index_changed, this, std::placeholders::_1));
    for (std::string_view option : options) {
        radio->add_option(option);
    }
}

// ConfigSubMenu

void ConfigSubMenu::back_button_pressed() {
    if (quit_sub_menu_callback != nullptr) {
        quit_sub_menu_callback();
    }
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
    set_display(Display::Flex);
    set_flex(1, 1, 100.0f, Unit::Percent);
    set_flex_direction(FlexDirection::Column);
    set_height(100.0f, Unit::Percent);

    recompui::ContextId context = get_current_context();
    header_container = context.create_element<Container>(FlexDirection::Row, JustifyContent::FlexStart, this);

    {
        back_button = context.create_element<Button>("Back", ButtonStyle::Secondary, header_container);
        back_button->add_pressed_callback(std::bind(&ConfigSubMenu::back_button_pressed, this));
        title_label = context.create_element<Label>("Title", LabelStyle::Large, header_container);
    }

    body_container = context.create_element<Container>(FlexDirection::Row, JustifyContent::SpaceEvenly, this);
    {
        config_container = context.create_element<Container>(FlexDirection::Column, JustifyContent::Center, body_container);
        config_container->set_display(Display::Block);
        config_container->set_flex_basis(100.0f);
        config_container->set_align_items(AlignItems::Center);
        {
            config_scroll_container = context.create_element<ScrollContainer>(ScrollDirection::Vertical, config_container);
        }

        description_label = context.create_element<Label>("Description", LabelStyle::Small, body_container);
        description_label->set_min_width(800.0f);
    }
}

ConfigSubMenu::~ConfigSubMenu() {

}

void ConfigSubMenu::enter(std::string_view title) {
    title_label->set_text(title);

    if (enter_sub_menu_callback != nullptr) {
        enter_sub_menu_callback();
    }
}

void ConfigSubMenu::clear_options() {
    config_scroll_container->clear_children();
    config_option_elements.clear();
    hover_option_elements.clear();
}

void ConfigSubMenu::add_option(ConfigOptionElement *option, std::string_view name, std::string_view description) {
    option->set_name(name);
    option->set_description(description);
    option->set_hover_callback(std::bind(&ConfigSubMenu::option_hovered, this, std::placeholders::_1, std::placeholders::_2));
    config_option_elements.emplace_back(option);
}

void ConfigSubMenu::add_slider_option(std::string_view name, std::string_view description, double min, double max, double step, bool percent) {
    ConfigOptionSlider *option_slider = get_current_context().create_element<ConfigOptionSlider>((min + max) / 2.0, min, max, step, percent, config_scroll_container);
    add_option(option_slider, name, description);
}

void ConfigSubMenu::add_text_option(std::string_view name, std::string_view description) {
    ConfigOptionTextInput *option_text_input = get_current_context().create_element<ConfigOptionTextInput>(config_scroll_container);
    add_option(option_text_input, name, description);
}

void ConfigSubMenu::add_radio_option(std::string_view name, std::string_view description, const std::vector<std::string> &options) {
    ConfigOptionRadio *option_radio = get_current_context().create_element<ConfigOptionRadio>(options, config_scroll_container);
    add_option(option_radio, name, description);
}

void ConfigSubMenu::set_enter_sub_menu_callback(std::function<void()> callback) {
    enter_sub_menu_callback = callback;
}

void ConfigSubMenu::set_quit_sub_menu_callback(std::function<void()> callback) {
    quit_sub_menu_callback = callback;
}

// ElementConfigSubMenu

ElementConfigSubMenu::ElementConfigSubMenu(const Rml::String &tag) : Rml::Element(tag) {
    SetProperty(Rml::PropertyId::Display, Rml::Style::Display::None);
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

void ElementConfigSubMenu::set_enter_sub_menu_callback(std::function<void()> callback) {
    config_sub_menu->set_enter_sub_menu_callback(callback);
}

void ElementConfigSubMenu::set_quit_sub_menu_callback(std::function<void()> callback) {
    config_sub_menu->set_quit_sub_menu_callback(callback);
}

ConfigSubMenu *ElementConfigSubMenu::get_config_sub_menu_element() const {
    return config_sub_menu;
}

}