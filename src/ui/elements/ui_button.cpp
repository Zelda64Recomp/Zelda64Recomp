#include "ui_button.h"

#include <cassert>

namespace recompui {

    Button::Button(const std::string &text, ButtonStyle style, Element *parent) : Element(parent, Events(EventType::Click, EventType::Hover), "button") {
        this->style = style;

        set_text(text);
        set_display(Display::Block);
        set_padding(23.0f);
        set_border_width(1.1f);
        set_border_radius(12.0f);
        set_font_size(28.0f);
        set_letter_spacing(3.08f);
        set_line_height(28.0f);
        set_font_style(FontStyle::Normal);
        set_font_weight(700);
        set_cursor(Cursor::Pointer);

        // transition: color 0.05s linear-in-out, background-color 0.05s linear-in-out;

        update_properties();
    }

    void Button::update_properties() {
        uint8_t border_opacity = hovered ? 255 : 204;
        uint8_t background_opacity = hovered ? 76 : 13;
        set_color(hovered ? Color{ 242, 242, 242, 255 } : Color{ 204, 204, 204, 255 });

        switch (style) {
        case ButtonStyle::Primary: {
            set_border_color({ 185, 125, 242, border_opacity });
            set_background_color({ 185, 125, 242, background_opacity });
            break;
        }
        case ButtonStyle::Secondary: {
            set_border_color({ 23, 214, 232, border_opacity });
            set_background_color({ 23, 214, 232, background_opacity });
            break;
        }
        default:
            assert(false && "Unknown button style.");
            break;
        }
    }

    void Button::process_event(const Event &e) {
        switch (e.type) {
        case EventType::Click:
            for (const auto &function : pressed_callbacks) {
                function();
            }
            break;
        case EventType::Hover:
            hovered = e.hover.active;
            update_properties();
            break;
        default:
            assert(false && "Unknown event type.");
            break;
        }
    }

    void Button::add_pressed_callback(std::function<void()> callback) {
        pressed_callbacks.emplace_back(callback);
    }
};