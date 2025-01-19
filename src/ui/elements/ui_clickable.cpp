#include "ui_clickable.h"

namespace recompui {

    Clickable::Clickable(Element *parent, bool draggable) : Element(parent, Events(EventType::Click, EventType::Hover, EventType::Enable, draggable ? EventType::Drag : EventType::None)) {
        if (draggable) {
            set_drag(Drag::Drag);
        }
    }

    void Clickable::process_event(const Event &e) {
        switch (e.type) {
        case EventType::Click: {
            const EventClick &click = std::get<EventClick>(e.variant);
            for (const auto &function : pressed_callbacks) {
                function(click.x, click.y);
            }
            break;
        }
        case EventType::Hover:
            set_style_enabled(hover_state, std::get<EventHover>(e.variant).active);
            break;
        case EventType::Enable:
            set_style_enabled(disabled_state, !std::get<EventEnable>(e.variant).active);
            break;
        case EventType::Drag: {
            const EventDrag &drag = std::get<EventDrag>(e.variant);
            for (const auto &function : dragged_callbacks) {
                function(drag.x, drag.y, drag.phase);
            }
            break;
        }
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