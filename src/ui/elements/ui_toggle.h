#pragma once

#include "ui_element.h"

namespace recompui {

    class Toggle : public Element {
    protected:
        std::unique_ptr<Element> floater;
        std::list<std::function<void(bool)>> checked_callbacks;
        bool checked = false;
        bool hovered = false;

        void set_checked_internal(bool checked, bool animate, bool setup);
        void update_properties();
        float floater_left_target() const;

        // Element overrides.
        virtual void process_event(const Event &e) override;
    public:
        Toggle(Element *parent);
        void set_checked(bool checked);
        bool is_checked() const;
        void add_checked_callback(std::function<void(bool)> callback);
    };

} // namespace recompui