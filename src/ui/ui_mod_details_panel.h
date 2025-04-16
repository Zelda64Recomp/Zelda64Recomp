#ifndef RECOMPUI_ELEMENT_MOD_DETAILS_PANEL_H
#define RECOMPUI_ELEMENT_MOD_DETAILS_PANEL_H

#include "librecomp/mods.hpp"
#include "elements/ui_button.h"
#include "elements/ui_container.h"
#include "elements/ui_image.h"
#include "elements/ui_label.h"
#include "elements/ui_toggle.h"
#include "elements/ui_scroll_container.h"

namespace recompui {

class ModDetailsPanel : public Element {
public:
    ModDetailsPanel(Element *parent);
    virtual ~ModDetailsPanel();
    void set_mod_details(const recomp::mods::ModDetails& details, const std::string &thumbnail, bool toggle_checked, bool toggle_enabled, bool toggle_label_visible, bool configure_enabled);
    void set_mod_toggled_callback(std::function<void(bool)> callback);
    void set_mod_configure_pressed_callback(std::function<void()> callback);
    void setup_mod_navigation(Element* nav_target);
    void clear_mod_navigation();
    Toggle* get_enable_toggle() { return enable_toggle; }
    Button* get_configure_button() { return configure_button; }
    void disable_toggle();
protected:
    std::string_view get_type_name() override { return "ModDetailsPanel"; }
private:
    recomp::mods::ModDetails cur_details;
    Container *thumbnail_container = nullptr;
    Image *thumbnail_image = nullptr;
    Container *header_container = nullptr;
    Container *header_details_container = nullptr;
    Label *title_label = nullptr;
    Label *version_label = nullptr;
    ScrollContainer *body_container = nullptr;
    Label *description_label = nullptr;
    Label *authors_label = nullptr;
    Container *buttons_container = nullptr;
    Container *enable_container = nullptr;
    Toggle *enable_toggle = nullptr;
    Label *enable_label = nullptr;
    Button *configure_button = nullptr;
    std::function<void(bool)> mod_toggled_callback = nullptr;
    std::function<void()> mod_configure_pressed_callback = nullptr;

    void enable_toggle_checked(bool checked);
    void configure_button_pressed();
};

} // namespace recompui
#endif
