#ifndef RECOMPUI_ELEMENT_MOD_MENU_H
#define RECOMPUI_ELEMENT_MOD_MENU_H

#include "librecomp/mods.hpp"
#include "elements/ui_scroll_container.h"
#include "ui_config_sub_menu.h"
#include "ui_mod_details_panel.h"

namespace recompui {

class ModMenu;

class ModEntryView : public Element {
public:
    ModEntryView(Element *parent);
    virtual ~ModEntryView();
    void set_mod_details(const recomp::mods::ModDetails &details);
    void set_mod_thumbnail(const std::string &thumbnail);
    void set_mod_enabled(bool enabled);
    void set_selected(bool selected);
private:
    Image *thumbnail_image = nullptr;
    Container *body_container = nullptr;
    Label *name_label = nullptr;
    Label *description_label = nullptr;
    Style checked_style;
    Style hover_style;
    Style checked_hover_style;
};

class ModEntryButton : public Element {
public:
    ModEntryButton(Element *parent, uint32_t mod_index);
    virtual ~ModEntryButton();
    void set_mod_selected_callback(std::function<void(uint32_t)> callback);
    void set_mod_drag_callback(std::function<void(uint32_t, EventDrag)> callback);
    void set_mod_details(const recomp::mods::ModDetails &details);
    void set_mod_thumbnail(const std::string &thumbnail);
    void set_mod_enabled(bool enabled);
    void set_selected(bool selected);
protected:
    virtual void process_event(const Event &e) override;
private:
    uint32_t mod_index = 0;
    ModEntryView *view = nullptr;
    std::function<void(uint32_t)> selected_callback = nullptr;
    std::function<void(uint32_t, EventDrag)> drag_callback = nullptr;
};

class ModEntrySpacer : public Element {
private:
    float height = 0.0f;
    float target_height = 0.0f;
    std::chrono::high_resolution_clock::duration last_time;

    void check_height_distance();
protected:
    virtual void process_event(const Event &e) override;
public:
    ModEntrySpacer(Element *parent);
    void set_target_height(float target_height, bool animate_to_target);
};

class ModMenu : public Element {
public:
    ModMenu(Element *parent);
    virtual ~ModMenu();
private:
    void refresh_mods();
    void open_mods_folder();
    void mod_toggled(bool enabled);
    void mod_selected(uint32_t mod_index);
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
    Container *body_empty_container = nullptr;
    Container *footer_container = nullptr;
    Button *refresh_button = nullptr;
    Button *mods_folder_button = nullptr;
    int32_t active_mod_index = -1;
    std::vector<ModEntryButton *> mod_entry_buttons;
    std::vector<ModEntrySpacer *> mod_entry_spacers;
    std::vector<float> mod_entry_middles;
    ModEntryView *mod_entry_floating_view = nullptr;
    float mod_drag_start_coordinates[2] = {};
    float mod_drag_view_coordinates[2] = {};
    uint32_t mod_drag_target_index = 0;
    std::vector<recomp::mods::ModDetails> mod_details{};
    std::unordered_set<std::string> loaded_thumbnails;
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
