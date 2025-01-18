#include "ui_clickable.h"

namespace recompui {

    static const std::string_view hover_state = "hover";
    static const std::string_view disabled_state = "disabled";

    Clickable::Clickable(Element *parent, bool draggable) : Element(parent, Events(EventType::Click, EventType::Hover, EventType::Enable, draggable ? EventType::Drag : EventType::None)) {
        if (draggable) {
            set_drag(Drag::Drag);
        }
    }

    void Clickable::process_event(const Event &e) {
        switch (e.type) {
        case EventType::Click:
            for (const auto &function : pressed_callbacks) {
                function(e.click.mouse.x, e.click.mouse.y);
            }
            break;
        case EventType::Hover:
            set_style_enabled(hover_state, e.hover.active);
            break;
        case EventType::Enable:
            set_style_enabled(disabled_state, !e.enable.enable);
            break;
        case EventType::Drag:
            for (const auto &function : dragged_callbacks) {
                function(e.drag.mouse.x, e.drag.mouse.y, e.drag.phase);
            }
            break;
        default:
            break;
        }
    }

    void Clickable::add_pressed_callback(std::function<void(float, float)> callback) {
        pressed_callbacks.emplace_back(callback);
    }

    void Clickable::add_dragged_callback(std::function<void(float, float, DragPhase)> callback) {
        dragged_callbacks.emplace_back(callback);
    }

};