#include "ui_config_sub_menu.h"

#include <cassert>
#include <string_view>

#include "recomp_ui.h"

namespace recompui {

// ConfigOptionElement


void ConfigOptionElement::process_event(const Event &e) {
    switch (e.type) {
    case EventType::Hover:
        if (hover_callback == nullptr) {
            break;
        }
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

void ConfigOptionElement::set_option_id(std::string_view id) {
    this->option_id = id;
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

void ConfigOptionElement::set_focus_callback(std::function<void(const std::string &, bool)> callback) {
    focus_callback = callback;
}

const std::string &ConfigOptionElement::get_description() const {
    return description;
}

// ConfigOptionSlider

void ConfigOptionSlider::slider_value_changed(double v) {
    callback(option_id, v);
}

ConfigOptionSlider::ConfigOptionSlider(Element *parent, double value, double min_value, double max_value, double step_value, bool percent, std::function<void(const std::string &, double)> callback) : ConfigOptionElement(parent) {
    this->callback = callback;

    slider = get_current_context().create_element<Slider>(this, percent ? SliderType::Percent : SliderType::Double);
    slider->set_max_width(380.0f);
    slider->set_min_value(min_value);
    slider->set_max_value(max_value);
    slider->set_step_value(step_value);
    slider->set_value(value);
    slider->add_value_changed_callback([this](double v){ slider_value_changed(v); });
    slider->set_focus_callback([this](bool active) {
        focus_callback(option_id, active);
    });
}

// ConfigOptionTextInput

void ConfigOptionTextInput::text_changed(const std::string &text) {
    callback(option_id, text);
}

ConfigOptionTextInput::ConfigOptionTextInput(Element *parent, std::string_view value, std::function<void(const std::string &, const std::string &)> callback) : ConfigOptionElement(parent) {
    this->callback = callback;

    text_input = get_current_context().create_element<TextInput>(this);
    text_input->set_max_width(400.0f);
    text_input->set_text(value);
    text_input->add_text_changed_callback([this](const std::string &text){ text_changed(text); });
    text_input->set_focus_callback([this](bool active) {
        focus_callback(option_id, active);
    });
}

// ConfigOptionRadio

void ConfigOptionRadio::index_changed(uint32_t index) {
    callback(option_id, index);
}

ConfigOptionRadio::ConfigOptionRadio(Element *parent, uint32_t value, const std::vector<std::string> &options, std::function<void(const std::string &, uint32_t)> callback) : ConfigOptionElement(parent) {
    this->callback = callback;

    radio = get_current_context().create_element<Radio>(this);
    radio->set_focus_callback([this](bool active) {
        focus_callback(option_id, active);
    });
    radio->add_index_changed_callback([this](uint32_t index){ index_changed(index); });
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
    recompui::focus_mod_configure_button();
}

void ConfigSubMenu::set_description_option_element(ConfigOptionElement *option, bool active) {
    if (active) {
        description_option_element = option;
    }
    else if (description_option_element == option) {
        description_option_element = nullptr;
    }

    if (description_option_element == nullptr) {
        description_label->set_text("");
    }
    else {
        description_label->set_text(description_option_element->get_description());
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
    header_container->set_padding(12.0f);
    header_container->set_gap(24.0f);

    {
        back_button = context.create_element<Button>(header_container, "Back", ButtonStyle::Secondary);
        back_button->add_pressed_callback([this](){ back_button_pressed(); });
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

        description_label = context.create_element<Label>(body_container, "", LabelStyle::Small);
        description_label->set_min_width(800.0f);
        description_label->set_padding_left(16.0f);
        description_label->set_padding_right(16.0f);
    }

    recompui::get_current_context().set_autofocus_element(back_button);
}

ConfigSubMenu::~ConfigSubMenu() {

}

void ConfigSubMenu::enter(std::string_view title) {
    title_label->set_text(title);
}

void ConfigSubMenu::clear_options() {
    config_scroll_container->clear_children();
    config_option_elements.clear();
    description_option_element = nullptr;
}

void ConfigSubMenu::add_option(ConfigOptionElement *option, std::string_view id, std::string_view name, std::string_view description) {
    option->set_option_id(id);
    option->set_name(name);
    option->set_description(description);
    option->set_hover_callback([this](ConfigOptionElement *option, bool active){ set_description_option_element(option, active); });
    option->set_focus_callback([this, option](const std::string &id, bool active) { set_description_option_element(option, active); });
    if (config_option_elements.empty()) {
        back_button->set_nav(NavDirection::Down, option->get_focus_element());
        option->set_nav(NavDirection::Up, back_button);
    }
    else {
        config_option_elements.back()->set_nav(NavDirection::Down, option->get_focus_element());
        option->set_nav(NavDirection::Up, config_option_elements.back()->get_focus_element());
    }

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
