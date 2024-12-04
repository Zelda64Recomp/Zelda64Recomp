#ifndef RECOMPUI_ELEMENT_MOD_MENU_H
#define RECOMPUI_ELEMENT_MOD_MENU_H

#include "common.h"
#include "librecomp/mods.hpp"
#include "ElementModDetailsPanel.h"

namespace recompui {

class ElementModMenu : public Rml::Element, public Rml::EventListener {
public:
	ElementModMenu(const Rml::String& tag);
	virtual ~ElementModMenu();
	void ProcessEvent(Rml::Event& event) final;
private:
	void RefreshMods();
	void CreateModList();
	Rml::ElementPtr CreateModListEntry(const recomp::mods::ModDetails& details, size_t index);
	Rml::Element *refresh_button;
	Rml::Element *close_button;
	Rml::Element *list_el; // The root mod list element.
	Rml::Element *list_el_scroll; // The scroll within the root mod list element.
	ElementModDetailsPanel *details_el; // The details panel.
	Rml::Element *active_list_entry_el = nullptr;
	std::vector<recomp::mods::ModDetails> mod_details{};
	std::string game_mod_id;
};

} // namespace recompui
#endif
