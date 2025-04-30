#include "ui_clickable.h"

namespace recompui {

    Clickable::Clickable(Element *parent, bool draggable) : Element(parent, Events(EventType::Click, EventType::MouseButton, EventType::Hover, EventType::Enable, draggable ? EventType::Drag : EventType::None)) {
        set_cursor(Cursor::Pointer);
        if (draggable) {
            set_drag(Drag::Drag);
        }
    }

    void Clickable::process_event(const Event &e) {
        switch (e.type) {
        case EventType::Click: {
            if (is_enabled()) {
                const EventClick &click = std::get<EventClick>(e.variant);
                for (const auto &function : clicked_callbacks) {
                    function(click.x, click.y);
                }
                break;
            }
        }
        case EventType::MouseButton: {
            if (is_enabled()) {
                const EventMouseButton &mousebutton = std::get<EventMouseButton>(e.variant);
                if (mousebutton.button == MouseButton::Left && mousebutton.pressed) {
                    for (const auto &function : pressed_callbacks) {
                        function(mousebutton.x, mousebutton.y);
                    }
                }
                break;
            }
        }
        case EventType::Hover:
            set_style_enabled(hover_state, std::get<EventHover>(e.variant).active && is_enabled());
            break;
        case EventType::Enable:
            {
                bool enable_active = std::get<EventEnable>(e.variant).active;
                set_style_enabled(disabled_state, !enable_active);
                if (enable_active) {
                    set_cursor(Cursor::Pointer);
                    set_focusable(true);
                }
                else {
                    set_cursor(Cursor::None);
                    set_focusable(false);
                }
            }
            break;
        case EventType::Drag: {
            if (is_enabled()) {
                const EventDrag &drag = std::get<EventDrag>(e.variant);
                for (const auto &function : dragged_callbacks) {
                    function(drag.x, drag.y, drag.phase);
                }
                break;
            }
        }
        default:
            break;
        }
    }

    void Clickable::add_clicked_callback(std::function<void(float, float)> callback) {
        clicked_callbacks.emplace_back(callback);
    }

    void Clickable::add_pressed_callback(std::function<void(float, float)> callback) {
        pressed_callbacks.emplace_back(callback);
    }

    void Clickable::add_dragged_callback(std::function<void(float, float, DragPhase)> callback) {
        dragged_callbacks.emplace_back(callback);
    }

};