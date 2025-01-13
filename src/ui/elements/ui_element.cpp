#include "ui_element.h"

#include <cassert>

namespace recompui {

Element::Element(Rml::Element *base) {
    assert(base != nullptr);

    this->base = base;
    this->owner = false;
}

Element::Element(Element *parent, uint32_t events_enabled, Rml::String base_class) {
    owner = true;

    Rml::ElementPtr element = parent->base->GetOwnerDocument()->CreateElement(base_class);
    if (parent != nullptr) {
        base = parent->base->AppendChild(std::move(element));

        if (parent->owner) {
            parent->add_child(this);
        }
    }
    else {
        base = element.release();
        orphaned = true;
    }

    register_event_listeners(events_enabled);
}

Element::~Element() {
    children.clear();

    if (owner) {
        if (orphaned) {
            delete base;
        }
        else {
            base->GetParentNode()->RemoveChild(base);
        }
    }
}

void Element::add_child(Element *child) {
    assert(child != nullptr);

    children.emplace_back(child);
}

void Element::set_property(Rml::PropertyId property_id, const Rml::Property &property, Animation animation) {
    assert(base != nullptr);

    if (animation.type == AnimationType::None) {
        base->SetProperty(property_id, property);

        // Only non-animated properties should be stored as part of the style.
        Style::set_property(property_id, property, animation);
    }
    else {
        const Rml::String property_name = Rml::StyleSheetSpecification::GetPropertyName(property_id);
        base->Animate(property_name, property, animation.duration);
    }
}

void Element::register_event_listeners(uint32_t events_enabled) {
    assert(base != nullptr);

    if (events_enabled & Events(EventType::Click)) {
        base->AddEventListener(Rml::EventId::Mousedown, this);
    }

    if (events_enabled & Events(EventType::Focus)) {
        base->AddEventListener(Rml::EventId::Focus, this);
        base->AddEventListener(Rml::EventId::Blur, this);
    }

    if (events_enabled & Events(EventType::Hover)) {
        base->AddEventListener(Rml::EventId::Mouseover, this);
        base->AddEventListener(Rml::EventId::Mouseout, this);
    }
}

void Element::apply_style(Style *style) {
    for (auto it : style->property_map) {
        base->SetProperty(it.first, it.second);
    }
}

void Element::apply_styles() {
    apply_style(this);

    for (size_t i = 0; i < styles_active.size(); i++) {
        if (styles_active[i]) {
            apply_style(styles[i]);
        }
    }
}

void Element::ProcessEvent(Rml::Event &event) {
    // Events that are processed during any phase.
    switch (event.GetId()) {
    case Rml::EventId::Mousedown:
        process_event(Event::click_event(event.GetParameter("mouse_x", 0.0f), event.GetParameter("mouse_y", 0.0f)));
        break;
    default:
        break;
    }

    // Events that are only processed during the Target phase.
    if (event.GetPhase() == Rml::EventPhase::Target) {
        switch (event.GetId()) {
        case Rml::EventId::Mouseover:
            process_event(Event::hover_event(true));
            break;
        case Rml::EventId::Mouseout:
            process_event(Event::hover_event(false));
            break;
        case Rml::EventId::Focus:
            process_event(Event::focus_event(true));
            break;
        case Rml::EventId::Blur:
            process_event(Event::focus_event(false));
            break;
        default:
            break;
        }
    }
}

void Element::process_event(const Event &) {
    // Does nothing by default.
}

void Element::clear_children() {
    children.clear();
}

void Element::add_style(const std::string &style_name, Style *style) {
    assert(style_name_index_map.find(style_name) == style_name_index_map.end());

    style_name_index_map[style_name] = styles.size();
    styles.emplace_back(style);
    styles_active.push_back(false);
}

void Element::enable_style(const std::string &style_name, bool enable) {
    auto it = style_name_index_map.find(style_name);
    assert(it != style_name_index_map.end());

    styles_active[it->second] = enable;

    apply_styles();
}

void Element::set_text(const std::string &text) {
    base->SetInnerRML(text);
}

};