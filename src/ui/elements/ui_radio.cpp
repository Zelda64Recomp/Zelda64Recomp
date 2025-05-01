#include "overloaded.h"
#include "ui_radio.h"
#include "../ui_utils.h"

namespace recompui {

    // RadioOption

    RadioOption::RadioOption(Element *parent, std::string_view name, uint32_t index) : Element(parent, Events(EventType::MouseButton, EventType::Click, EventType::Focus, EventType::Hover, EventType::Enable, EventType::Update), "label", true) {
        this->index = index;

        enable_focus();
        set_text(name);
        set_cursor(Cursor::Pointer);
        set_font_size(20.0f);
        set_letter_spacing(2.8f);
        set_line_height(20.0f);
        set_font_weight(400);
        set_font_style(FontStyle::Normal);
        set_border_color(Color{ 242, 242, 242, 0 });
        set_border_bottom_width(1.0f);
        set_color(Color{ 255, 255, 255, 153 });
        set_padding_bottom(8.0f);
        set_text_transform(TextTransform::Uppercase);
        set_height_auto();
        hover_style.set_color(Color{ 255, 255, 255, 204 });
        checked_style.set_color(Color{ 255, 255, 255, 255 });
        checked_style.set_border_color(Color{ 242, 242, 242, 255 });
        pulsing_style.set_border_color(Color{ 23, 214, 232, 244 });

        add_style(&hover_style, { hover_state });
        add_style(&checked_style, { checked_state });
        add_style(&pulsing_style, { focus_state });
    }

    void RadioOption::set_pressed_callback(std::function<void(uint32_t)> callback) {
        pressed_callback = callback;
    }

    void RadioOption::set_focus_callback(std::function<void(bool)> callback) {
        focus_callback = callback;
    }

    void RadioOption::set_selected_state(bool enable) {
        set_style_enabled(checked_state, enable);
    }

    void RadioOption::process_event(const Event &e) {
        switch (e.type) {
        case EventType::MouseButton:
            {
                const EventMouseButton &mousebutton = std::get<EventMouseButton>(e.variant);
                if (mousebutton.button == MouseButton::Left && mousebutton.pressed) {
                    pressed_callback(index);
                }
            }
            break;
        case EventType::Click:
            pressed_callback(index);
            break;
        case EventType::Hover:
            set_style_enabled(hover_state, std::get<EventHover>(e.variant).active);
            break;
        case EventType::Enable:
            set_style_enabled(disabled_state, !std::get<EventEnable>(e.variant).active);
            break;
        case EventType::Focus:
            {
                bool active = std::get<EventFocus>(e.variant).active;
                set_style_enabled(focus_state, active);
                if (active) {
                    queue_update();
                }
                if (focus_callback != nullptr) {
                    focus_callback(active);
                }
            }
            break;
        case EventType::Update:
            if (is_style_enabled(focus_state)) {
                pulsing_style.set_color(recompui::get_pulse_color(750));
                apply_styles();
                queue_update();
            }
            break;
        default:
            break;
        }
    }

    // Radio

    void Radio::set_index_internal(uint32_t index, bool setup, bool trigger_callbacks) {
        if (this->index != index || setup) {
            options[this->index]->set_selected_state(false);
            this->index = index;
            options[index]->set_selected_state(true);

            if (trigger_callbacks) {
                for (const auto &function : index_changed_callbacks) {
                    function(index);
                }
            }
        }
    }

    void Radio::option_selected(uint32_t index) {
        set_index_internal(index, false, true);
    }
    
    void Radio::set_input_value(const ElementValue& val) {
        std::visit(overloaded {
            [this](uint32_t u) { set_index(u); }, 
            [this](float f) { set_index(f); }, 
            [this](double d) { set_index(d); },
            [](std::monostate) {}
        }, val);
    }

    Radio::Radio(Element *parent) : Container(parent, FlexDirection::Row, JustifyContent::FlexStart, Events(EventType::Focus, EventType::Update)) {
        set_gap(24.0f);
        set_align_items(AlignItems::FlexStart);
        enable_focus();
    }

