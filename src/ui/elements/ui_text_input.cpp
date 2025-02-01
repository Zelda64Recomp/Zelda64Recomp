#include "ui_text_input.h"

#include <cassert>

namespace recompui {

    void TextInput::process_event(const Event &e) {
        switch (e.type) {
        case EventType::Text: {
            const EventText &event = std::get<EventText>(e.variant);
            text = event.text;

            for (const auto &function : text_changed_callbacks) {
                function(text);
            }

            break;
        }
        default:
            break;
        }
    }
    
    TextInput::TextInput(Element *parent) : Element(parent, Events(EventType::Text), "input") {
        set_min_width(60.0f);
        set_max_width(400.0f);
        set_border_color(Color{ 242, 242, 242, 255 });
        set_border_bottom_width(1.0f);
        set_padding_bottom(6.0f);
    }

    void TextInput::set_text(std::string_view text) {
        this->text = std::string(text);
        set_attribute("value", this->text);
    }

    const std::string &TextInput::get_text() {
        return text;
    }

    void TextInput::add_text_changed_callback(std::function<void(const std::string &)> callback) {
        text_changed_callbacks.emplace_back(callback);
    }

};