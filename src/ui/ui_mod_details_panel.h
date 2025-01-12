#ifndef RECOMPUI_ELEMENT_MOD_DETAILS_PANEL_H
#define RECOMPUI_ELEMENT_MOD_DETAILS_PANEL_H

#include "librecomp/mods.hpp"
#include "elements/ui_button.h"
#include "elements/ui_container.h"
#include "elements/ui_image.h"
#include "elements/ui_label.h"
#include "elements/ui_toggle.h"

namespace recompui {

class ModDetailsPanel : public Element {
public:
    ModDetailsPanel(Element *parent);
	virtual ~ModDetailsPanel();
    void set_mod_details(const recomp::mods::ModDetails& details);
private:
    recomp::mods::ModDetails cur_details;
    std::unique_ptr<recompui::Container> thumbnail_container;
    std::unique_ptr<recompui::Image> thumbnail_image;
    std::unique_ptr<recompui::Container> header_container;
    std::unique_ptr<recompui::Container> header_details_container;
    std::unique_ptr<recompui::Label> title_label;
    std::unique_ptr<recompui::Label> version_label;
    std::unique_ptr<recompui::Container> body_container;
    std::unique_ptr<recompui::Label> description_label;
    std::unique_ptr<recompui::Label> authors_label;
    std::unique_ptr<recompui::Container> buttons_container;
    std::unique_ptr<recompui::Toggle> enable_toggle;
    std::unique_ptr<recompui::Button> configure_button;
    std::unique_ptr<recompui::Button> erase_button;
};

} // namespace recompui
#endif
