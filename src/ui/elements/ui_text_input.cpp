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