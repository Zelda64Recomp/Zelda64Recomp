#include "ui_slider.h"

#include <cmath>
#include <charconv>

namespace recompui {

    void Slider::set_value_internal(double v, bool setup, bool trigger_callbacks) {
        if (step_value != 0.0) {
            v = std::lround(v / step_value) * step_value;
        }

        if (value != v || setup) {
            value = v;
            update_circle_position();
            update_label_text();

            if (trigger_callbacks) {
                for (auto callback : value_changed_callbacks) {
                    callback(v);
                }
            }
        }
    }

    void Slider::bar_clicked(float x, float) {
        update_value_from_mouse(x);
    }

    void Slider::bar_dragged(float x, float, DragPhase) {
        update_value_from_mouse(x);
    }

    void Slider::circle_dragged(float x, float, DragPhase) {
        update_value_from_mouse(x);
    }

    void Slider::update_value_from_mouse(float x) {
        double left = slider_element->get_absolute_left();
        double width = slider_element->get_client_width();
        double ratio = std::clamp((x - left) / width, 0.0, 1.0);
        set_value_internal(min_value + ratio * (max_value - min_value), false, true);
    }

    void Slider::update_circle_position() {
        double ratio = std::clamp((value - min_value) / (max_value - min_value), 0.0, 1.0);
        circle_element->set_left(slider_width_dp * ratio);
    }

    void Slider::update_label_text() {
        char text_buffer[32];
        int precision = type == SliderType::Double ? 1 : 0;
        auto result = std::to_chars(text_buffer, text_buffer + sizeof(text_buffer) - 1, value, std::chars_format::fixed, precision);
        if (result.ec == std::errc()) {
            if (type == SliderType::Percent) {
                *result.ptr = '%';
                result.ptr++;
            }

            value_label->set_text(std::string(text_buffer, result.ptr));
        }
    }

    Slider::Slider(Element *parent, SliderType type) : Element(parent) {
        this->type = type;

        set_display(Display::Flex);
        set_flex(1.0f, 1.0f, 100.0f, Unit::Percent);
        set_flex_direction(FlexDirection::Row);

        ContextId context = get_current_context();

        value_label = context.create_element<Label>(this, "0", LabelStyle::Small);
        value_label->set_margin_right(20.0f);
        value_label->set_min_width(60.0f);
        value_label->set_max_width(60.0f);

        slider_element = context.create_element<Element>(this);
        slider_element->set_width(slider_width_dp);

        {
            bar_element = context.create_element<Clickable>(slider_element, true);
            bar_element->set_width(100.0f, Unit::Percent);
            bar_element->set_height(2.0f);
            bar_element->set_margin_top(8.0f);
            bar_element->set_background_color(Color{ 255, 255, 255, 50 });
            bar_element->add_pressed_callback(std::bind(&Slider::bar_clicked, this, std::placeholders::_1, std::placeholders::_2));
            bar_element->add_dragged_callback(std::bind(&Slider::bar_dragged, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            
            circle_element = context.create_element<Clickable>(slider_element, true);
            circle_element->set_position(Position::Relative);
            circle_element->set_width(16.0f);
            circle_element->set_height(16.0f);
            circle_element->set_margin_top(-8.0f);
            circle_element->set_margin_right(-8.0f);
            circle_element->set_margin_left(-8.0f);
            circle_element->set_background_color(Color{ 204, 204, 204, 255 });
            circle_element->set_border_radius(8.0f);
            circle_element->add_dragged_callback(std::bind(&Slider::circle_dragged, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            circle_element->set_cursor(Cursor::Pointer);
        }

        set_value_internal(value, true, false);
    }

    Slider::~Slider() {

    }

    void Slider::set_value(double v) {
        set_value_internal(v, false, false);
    }

    double Slider::get_value() const {
        return value;
    }
    void Slider::set_min_value(double v) {
        min_value = v;
    }

    double Slider::get_min_value() const {
        return min_value;
    }

    void Slider::set_max_value(double v) {
        max_value = v;
    }

    double Slider::get_max_value() const {
        return max_value;
    }

    void Slider::set_step_value(double v) {
        step_value = v;
    }

    double Slider::get_step_value() const {
        return step_value;
    }

    void Slider::add_value_changed_callback(std::function<void(double)> callback) {
        value_changed_callbacks.emplace_back(callback);
    }

} // namespace recompui