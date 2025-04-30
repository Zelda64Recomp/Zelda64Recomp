#pragma once

#include "ui_element.h"

namespace recompui {

    class Clickable : public Element {
    protected:
        std::vector<std::function<void(float, float)>> clicked_callbacks;
        std::vector<std::function<void(float, float)>> pressed_callbacks;
        std::vector<std::function<void(float, float, DragPhase)>> dragged_callbacks;

        // Element overrides.
        virtual void process_event(const Event &e) override;
        std::string_view get_type_name() override { return "Clickable"; }
    public:
        Clickable(Element *parent, bool draggable = false);
        void add_clicked_callback(std::function<void(float, float)> callback);
        void add_pressed_callback(std::function<void(float, float)> callback);
        void add_dragged_callback(std::function<void(float, float, DragPhase)> callback);
    };

} // namespace recompui