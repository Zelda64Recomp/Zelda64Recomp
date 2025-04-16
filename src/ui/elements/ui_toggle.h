#pragma once

#include "ui_element.h"

namespace recompui {

    class Toggle : public Element {
    protected:
        Element *floater;
        float floater_left = 0.0f;
        std::chrono::high_resolution_clock::duration last_time;
        std::list<std::function<void(bool)>> checked_callbacks;
        Style checked_style;
        Style hover_style;
        Style focus_style;
        Style checked_hover_style;
        Style checked_focus_style;
        Style disabled_style;
        Style checked_disabled_style;
        Style floater_checked_style;
        Style floater_disabled_style;
        Style floater_disabled_checked_style;
        bool checked = false;

        void set_checked_internal(bool checked, bool animate, bool setup, bool trigger_callbacks);
        float floater_left_target() const;

        // Element overrides.
        virtual void process_event(const Event &e) override;
        std::string_view get_type_name() override { return "Toggle"; }
    public:
        Toggle(Element *parent);
        void set_checked(bool checked);
        bool is_checked() const;
        void add_checked_callback(std::function<void(bool)> callback);
    };

} // namespace recompui