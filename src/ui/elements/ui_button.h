#pragma once

#include "ui_element.h"

namespace recompui {

    enum class ButtonStyle {
        Primary,
        Secondary
    };

    class Button : public Element {
    protected:
        ButtonStyle style = ButtonStyle::Primary;
        Style hover_style;
        Style focus_style;
        Style disabled_style;
        Style hover_disabled_style;
        std::list<std::function<void()>> pressed_callbacks;

        // Element overrides.
        virtual void process_event(const Event &e) override;
        std::string_view get_type_name() override { return "Button"; }
    public:
        Button(Element *parent, const std::string &text, ButtonStyle style);
        void add_pressed_callback(std::function<void()> callback);
        Style* get_hover_style() { return &hover_style; }
        Style* get_focus_style() { return &focus_style; }
        Style* get_disabled_style() { return &disabled_style; }
        Style* get_hover_disabled_style() { return &hover_disabled_style; }
    };

} // namespace recompui