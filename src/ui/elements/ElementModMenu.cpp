#include "ElementModMenu.h"
#include "presets.h"
#include "librecomp/mods.hpp"

#include <string>

namespace recompui {

static const BEM mod_menu_bem("mod-menu");

static const std::string cls_base = mod_menu_bem.get_block(); 
static const std::string cls_modal_wrapper = mod_menu_bem.el("modal-wrapper"); 
static const std::string cls_modal_header = mod_menu_bem.el("modal-header"); 
static const std::string cls_modal_body = mod_menu_bem.el("modal-body"); 
static const std::string cls_list = mod_menu_bem.el("list"); 
static const std::string cls_list_scroll = mod_menu_bem.el("list-scroll"); 
static const std::string cls_details = mod_menu_bem.el("details"); 

static Rml::Element *add_div_with_class(Rml::ElementDocument *doc, Rml::Element *parent_el, const std::string& cls) {
    Rml::Element *el = parent_el->AppendChild(doc->CreateElement("div"));
    el->SetClass(cls.c_str(), true);
    return el;
}

ElementModMenu::ElementModMenu(const Rml::String& tag) : Rml::Element(tag)
{
    SetAttribute("recomp-store-element", true);
    Rml::ElementDocument *doc = GetOwnerDocument();
    SetClass(mod_menu_bem.block, true);

    {
        Rml::Element *modal_wrapper_el = add_div_with_class(doc, this, cls_modal_wrapper);
        {
            Rml::Element *header_el = add_div_with_class(doc, modal_wrapper_el, cls_modal_header);
            {
                add_button(doc, header_el, "Refresh", ButtonVariant::Primary);
                add_icon_button(doc, header_el, "icons/X.svg", ButtonVariant::Tertiary);
            }

            Rml::Element *body_el = add_div_with_class(doc, modal_wrapper_el, cls_modal_body);
            {
                Rml::Element *list_el = add_div_with_class(doc, body_el, cls_list);
                {
                    Rml::Element *list_el_scroll = add_div_with_class(doc, list_el, cls_list_scroll);
                    {
                        std::vector<recomp::mods::ModDetails> mods = recomp::mods::get_mod_details("mm");
                        for (auto& mod : mods) {
                            Rml::Element *mod_el = list_el_scroll->AppendChild(doc->CreateElement("div"));
                            mod_el->SetInnerRML(mod.mod_id);
                        }
                    } // list_el_scroll
                } // list_el

                Rml::Element *details_el = add_div_with_class(doc, body_el, cls_details);
                details_el->SetInnerRML("two");
            }
        }
    }
}

ElementModMenu::~ElementModMenu()
{
}


} // namespace Rml
