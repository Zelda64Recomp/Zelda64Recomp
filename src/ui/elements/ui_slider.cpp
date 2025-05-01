#include "overloaded.h"
#include "ui_slider.h"
#include "../ui_utils.h"

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

    void Slider::bar_pressed(float x, float) {
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
        circle_element->set_left(ratio * 100.0, Unit::Percent);
    }

    void Slider::update_label_text() {
        char text_buffer[32];

        if (type == SliderType::Double) {
            std::snprintf(text_buffer, sizeof(text_buffer), "%.1f", value);
        } else if (type == SliderType::Percent) {
            std::snprintf(text_buffer, sizeof(text_buffer), "%d%%", static_cast<int>(value));
        } else {
            std::snprintf(text_buffer, sizeof(text_buffer), "%d", static_cast<int>(value));
        }

        value_label->set_text(text_buffer);
    }
    
    void Slider::set_input_value(const ElementValue& val) {
        std::visit(overloaded {
            [this](uint32_t u) { set_value(u); }, 
            [this](float f) { set_value(f); }, 
            [this](double d) { set_value(d); },
            [](std::monostate) {}
        }, val);
    }

    void Slider::process_event(const Event& e) {
        switch (e.type) {
        case EventType::Focus:
            {
                bool active = std::get<EventFocus>(e.variant).active;
                circle_element->set_style_enabled(focus_state, active);
                if (active) {
                    queue_update();
                }
                if (focus_callback != nullptr) {
                    focus_callback(active);
                }
            }
            break;
        case EventType::Update:
            if (is_enabled()) {
                if (circle_element->is_style_enabled(focus_state)) {
                    circle_element->set_background_color(recompui::get_pulse_color(750));
                    queue_update();
                }
                else {
                    circle_element->set_background_color(Color{ 204, 204, 204, 255 });
                }
            }
            else {
                circle_element->set_background_color(Color{ 102, 102, 102, 255 });
            }
            break;
        case EventType::Navigate:
            {
                NavDirection dir = std::get<EventNavigate>(e.variant).direction;
                if (dir == NavDirection::Left) {
                    do_step(false);
                }
                else if (dir == NavDirection::Right) {
                    do_step(true);
                }
            }
            break;
        case EventType::Enable:
            {
                bool enable_active = std::get<EventEnable>(e.variant).active;
                circle_element->set_enabled(enable_active);
                if (enable_active) {
                    set_cursor(Cursor::Pointer);
                    set_focusable(true);
                    circle_element->set_background_color(Color{ 204, 204, 204, 255 });
                }
                else {
                    set_cursor(Cursor::None);
                    set_focusable(false);
                    circle_element->set_background_color(Color{ 102, 102, 102, 255 });
                }
            }
            break;
        default:
            break;
        }
    }

    Slider::Slider(Element *parent, SliderType type) : Element(parent, Events(EventType::Focus, EventType::Update, EventType::Navigate, EventType::Enable)) {
        this->type = type;

        set_cursor(Cursor::Pointer);
        set_display(Display::Flex);
        set_flex_direction(FlexDirection::Row);
        set_text_align(TextAlign::Left);
        set_min_width(120.0f);

        enable_focus();
        set_nav_none(NavDirection::Left);
        set_nav_none(NavDirection::Right);

        ContextId context = get_current_context();

        value_label = context.create_element<Label>(this, "0", LabelStyle::Small);
        value_label->set_margin_right(20.0f);
        value_label->set_min_width(60.0f);
        value_label->set_max_width(60.0f);

        slider_element = context.create_element<Clickable>(this, true);
        slider_element->set_flex(1.0f, 0.0f);
        slider_element->add_pressed_callback([this](float x, float y){ bar_pressed(x, y); focus(); });
        slider_element->add_dragged_callback([this](float x, float y, recompui::DragPhase phase){ bar_dragged(x, y, phase); focus(); });

        {
            bar_element = context.create_element<Clickable>(slider_element, true);
            bar_element->set_width(100.0f, Unit::Percent);
            bar_element->set_height(2.0f);
            bar_element->set_margin_top(8.0f);
            bar_element->set_background_color(Color{ 255, 255, 255, 50 });
            bar_element->add_pressed_callback([this](float x, float y){ bar_pressed(x, y); focus(); });
            bar_element->add_dragged_callback([this](float x, float y, recompui::DragPhase phase){ bar_dragged(x, y, phase); focus(); });
            
            circle_element = context.create_element<Clickable>(bar_element, true);
            circle_element->set_position(Position::Relative);
            circle_element->set_width(16.0f);
            circle_element->set_height(16.0f);
            circle_element->set_margin_top(-7.0f);
            circle_element->set_margin_right(-8.0f);
            circle_element->set_margin_left(-8.0f);
            circle_element->set_background_color(Color{ 204, 204, 204, 255 });
            circle_element->set_border_radius(8.0f);
            circle_element->add_pressed_callback([this](float, float){ focus(); });
            circle_element->add_dragged_callback([this](float x, float y, recompui::DragPhase phase){ circle_dragged(x, y, phase); focus(); });
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

    void Slider::set_focus_callback(std::function<void(bool)> callback) {
        focus_callback = callback;
    }

    void Slider::do_step(bool increment) {
        double new_value = value;
        if (increment) {
            new_value += step_value;
        }
        else {
            new_value -= step_value;
        }
        new_value = std::clamp(new_value, min_value, max_value);
        if (new_value != value) {
            set_value_internal(new_value, false, true);
        }
    }

} // namespace recompui
