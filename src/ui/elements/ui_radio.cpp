#include "ui_radio.h"

namespace recompui {

    // RadioOption

    RadioOption::RadioOption(Element *parent, std::string_view name, uint32_t index) : Element(parent, Events(EventType::Click, EventType::Focus, EventType::Hover, EventType::Enable), "label") {
        this->index = index;

        set_text(name);
        set_cursor(Cursor::Pointer);
        set_font_size(20.0f);
        set_letter_spacing(2.8f);
        set_line_height(20.0f);
        set_font_weight(400);
        set_font_style(FontStyle::Normal);
        set_border_color(Color{ 242, 242, 242, 255 });
        set_border_bottom_width(0.0f);
        set_color(Color{ 255, 255, 255, 153 });
        set_padding_bottom(8.0f);
        set_text_transform(TextTransform::Uppercase);
        hover_style.set_color(Color{ 255, 255, 255, 204 });
        checked_style.set_color(Color{ 255, 255, 255, 255 });
        checked_style.set_border_bottom_width(1.0f);

        add_style(&hover_style, { hover_state });
        add_style(&checked_style, { checked_state });
    }

    void RadioOption::set_pressed_callback(std::function<void(uint32_t)> callback) {
        pressed_callback = callback;
    }

    void RadioOption::set_selected_state(bool enable) {
        set_style_enabled(checked_state, enable);
    }

    void RadioOption::process_event(const Event &e) {
        switch (e.type) {
        case EventType::Click:
            pressed_callback(index);
            break;
        case EventType::Hover:
            set_style_enabled(hover_state, std::get<EventHover>(e.variant).active);
            break;
        case EventType::Enable:
            set_style_enabled(disabled_state, !std::get<EventEnable>(e.variant).active);
            break;
        default:
            break;
        }
    }

    // Radio

    void Radio::set_index_internal(uint32_t index, bool setup, bool trigger_callbacks) {
        if (this->index != index || setup) {
            options[this->index]->set_selected_state(false);
            this->index = index;
            options[index]->set_selected_state(true);

            if (trigger_callbacks) {
                for (const auto &function : index_changed_callbacks) {
                    function(index);
                }
            }
        }
    }

    void Radio::option_selected(uint32_t index) {
        set_index_internal(index, false, true);
    }

    Radio::Radio(Element *parent) : Container(parent, FlexDirection::Row, JustifyContent::FlexStart) {
        set_gap(24.0f);
        set_flex_grow(0.0f);
    }

    Radio::~Radio() {

    }

    void Radio::add_option(std::string_view name) {
        RadioOption *option = get_current_context().create_element<RadioOption>(this, name, uint32_t(options.size()));
        option->set_pressed_callback(std::bind(&Radio::option_selected, this, std::placeholders::_1));
        options.emplace_back(option);

        // The first option was added, select it.
        if (options.size() == 1) {
            set_index_internal(0, true, false);
        }
    }

    void Radio::set_index(uint32_t index) {
        set_index_internal(index, false, false);
    }

    uint32_t Radio::get_index() const {
        return index;
    }

    void Radio::add_index_changed_callback(std::function<void(uint32_t)> callback) {
        index_changed_callbacks.emplace_back(callback);
    }

};