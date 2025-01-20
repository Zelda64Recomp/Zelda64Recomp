#pragma once

#include "ui_container.h"
#include "ui_label.h"

namespace recompui {
    class RadioOption : public Element {
    private:
        Style hover_style;
        Style checked_style;
        std::function<void(uint32_t)> pressed_callback = nullptr;
        uint32_t index = 0;
    protected:
        virtual void process_event(const Event &e) override;
    public:
        RadioOption(Element *parent, std::string_view name, uint32_t index);
        void set_pressed_callback(std::function<void(uint32_t)> callback);
        void set_selected_state(bool enable);
    };

    class Radio : public Container {
    private:
        std::vector<RadioOption *> options;
        uint32_t index = 0;
        std::vector<std::function<void(uint32_t)>> index_changed_callbacks;

        void set_index_internal(uint32_t index, bool setup, bool trigger_callbacks);
        void option_selected(uint32_t index);
    public:
        Radio(Element *parent);
        virtual ~Radio();
        void add_option(std::string_view name);
        void set_index(uint32_t index);
        uint32_t get_index() const;
        void add_index_changed_callback(std::function<void(uint32_t)> callback);
    };

} // namespace recompui