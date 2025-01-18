#ifndef RECOMPUI_CONFIG_SUB_MENU_H
#define RECOMPUI_CONFIG_SUB_MENU_H

#include <span>

#include "elements/ui_button.h"
#include "elements/ui_container.h"
#include "elements/ui_label.h"
#include "elements/ui_scroll_container.h"
#include "elements/ui_slider.h"

namespace recompui {

class ConfigOptionElement : public Element {
protected:
    Label *name_label = nullptr;
    std::string name;
    std::string description;
    std::function<void(ConfigOptionElement *, bool)> hover_callback = nullptr;

    virtual void process_event(const Event &e) override;
public:
    ConfigOptionElement(Element *parent);
    virtual ~ConfigOptionElement();
    void set_name(std::string_view name);
    void set_description(std::string_view description);
    void set_hover_callback(std::function<void(ConfigOptionElement *, bool)> callback);
    const std::string &get_description() const;
};

class ConfigOptionSlider : public ConfigOptionElement {
protected:
    Slider *slider = nullptr;

    void slider_value_changed(double v);
public:
    ConfigOptionSlider(Element *parent);
    virtual ~ConfigOptionSlider();
    void set_value(double v);
    void set_min_value(double v);
    void set_max_value(double v);
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
    std::function<void()> enter_sub_menu_callback = nullptr;
    std::function<void()> quit_sub_menu_callback = nullptr;
    std::vector<ConfigOptionElement *> config_option_elements;
    std::unordered_set<ConfigOptionElement *> hover_option_elements;

    void back_button_pressed();
    void option_hovered(ConfigOptionElement *option, bool active);
    void add_option(ConfigOptionElement *option, std::string_view name, std::string_view description);

public:
    ConfigSubMenu(Element *parent);
    virtual ~ConfigSubMenu();
    void enter(std::string_view title);
    void clear_options();
    void add_slider_option(std::string_view name, std::string_view description, double min, double max);
    void set_enter_sub_menu_callback(std::function<void()> callback);
    void set_quit_sub_menu_callback(std::function<void()> callback);
};

class ElementConfigSubMenu : public Rml::Element {
public:
    ElementConfigSubMenu(const Rml::String &tag);
    virtual ~ElementConfigSubMenu();
    void set_display(bool display);
    void set_enter_sub_menu_callback(std::function<void()> callback);
    void set_quit_sub_menu_callback(std::function<void()> callback);
    ConfigSubMenu *get_config_sub_menu_element() const;
private:
    std::unique_ptr<ConfigSubMenu> config_sub_menu;
};

}
#endif