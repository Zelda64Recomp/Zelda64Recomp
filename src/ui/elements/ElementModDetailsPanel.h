#ifndef RECOMPUI_ELEMENT_MOD_DETAILS_PANEL_H
#define RECOMPUI_ELEMENT_MOD_DETAILS_PANEL_H

#include "common.h"
#include "librecomp/mods.hpp"

namespace recompui {

class ElementModDetailsPanel : public Rml::Element {
public:
	ElementModDetailsPanel(const Rml::String& tag);
	virtual ~ElementModDetailsPanel();
    void SetModDetails(const recomp::mods::ModDetails& details);
private:
    recomp::mods::ModDetails cur_details;
    Rml::Element* thumbnail_el;
    Rml::Element* title_el;
    Rml::Element* authors_el;
    Rml::Element* version_el;
    Rml::Element* description_el;
};

} // namespace recompui
#endif
