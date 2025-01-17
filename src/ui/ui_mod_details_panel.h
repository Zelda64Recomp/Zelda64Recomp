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
    void set_mod_details(const recomp::mods::ModDetails& details, bool enabled);
    void set_mod_toggled_callback(std::function<void(bool)> callback);
private:
    recomp::mods::ModDetails cur_details;
    Container *thumbnail_container = nullptr;
    Image *thumbnail_image = nullptr;
    Container *header_container = nullptr;
    Container *header_details_container = nullptr;
    Label *title_label = nullptr;
    Label *version_label = nullptr;
    Container *body_container = nullptr;
    Label *description_label = nullptr;
    Label *authors_label = nullptr;
    Container *buttons_container = nullptr;
    Toggle *enable_toggle = nullptr;
    Button *configure_button = nullptr;
    Button *erase_button = nullptr;
    std::function<void(bool)> mod_toggled_callback = {};

    void enable_toggle_checked(bool checked);
};

} // namespace recompui
#endif
