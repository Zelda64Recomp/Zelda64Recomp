#pragma once

#include "ui_clickable.h"
#include "ui_label.h"

namespace recompui {
    
    enum SliderType {
        Double,
        Percent,
        Integer
    };

    class Slider : public Element {
    private:
        SliderType type = SliderType::Percent;
        Label *value_label = nullptr;
        Clickable *slider_element = nullptr;
        Clickable *bar_element = nullptr;
        Clickable *circle_element = nullptr;
        double value = 50.0;
        double min_value = 0.0;
        double max_value = 100.0;
        double step_value = 0.0;
        std::vector<std::function<void(double)>> value_changed_callbacks;
        std::function<void(bool)> focus_callback = nullptr;

        void set_value_internal(double v, bool setup, bool trigger_callbacks);
        void bar_pressed(float x, float y);
        void bar_dragged(float x, float y, DragPhase phase);
        void circle_dragged(float x, float y, DragPhase phase);
        void update_value_from_mouse(float x);
        void update_circle_position();
        void update_label_text();
        void set_input_value(const ElementValue& val) override;
        ElementValue get_element_value() override { return get_value(); }

    protected:
        virtual void process_event(const Event &e) override;
        std::string_view get_type_name() override { return "Slider"; }

    public:
        Slider(Element *parent, SliderType type);
        virtual ~Slider();
        void set_value(double v);
        double get_value() const;
        void set_min_value(double v);
        double get_min_value() const;
        void set_max_value(double v);
        double get_max_value() const;
        void set_step_value(double v);
        double get_step_value() const;
        void add_value_changed_callback(std::function<void(double)> callback);
        void do_step(bool increment);
        void set_focus_callback(std::function<void(bool)> callback);
    };

} // namespace recompui
