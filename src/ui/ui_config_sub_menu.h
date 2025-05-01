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
    std::string option_id;
    std::string name;
    std::string description;
    std::function<void(ConfigOptionElement *, bool)> hover_callback = nullptr;
    std::function<void(const std::string &, bool)> focus_callback = nullptr;

    virtual void process_event(const Event &e) override;
    std::string_view get_type_name() override { return "ConfigOptionElement"; }
public:
    ConfigOptionElement(Element *parent);
    virtual ~ConfigOptionElement();
    void set_option_id(std::string_view id);
    void set_name(std::string_view name);
    void set_description(std::string_view description);
    void set_hover_callback(std::function<void(ConfigOptionElement *, bool)> callback);
    void set_focus_callback(std::function<void(const std::string &, bool)> callback);
    const std::string &get_description() const;
    void set_nav_auto(NavDirection dir) override { get_focus_element()->set_nav_auto(dir); }
    void set_nav_none(NavDirection dir) override { get_focus_element()->set_nav_none(dir); }
    void set_nav(NavDirection dir, Element* element) override { get_focus_element()->set_nav(dir, element); }
    void set_nav_manual(NavDirection dir, const std::string& target) override { get_focus_element()->set_nav_manual(dir, target); }
    virtual Element* get_focus_element() { return this; }
};

class ConfigOptionSlider : public ConfigOptionElement {
protected:
    Slider *slider = nullptr;
    std::function<void(const std::string &, double)> callback;

    void slider_value_changed(double v);
    std::string_view get_type_name() override { return "ConfigOptionSlider"; }
public:
    ConfigOptionSlider(Element *parent, double value, double min_value, double max_value, double step_value, bool percent, std::function<void(const std::string &, double)> callback);
    Element* get_focus_element() override { return slider; }
};

class ConfigOptionTextInput : public ConfigOptionElement {
protected:
    TextInput *text_input = nullptr;
    std::function<void(const std::string &, const std::string &)> callback;

    void text_changed(const std::string &text);
    std::string_view get_type_name() override { return "ConfigOptionTextInput"; }
public:
    ConfigOptionTextInput(Element *parent, std::string_view value, std::function<void(const std::string &, const std::string &)> callback);
    Element* get_focus_element() override { return text_input; }
};

class ConfigOptionRadio : public ConfigOptionElement {
protected:
    Radio *radio = nullptr;
    std::function<void(const std::string &, uint32_t)> callback;

    void index_changed(uint32_t index);
    std::string_view get_type_name() override { return "ConfigOptionRadio"; }
public:
    ConfigOptionRadio(Element *parent, uint32_t value, const std::vector<std::string> &options, std::function<void(const std::string &, uint32_t)> callback);
    Element* get_focus_element() override { return radio; }    
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
    ConfigOptionElement * description_option_element = nullptr;

    void back_button_pressed();
    void set_description_option_element(ConfigOptionElement *option, bool active);
    void add_option(ConfigOptionElement *option, std::string_view id, std::string_view name, std::string_view description);
protected:
    std::string_view get_type_name() override { return "ConfigSubMenu"; }
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
