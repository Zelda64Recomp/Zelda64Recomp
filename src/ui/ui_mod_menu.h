#ifndef RECOMPUI_ELEMENT_MOD_MENU_H
#define RECOMPUI_ELEMENT_MOD_MENU_H

#include "librecomp/mods.hpp"
#include "elements/ui_scroll_container.h"
#include "ui_mod_details_panel.h"

namespace recompui {

class ModMenu;

class ModEntry : public Element {
public:
    ModEntry(Element *parent, const recomp::mods::ModDetails &details, uint32_t mod_index, ModMenu *mod_menu);
    virtual ~ModEntry();
protected:
    virtual void process_event(const Event &e);
private:
    uint32_t mod_index;
    ModMenu *mod_menu;
    std::unique_ptr<Image> thumbnail_image;
    std::unique_ptr<Container> body_container;
    std::unique_ptr<Label> name_label;
    std::unique_ptr<Label> description_label;
};

class ModMenu : public Element {
public:
    ModMenu(Element *parent);
	virtual ~ModMenu();
    void set_active_mod(uint32_t mod_index);
private:
	void refresh_mods();
	void create_mod_list();
    
    std::unique_ptr<Container> body_container;
    std::unique_ptr<Container> list_container;
    std::unique_ptr<ScrollContainer> list_scroll_container;
	std::unique_ptr<ModDetailsPanel> mod_details_panel;
    std::unique_ptr<Container> footer_container;
    std::unique_ptr<Button> refresh_button;
    std::vector<std::unique_ptr<ModEntry>> mod_entries;
	std::vector<recomp::mods::ModDetails> mod_details{};
	std::string game_mod_id;
};

class ElementModMenu : public Rml::Element {
public:
    ElementModMenu(const Rml::String& tag);
    virtual ~ElementModMenu();
private:
    std::unique_ptr<ModMenu> mod_menu;
};

} // namespace recompui
#endif
