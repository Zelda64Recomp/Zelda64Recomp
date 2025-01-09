#include "ui_toggle.h"

#include <cassert>

namespace recompui {

    Toggle::Toggle(Element *parent) : Element(parent, Events(EventType::Click, EventType::Hover), "button") {
        set_width(162.0f);
        set_height(72.0f);
        set_border_radius(36.0f);
        set_opacity(0.9f);
        set_cursor(Cursor::Pointer);
        set_border_width(2.0f);

        floater = std::make_unique<Element>(this);
        floater->set_position(Position::Relative);
        floater->set_top(2.0f);
        floater->set_width(80.0f);
        floater->set_height(64.0f);
        floater->set_border_radius(32.0f);

        set_checked_internal(false, false, true);
        update_properties();
    }

    void Toggle::set_checked_internal(bool checked, bool animate, bool setup) {
        if (this->checked != checked || setup) {
            this->checked = checked;

            floater->set_left(floater_left_target(), Unit::Dp, animate ? Animation::tween(0.1f) : Animation());

            if (!setup) {
                update_properties();

                for (const auto &function : checked_callbacks) {
                    function(checked);
                }
            }
        }
    }

    void Toggle::update_properties() {
        Color border_color = checked ? Color{ 34, 177, 76, 255 } : Color{ 177, 76, 34, 255 };
        Color main_color = checked ? Color{ 68, 206, 120, 255 } : Color{ 206, 120, 68, 255 };
        main_color.a = hovered ? 76 : 0;

        set_border_color(border_color);
        set_background_color(main_color);
        floater->set_background_color(border_color);
    }

    float Toggle::floater_left_target() const {
        return checked ? 4.0f : 78.0f;
    }

    void Toggle::process_event(const Event &e) {
        switch (e.type) {
        case EventType::Click:
            set_checked_internal(!checked, true, false);
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

    void Toggle::set_checked(bool checked) {
        set_checked_internal(checked, false, false);
    }

    bool Toggle::is_checked() const {
        return checked;
    }

    void Toggle::add_checked_callback(std::function<void(bool)> callback) {
        checked_callbacks.emplace_back(callback);
    }
};