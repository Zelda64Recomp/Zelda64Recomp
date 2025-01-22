#ifndef RECOMPUI_CONFIG_SUB_MENU_H
#define RECOMPUI_CONFIG_SUB_MENU_H

#include <span>

#include "elements/ui_button.h"
#include "elements/ui_container.h"
#include "elements/ui_label.h"
#include "elements/ui_radio.h"
#include "elements/ui_scroll_container.h"
#include "elements/ui_slider.h"
#include "elements/ui_text_input.h"

namespace recompui {

class ConfigOptionElement : public Element {
protected:
    Label *name_label = nullptr;
    std::string id;
    std::string name;
    std::string description;
    std::function<void(ConfigOptionElement *, bool)> hover_callback = nullptr;

    virtual void process_event(const Event &e) override;
public:
    ConfigOptionElement(Element *parent);
    virtual ~ConfigOptionElement();
    void set_id(std::string_view id);
    void set_name(std::string_view name);
    void set_description(std::string_view description);
    void set_hover_callback(std::function<void(ConfigOptionElement *, bool)> callback);
    const std::string &get_description() const;
};

class ConfigOptionSlider : public ConfigOptionElement {
protected:
    Slider *slider = nullptr;
    std::function<void(const std::string &, double)> callback;

    void slider_value_changed(double v);
public:
    ConfigOptionSlider(Element *parent, double value, double min_value, double max_value, double step_value, bool percent, std::function<void(const std::string &, double)> callback);
};

class ConfigOptionTextInput : public ConfigOptionElement {
protected:
    TextInput *text_input = nullptr;
    std::function<void(const std::string &, const std::string &)> callback;

    void text_changed(const std::string &text);
public:
    ConfigOptionTextInput(Element *parent, std::string_view value, std::function<void(const std::string &, const std::string &)> callback);
};

class ConfigOptionRadio : public ConfigOptionElement {
protected:
    Radio *radio = nullptr;
    std::function<void(const std::string &, uint32_t)> callback;

    void index_changed(uint32_t index);
public:
    ConfigOptionRadio(Element *parent, uint32_t value, const std::vector<std::string> &options, std::function<void(const std::string &, uint32_t)> callback);
};

class ConfigSubMenu : public Element {
private:
    Container *header_container = nullptr;
    Button *back_button = nullptr;
    Label *title_label = nullptr;
    Container *body_container = nullptr;
    Label *description_label = nullptr;
    Container *config_container = nullptr;
    ScrollContainer *config_scroll_container = nullptr;
    std::vector<ConfigOptionElement *> config_option_elements;
    std::unordered_set<ConfigOptionElement *> hover_option_elements;

    void back_button_pressed();
    void option_hovered(ConfigOptionElement *option, bool active);
    void add_option(ConfigOptionElement *option, std::string_view id, std::string_view name, std::string_view description);

public:
    ConfigSubMenu(Element *parent);
    virtual ~ConfigSubMenu();
    void enter(std::string_view title);
    void clear_options();
    void add_slider_option(std::string_view id, std::string_view name, std::string_view description, double value, double min, double max, double step, bool percent, std::function<void(const std::string &, double)> callback);
    void add_text_option(std::string_view id, std::string_view name, std::string_view description, std::string_view value, std::function<void(const std::string &, const std::string &)> callback);
    void add_radio_option(std::string_view id, std::string_view name, std::string_view description, uint32_t value, const std::vector<std::string> &options, std::function<void(const std::string &, uint32_t)> callback);
};

class ElementConfigSubMenu : public Rml::Element {
public:
    ElementConfigSubMenu(const Rml::String &tag);
    virtual ~ElementConfigSubMenu();
    void set_display(bool display);
    ConfigSubMenu *get_config_sub_menu_element() const;
private:
    ConfigSubMenu *config_sub_menu;
};

}
#endif