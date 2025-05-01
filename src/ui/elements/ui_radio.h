#pragma once

#include "ui_container.h"
#include "ui_label.h"

namespace recompui {
    class RadioOption : public Element {
    private:
        Style hover_style;
        Style checked_style;
        Style pulsing_style;
        std::function<void(uint32_t)> pressed_callback = nullptr;
        std::function<void(bool)> focus_callback = nullptr;
        uint32_t index = 0;
    protected:
        virtual void process_event(const Event &e) override;
        std::string_view get_type_name() override { return "LabelRadioOption"; }
    public:
        RadioOption(Element *parent, std::string_view name, uint32_t index);
        void set_pressed_callback(std::function<void(uint32_t)> callback);
        void set_focus_callback(std::function<void(bool)> callback);
        void set_selected_state(bool enable);
    };

    class Radio : public Container {
    private:
        std::vector<RadioOption *> options;
        uint32_t index = 0;
        std::vector<std::function<void(uint32_t)>> index_changed_callbacks;
        std::function<void(bool)> focus_callback = nullptr;
        bool child_focus_queued = false;

        void set_index_internal(uint32_t index, bool setup, bool trigger_callbacks);
        void option_selected(uint32_t index);
        void set_input_value(const ElementValue& val) override;
        ElementValue get_element_value() override { return get_index(); }
    protected:
        virtual void process_event(const Event &e) override;
        std::string_view get_type_name() override { return "LabelRadio"; }
        void queue_child_focus() { child_focus_queued = true; queue_update(); }
    public:
        Radio(Element *parent);
        virtual ~Radio();
        void add_option(std::string_view name);
        void set_index(uint32_t index);
        uint32_t get_index() const;
        void add_index_changed_callback(std::function<void(uint32_t)> callback);
        void set_focus_callback(std::function<void(bool)> callback);
        size_t num_options() const { return options.size(); }
        RadioOption* get_option_element(size_t option_index) { return options[option_index]; }
        RadioOption* get_current_option_element() { return options.empty() ? nullptr : options[index]; }
        void set_nav_auto(NavDirection dir) override;
        void set_nav_none(NavDirection dir) override;
        void set_nav(NavDirection dir, Element* element) override;
        void set_nav_manual(NavDirection dir, const std::string& target) override;
    };

} // namespace recompui
