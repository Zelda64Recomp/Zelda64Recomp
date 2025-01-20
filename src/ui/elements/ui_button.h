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
        Style disabled_style;
        Style hover_disabled_style;
        std::list<std::function<void()>> pressed_callbacks;

        // Element overrides.
        virtual void process_event(const Event &e) override;
    public:
        Button(Element *parent, const std::string &text, ButtonStyle style);
        void add_pressed_callback(std::function<void()> callback);
    };

} // namespace recompui