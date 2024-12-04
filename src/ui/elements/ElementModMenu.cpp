#include "ElementModMenu.h"
#include "ElementModDetailsPanel.h"
#include "presets.h"
#include "librecomp/mods.hpp"

#include <string>

#define MOD_MENU_BEM "mod-menu"

namespace recompui {

static const std::string cls_base = BLOCK(MOD_MENU_BEM);
static const std::string cls_modal_wrapper = EL(MOD_MENU_BEM, "modal-wrapper");
static const std::string cls_modal_header = EL(MOD_MENU_BEM, "modal-header");
static const std::string cls_modal_body = EL(MOD_MENU_BEM, "modal-body");
static const std::string cls_list = EL(MOD_MENU_BEM, "list");
static const std::string cls_list_scroll = EL(MOD_MENU_BEM, "list-scroll");
static const std::string cls_list_entry = EL(MOD_MENU_BEM, "list-entry");
static const std::string cls_list_entry_thumbnail = EL(MOD_MENU_BEM, "list-entry-thumbnail");
static const std::string cls_list_entry_body = EL(MOD_MENU_BEM, "list-entry-body");
static const std::string cls_list_entry_name = EL(MOD_MENU_BEM, "list-entry-name");
static const std::string cls_list_entry_description = EL(MOD_MENU_BEM, "list-entry-description");

void ElementModMenu::ProcessEvent(Rml::Event& event) {
    Rml::Element* event_element = event.GetCurrentElement();
    Rml::EventId event_id = event.GetId();
    switch (event_id) {
        // Click event handlers.
        case Rml::EventId::Click:
            // Refresh
            if (event_element == refresh_button) {
                RefreshMods();
            }
            // Close
            else if (event_element == close_button) {

            }
            break;
        case Rml::EventId::Focus:
            {
                size_t mod_index;
                Rml::Variant *val = event_element->GetAttribute("mod_index");
                if (val->GetInto(mod_index) && mod_index < mod_details.size()) {
                    details_el->SetModDetails(mod_details[mod_index]);
                }
                if (active_list_entry_el != nullptr) {
                    active_list_entry_el->RemoveAttribute("is_selected");
                }
                event_element->SetAttribute("is_selected", true);
                active_list_entry_el = event_element;
            }
    }
}

Rml::ElementPtr ElementModMenu::CreateModListEntry(const recomp::mods::ModDetails& details, size_t index) {
    Rml::ElementDocument *doc = GetOwnerDocument();

    Rml::ElementPtr mod_el = doc->CreateElement("div");
    mod_el->SetClass(cls_list_entry, true);
    mod_el->SetAttribute("mod_index", index);
    {
        Rml::Element* thumbnail_el = add_div_with_class(doc, mod_el.get(), cls_list_entry_thumbnail);
        Rml::Element *body_el = add_div_with_class(doc, mod_el.get(), cls_list_entry_body);
        {
            Rml::Element *name_el = add_div_with_class(doc, body_el, cls_list_entry_name);
            name_el->SetInnerRML(details.mod_id);

            Rml::Element *description_el = add_div_with_class(doc, body_el, cls_list_entry_description);
            description_el->SetInnerRML("Short description of mod here.");
        } // body_el
    } // mod_el

    return mod_el;
}

void ElementModMenu::CreateModList() {
    Rml::ElementDocument *doc = GetOwnerDocument();

    // Clear the contents of the list scroll.
    list_el_scroll->SetInnerRML("");
    Rml::Element* prev_el = refresh_button;
    active_list_entry_el = nullptr;

    bool first = true;

    // Create the child elements for the list scroll.
    for (size_t mod_index = 0; mod_index < mod_details.size(); mod_index++) {
        const recomp::mods::ModDetails& details = mod_details[mod_index];
        Rml::Element *mod_el = list_el_scroll->AppendChild(CreateModListEntry(details, mod_index));
        mod_el->SetAttribute("mod_index", mod_index);
        mod_el->AddEventListener(Rml::EventId::Focus, this, false);
        mod_el->SetId("mod-list-entry-" + std::to_string(mod_index));

        mod_el->SetProperty("nav-up", "#" + prev_el->GetId());
        prev_el->SetProperty("nav-down", "#" + mod_el->GetId());

        if (first) {
            active_list_entry_el = mod_el;
        }
        first = false;

        prev_el = mod_el;
    }
    
    active_list_entry_el->SetAttribute("is_selected", true);

    DirtyLayout();
}

void ElementModMenu::RefreshMods() {
    recomp::mods::scan_mods();
    mod_details = recomp::mods::get_mod_details(game_mod_id);

    details_el->SetModDetails(mod_details[0]);

    CreateModList();
}

ElementModMenu::ElementModMenu(const Rml::String& tag) : Rml::Element(tag) {
    game_mod_id = "mm";
    SetAttribute("recomp-store-element", true);
    Rml::ElementDocument *doc = GetOwnerDocument();
    SetClass(cls_base, true);

    {
        Rml::Element *modal_wrapper_el = add_div_with_class(doc, this, cls_modal_wrapper);
        {
            Rml::Element *header_el = add_div_with_class(doc, modal_wrapper_el, cls_modal_header);
            {
                refresh_button = add_button(doc, header_el, "Refresh", ButtonVariant::Primary);
                refresh_button->AddEventListener(Rml::EventId::Click, this, false);
                refresh_button->SetId("refresh-button");
                close_button = add_icon_button(doc, header_el, "icons/X.svg", ButtonVariant::Tertiary);
                close_button->AddEventListener(Rml::EventId::Click, this, false);
                close_button->SetId("close-button");

                refresh_button->SetProperty("nav-right", "#" + close_button->GetId());
                close_button->SetProperty("nav-left", "#" + refresh_button->GetId());
            } // header_el

            Rml::Element *body_el = add_div_with_class(doc, modal_wrapper_el, cls_modal_body);
            {
                list_el = add_div_with_class(doc, body_el, cls_list);
                {
                    list_el_scroll = add_div_with_class(doc, list_el, cls_list_scroll);
                } // list_el

                details_el =
                    static_cast<ElementModDetailsPanel*>(body_el->AppendChild(doc->CreateElement("recomp-mod-details-panel")));
            } // body_el
        } // modal_wrapper_el
    }

    RefreshMods();
}

ElementModMenu::~ElementModMenu() {
}


} // namespace Rml
