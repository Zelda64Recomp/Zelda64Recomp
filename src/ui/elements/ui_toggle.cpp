#include "ui_toggle.h"

#include <cassert>

#include <ultramodern/ultramodern.hpp>

namespace recompui {

    Toggle::Toggle(Element *parent) : Element(parent, Events(EventType::Click, EventType::Focus, EventType::Hover, EventType::Enable), "button") {
        enable_focus();

        set_width(162.0f);
        set_height(72.0f);
        set_border_radius(36.0f);
        set_opacity(0.9f);
        set_cursor(Cursor::Pointer);
        set_border_width(2.0f);
        set_border_color(Color{ 177, 76, 34, 255 });
        set_background_color(Color{ 0, 0, 0, 0 });
        checked_style.set_border_color(Color{ 34, 177, 76, 255 });
        hover_style.set_border_color(Color{ 177, 76, 34, 255 });
        hover_style.set_background_color(Color{ 206, 120, 68, 76 });
        focus_style.set_border_color(Color{ 177, 76, 34, 255 });
        focus_style.set_background_color(Color{ 206, 120, 68, 76 });
        checked_hover_style.set_border_color(Color{ 34, 177, 76, 255 });
        checked_hover_style.set_background_color(Color{ 68, 206, 120, 76 });
        checked_focus_style.set_border_color(Color{ 34, 177, 76, 255 });
        checked_focus_style.set_background_color(Color{ 68, 206, 120, 76 });
        disabled_style.set_border_color(Color{ 177, 76, 34, 128 });
        checked_disabled_style.set_border_color(Color{ 34, 177, 76, 128 });
        add_style(&checked_style, checked_state);
        add_style(&hover_style, hover_state);
        add_style(&focus_style, focus_state);
        add_style(&checked_hover_style, { checked_state, hover_state });
        add_style(&checked_focus_style, { checked_state, focus_state });
        add_style(&disabled_style, disabled_state);
        add_style(&checked_disabled_style, { checked_state, disabled_state });

        ContextId context = get_current_context();

        floater = context.create_element<Element>(this);
        floater->set_position(Position::Relative);
        floater->set_top(2.0f);
        floater->set_width(80.0f);
        floater->set_height(64.0f);
        floater->set_border_radius(32.0f);
        floater->set_background_color(Color{ 177, 76, 34, 255 });
        floater_checked_style.set_background_color(Color{ 34, 177, 76, 255 });
        floater_disabled_style.set_background_color(Color{ 177, 76, 34, 128 });
        floater_disabled_checked_style.set_background_color(Color{ 34, 177, 76, 128 });
        floater->add_style(&floater_checked_style, checked_state);
        floater->add_style(&floater_disabled_style, disabled_state);
        floater->add_style(&floater_disabled_checked_style, { checked_state, disabled_state });

        set_checked_internal(false, false, true, false);
    }

    void Toggle::set_checked_internal(bool checked, bool animate, bool setup, bool trigger_callbacks) {
        if (this->checked != checked || setup) {
            this->checked = checked;

            if (animate) {
                last_time = ultramodern::time_since_start();
                queue_update();
            }
            else {
                floater_left = floater_left_target();
            }

            floater->set_left(floater_left, Unit::Dp);

            if (trigger_callbacks) {
                for (const auto &function : checked_callbacks) {
                    function(checked);
                }
            }

            set_style_enabled(checked_state, checked);
            floater->set_style_enabled(checked_state, checked);
        }
    }

    float Toggle::floater_left_target() const {
        return checked ? 78.0f : 4.0f;
    }

    void Toggle::process_event(const Event &e) {
        switch (e.type) {
        case EventType::Click:
            if (is_enabled()) {
                set_checked_internal(!checked, true, false, true);
            }

            break;
        case EventType::Hover: {
            bool hover_active = std::get<EventHover>(e.variant).active && is_enabled();
            set_style_enabled(hover_state, hover_active);
            floater->set_style_enabled(hover_state, hover_active);
            break;
        }
        case EventType::Focus: {
            bool focus_active = std::get<EventFocus>(e.variant).active;
            set_style_enabled(focus_state, focus_active);
            break;
        }
        case EventType::Enable: {
            bool enable_active = std::get<EventEnable>(e.variant).active;
            set_style_enabled(disabled_state, !enable_active);
            floater->set_style_enabled(disabled_state, !enable_active);
            if (enable_active) {
                set_cursor(Cursor::Pointer);
                set_focusable(true);
            }
            else {
                set_cursor(Cursor::None);
                set_focusable(false);
            }
            break;
        }
        case EventType::Update: {
            std::chrono::high_resolution_clock::duration now = ultramodern::time_since_start();
            float delta_time = std::max(std::chrono::duration<float>(now - last_time).count(), 0.0f);
            last_time = now;

            constexpr float dp_speed = 740.0f;
            const float target = floater_left_target();
            if (target < floater_left) {
                floater_left += std::max(-dp_speed * delta_time, target - floater_left);
            }
            else {
                floater_left += std::min(dp_speed * delta_time, target - floater_left);
            }

            if (abs(target - floater_left) < 1e-4f) {
                floater_left = target;
            }
            else {
                queue_update();
            }

            floater->set_left(floater_left, Unit::Dp);

            break;
        }
        default:
            break;
        }
    }

    void Toggle::set_checked(bool checked) {
        set_checked_internal(checked, false, false, false);
    }

    bool Toggle::is_checked() const {
        return checked;
    }

    void Toggle::add_checked_callback(std::function<void(bool)> callback) {
        checked_callbacks.emplace_back(callback);
    }
};