#ifndef RECOMPUI_ELEMENT_MOD_MENU_H
#define RECOMPUI_ELEMENT_MOD_MENU_H

#include "librecomp/mods.hpp"
#include "elements/ui_scroll_container.h"
#include "ui_config_sub_menu.h"
#include "ui_mod_details_panel.h"

namespace recompui {

class ModMenu;

class ModEntry : public Element {
public:
    ModEntry(Element *parent, uint32_t mod_index, ModMenu *mod_menu);
    virtual ~ModEntry();
    void set_mod_drag_callback(std::function<void(uint32_t, EventDrag)> callback);
    void set_mod_details(const recomp::mods::ModDetails &details);
protected:
    virtual void process_event(const Event &e);
private:
    uint32_t mod_index = 0;
    ModMenu *mod_menu = nullptr;
    Image *thumbnail_image = nullptr;
    Container *body_container = nullptr;
    Label *name_label = nullptr;
    Label *description_label = nullptr;
    std::function<void(uint32_t, EventDrag)> drag_callback = nullptr;
};

class ModMenu : public Element {
public:
    ModMenu(Element *parent);
    virtual ~ModMenu();
    void set_active_mod(int32_t mod_index);
private:
    void refresh_mods();
    void mod_toggled(bool enabled);
    void mod_dragged(uint32_t mod_index, EventDrag drag);
    void mod_configure_requested();
    void mod_enum_option_changed(const std::string &id, uint32_t value);
    void mod_string_option_changed(const std::string &id, const std::string &value);
    void mod_number_option_changed(const std::string &id, double value);
    void create_mod_list();

    Container *body_container = nullptr;
    Container *list_container = nullptr;
    ScrollContainer *list_scroll_container = nullptr;
    ModDetailsPanel *mod_details_panel = nullptr;
    Container *footer_container = nullptr;
    Button *refresh_button = nullptr;
    int32_t active_mod_index = -1;
    std::vector<ModEntry *> mod_entries;
    std::vector<recomp::mods::ModDetails> mod_details{};
    std::string game_mod_id;

    ConfigSubMenu *config_sub_menu;
};

class ElementModMenu : public Rml::Element {
public:
    ElementModMenu(const Rml::String& tag);
    virtual ~ElementModMenu();
private:
    ModMenu *mod_menu;
};

} // namespace recompui
#endif
