#include "ui_element.h"
#include "../core/ui_context.h"

#include <cassert>

namespace recompui {

Element::Element(Rml::Element *base) {
    assert(base != nullptr);

    this->base = base;
    this->base_owning = {};
    this->shim = true;
}

Element::Element(Element* parent, uint32_t events_enabled, Rml::String base_class) {
    ContextId context = get_current_context();
    base_owning = context.get_document()->CreateElement(base_class);

    if (parent != nullptr) {
        base = parent->base->AppendChild(std::move(base_owning));
        parent->add_child(this);
    }
    else {
        base = base_owning.get();
    }

    register_event_listeners(events_enabled);
}

Element::~Element() {
    if (!shim) {
        clear_children();
        if (!base_owning) {
            base->GetParentNode()->RemoveChild(base);
        }
    }
}

void Element::add_child(Element *child) {
    assert(child != nullptr);

    children.emplace_back(child);

    if (shim) {
        ContextId context = get_current_context();
        context.add_loose_element(child);
    }
}

void Element::set_property(Rml::PropertyId property_id, const Rml::Property &property) {
    assert(base != nullptr);

    base->SetProperty(property_id, property);
    Style::set_property(property_id, property);
}

void Element::register_event_listeners(uint32_t events_enabled) {
    assert(base != nullptr);

    this->events_enabled = events_enabled;

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

    if (events_enabled & Events(EventType::Drag)) {
        base->AddEventListener(Rml::EventId::Drag, this);
        base->AddEventListener(Rml::EventId::Dragstart, this);
        base->AddEventListener(Rml::EventId::Dragend, this);
    }

    if (events_enabled & Events(EventType::Text)) {
        base->AddEventListener(Rml::EventId::Change, this);
    }
}

void Element::apply_style(Style *style) {
    for (auto it : style->property_map) {
        base->SetProperty(it.first, it.second);
    }
}

void Element::apply_styles() {
    apply_style(this);

    for (size_t i = 0; i < styles_counter.size(); i++) {
        if (styles_counter[i] == 0) {
            apply_style(styles[i]);
        }
    }
}

void Element::propagate_disabled(bool disabled) {
    disabled_from_parent = disabled;

    bool attribute_state = disabled_from_parent || !enabled;
    if (disabled_attribute != attribute_state) {
        disabled_attribute = attribute_state;
        base->SetAttribute("disabled", attribute_state);

        if (events_enabled & Events(EventType::Enable)) {
            process_event(Event::enable_event(!attribute_state));
        }

        for (auto &child : children) {
            child->propagate_disabled(attribute_state);
        }
    }
}

void Element::ProcessEvent(Rml::Event &event) {
    ContextId context = ContextId::null();
    Rml::ElementDocument* doc = event.GetTargetElement()->GetOwnerDocument();
    if (doc != nullptr) {
        context = get_context_from_document(doc);
    }

    // TODO disallow null contexts once the entire UI system has been migrated.
    if (context != ContextId::null()) {
        context.open();
    }

    // Events that are processed during any phase.
    switch (event.GetId()) {
    case Rml::EventId::Mousedown:
        process_event(Event::click_event(event.GetParameter("mouse_x", 0.0f), event.GetParameter("mouse_y", 0.0f)));
        break;
    case Rml::EventId::Drag:
        process_event(Event::drag_event(event.GetParameter("mouse_x", 0.0f), event.GetParameter("mouse_y", 0.0f), DragPhase::Move));
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
        case Rml::EventId::Dragstart:
            process_event(Event::drag_event(event.GetParameter("mouse_x", 0.0f), event.GetParameter("mouse_y", 0.0f), DragPhase::Start));
            break;
        case Rml::EventId::Dragend:
            process_event(Event::drag_event(event.GetParameter("mouse_x", 0.0f), event.GetParameter("mouse_y", 0.0f), DragPhase::End));
            break;
        case Rml::EventId::Change: {
            if (events_enabled & Events(EventType::Text)) {
                Rml::Variant *value_variant = base->GetAttribute("value");
                if (value_variant != nullptr) {
                    process_event(Event::text_event(value_variant->Get<std::string>()));
                }
            }

            break;
        }
        default:
            break;
        }
    }

    if (context != ContextId::null()) {
        context.close();
    }
}

void Element::set_attribute(const Rml::String &attribute_key, const Rml::String &attribute_value) {
    base->SetAttribute(attribute_key, attribute_value);
}

void Element::process_event(const Event &) {
    // Does nothing by default.
}

void Element::clear_children() {
    if (children.empty()) {
        return;
    }
    ContextId context = get_current_context();

    // Remove the children from the context.
    for (Element* child : children) {
        context.destroy_resource(child);
    }

    // Clear the child list.
    children.clear();
}

void Element::add_style(Style *style, const std::string_view style_name) {
    add_style(style, { style_name });
}

void Element::add_style(Style *style, const std::initializer_list<std::string_view> &style_names) {
    for (const std::string_view &style_name : style_names) {
        style_name_index_map.emplace(style_name, styles.size());
    }

    styles.emplace_back(style);

    uint32_t initial_style_counter = style_names.size();
    for (const std::string_view &style_name : style_names) {
        if (style_active_set.find(style_name) != style_active_set.end()) {
            initial_style_counter--;
        }
    }

    styles_counter.push_back(initial_style_counter);
}

void Element::set_enabled(bool enabled) {
    this->enabled = enabled;

    propagate_disabled(disabled_from_parent);
}

bool Element::is_enabled() const {
    return enabled && !disabled_from_parent;
}

void Element::set_text(std::string_view text) {
    base->SetInnerRML(std::string(text));
}

void Element::set_src(std::string_view src) {
    base->SetAttribute("src", std::string(src));
}

void Element::set_style_enabled(std::string_view style_name, bool enable) {
    if (enable && style_active_set.find(style_name) == style_active_set.end()) {
        // Style was disabled and will be enabled.
        style_active_set.emplace(style_name);

    }
    else if (!enable && style_active_set.find(style_name) != style_active_set.end()) {
        // Style was enabled and will be disabled.
        style_active_set.erase(style_name);
    }
    else {
        // Do nothing.
        return;
    }

    auto range = style_name_index_map.equal_range(style_name);
    for (auto it = range.first; it != range.second; it++) {
        if (enable) {
            styles_counter[it->second]--;
        }
        else {
            styles_counter[it->second]++;
        }
    }

    apply_styles();
}

float Element::get_absolute_left() {
    return base->GetAbsoluteLeft();
}

float Element::get_absolute_top() {
    return base->GetAbsoluteTop();
}

float Element::get_client_left() {
    return base->GetClientLeft();
}

float Element::get_client_top() {
    return base->GetClientTop();
}

float Element::get_client_width() {
    return base->GetClientWidth();
}

float Element::get_client_height() {
    return base->GetClientHeight();
}

void Element::queue_update() {
    ContextId cur_context = get_current_context();

    // TODO disallow null contexts once the entire UI system has been migrated.
    if (cur_context == ContextId::null()) {
        return;
    }    

    cur_context.queue_element_update(resource_id);
}

}