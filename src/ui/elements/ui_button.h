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
        std::unique_ptr<Element> floater;
        std::list<std::function<void()>> pressed_callbacks;
        bool hovered = false;

        void update_properties();

        // Element overrides.
        virtual void process_event(const Event &e) override;
    public:
        Button(const std::string &text, ButtonStyle style, Element *parent);
        void add_pressed_callback(std::function<void()> callback);
    };

} // namespace recompui