    void Radio::process_event(const Event &e) {
        switch (e.type) {
        case EventType::Focus:
            if (!options.empty()) {
                if (std::get<EventFocus>(e.variant).active) {
                    blur();
                    queue_child_focus();
                }
                if (focus_callback != nullptr) {
                    focus_callback(std::get<EventFocus>(e.variant).active);
                }
            }
            break;
        case EventType::Update:
            if (child_focus_queued) {
                child_focus_queued = false;
                options[index]->focus();
            }
        }
    }

    Radio::~Radio() {

    }

    void Radio::add_option(std::string_view name) {
        RadioOption *option = get_current_context().create_element<RadioOption>(this, name, uint32_t(options.size()));
        option->set_pressed_callback([this](uint32_t index){ options[index]->focus(); option_selected(index); });
        option->set_focus_callback([this](bool active) {
            if (focus_callback != nullptr) {
                focus_callback(active);
            }
        });
        options.emplace_back(option);

        // The first option was added, select it.
        if (options.size() == 1) {
            set_index_internal(0, true, false);
        }
        // At least one other option already existed, so set up navigation.
        else {
            options[options.size() - 2]->set_nav(NavDirection::Right, options[options.size() - 1]);
            options[options.size() - 1]->set_nav(NavDirection::Left, options[options.size() - 2]);
        }
    }

    void Radio::set_index(uint32_t index) {
        set_index_internal(index, false, false);
    }

    uint32_t Radio::get_index() const {
        return index;
    }

    void Radio::add_index_changed_callback(std::function<void(uint32_t)> callback) {
        index_changed_callbacks.emplace_back(callback);
    }

    void Radio::set_focus_callback(std::function<void(bool)> callback) {
        focus_callback = callback;
    }
    
    void Radio::set_nav_auto(NavDirection dir) {
        Element::set_nav_auto(dir);
        if (!options.empty()) {
            switch (dir) {
                case NavDirection::Up:
                case NavDirection::Down:
                    for (Element* e : options) {
                        e->set_nav_auto(dir);
                    }
                    break;
                case NavDirection::Left:
                    options.front()->set_nav_auto(dir);
                    break;
                case NavDirection::Right:
                    options.back()->set_nav_auto(dir);
                    break;
            }
        }
    }

    void Radio::set_nav_none(NavDirection dir) {
        Element::set_nav_none(dir);
        if (!options.empty()) {
            switch (dir) {
                case NavDirection::Up:
                case NavDirection::Down:
                    for (Element* e : options) {
                        e->set_nav_none(dir);
                    }
                    break;
                case NavDirection::Left:
                    options.front()->set_nav_none(dir);
                    break;
                case NavDirection::Right:
                    options.back()->set_nav_none(dir);
                    break;
            }
        }
    }

    void Radio::set_nav(NavDirection dir, Element* element) {
        Element::set_nav(dir, element);
        if (!options.empty()) {
            switch (dir) {
                case NavDirection::Up:
                case NavDirection::Down:
                    for (Element* e : options) {
                        e->set_nav(dir, element);
                    }
                    break;
                case NavDirection::Left:
                    options.front()->set_nav(dir, element);
                    break;
                case NavDirection::Right:
                    options.back()->set_nav(dir, element);
                    break;
            }
        }
    }

    void Radio::set_nav_manual(NavDirection dir, const std::string& target) {
        Element::set_nav_manual(dir, target);
        if (!options.empty()) {
            switch (dir) {
                case NavDirection::Up:
                case NavDirection::Down:
                    for (Element* e : options) {
                        e->set_nav_manual(dir, target);
                    }
                    break;
                case NavDirection::Left:
                    options.front()->set_nav_manual(dir, target);
                    break;
                case NavDirection::Right:
                    options.back()->set_nav_manual(dir, target);
                    break;
            }
        }
    }

};
