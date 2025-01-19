#pragma once

#include "ui_element.h"

namespace recompui {

    class TextInput : public Element {
    private:
        std::string text;
        std::vector<std::function<void(const std::string &)>> text_changed_callbacks;
    protected:
        virtual void process_event(const Event &e) override;
    public:
        TextInput(Element *parent);
        void set_text(std::string_view text);
        const std::string &get_text();
        void add_text_changed_callback(std::function<void(const std::string &)> callback);
    };

} // namespace recompui
