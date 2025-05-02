#include "RmlUi/Core/StringUtilities.h"

#include "overloaded.h"
#include "recomp_ui.h"
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

Element::Element(Element* parent, uint32_t events_enabled, Rml::String base_class, bool can_set_text) : can_set_text(can_set_text) {
    ContextId context = get_current_context();
    base_owning = context.get_document()->CreateElement(base_class);

    if (parent != nullptr) {
        base = parent->base->AppendChild(std::move(base_owning));
        parent->add_child(this);
    }
    else {
        base = base_owning.get();
    }

    set_display(Display::Block);
    set_property(Rml::PropertyId::BoxSizing, Rml::Style::BoxSizing::BorderBox);

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
    if (can_set_text) {
        assert(false && "Elements with settable text cannot have children");
        return;
    }

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
        base->AddEventListener(Rml::EventId::Click, this);
    }

    if (events_enabled & Events(EventType::MouseButton)) {
        base->AddEventListener(Rml::EventId::Mousedown, this);
        base->AddEventListener(Rml::EventId::Mouseup, this);
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

    if (events_enabled & Events(EventType::Navigate)) {
        base->AddEventListener(Rml::EventId::Keydown, this);
    }
}

void Element::apply_style(Style *style) {
    for (auto it : style->property_map) {
        // Skip redundant SetProperty calls to prevent dirtying unnecessary state.
        // This avoids expensive layout operations when a simple color-only style is applied.
        const Rml::Property* cur_value = base->GetLocalProperty(it.first);
        if (*cur_value != it.second) {
            base->SetProperty(it.first, it.second);
        }
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
            handle_event(Event::enable_event(!attribute_state));
        }

        for (auto &child : children) {
            child->propagate_disabled(attribute_state);
        }
    }
}

void Element::handle_event(const Event& event) {
    for (const auto& callback : callbacks) {
        recompui::queue_ui_callback(resource_id, event, callback);
    }

    process_event(event);
}

void Element::set_id(const std::string& new_id) {
    id = new_id;
    base->SetId(new_id);
}

recompui::MouseButton convert_rml_mouse_button(int button) {
    switch (button) {
        case 0:
            return recompui::MouseButton::Left;
        case 1:
            return recompui::MouseButton::Right;
        case 2:
            return recompui::MouseButton::Middle;
        default:
            return recompui::MouseButton::Count;
    }
}

void Element::ProcessEvent(Rml::Event &event) {
    ContextId prev_context = recompui::try_close_current_context();
    ContextId context = ContextId::null();
    Rml::ElementDocument* doc = event.GetTargetElement()->GetOwnerDocument();
    if (doc != nullptr) {
        context = get_context_from_document(doc);
    }

    bool did_open = false;

    // TODO disallow null contexts once the entire UI system has been migrated.
    if (context != ContextId::null()) {
        did_open = context.open_if_not_already();
    }

    // Events that are processed during any phase.
    switch (event.GetId()) {
    case Rml::EventId::Click:
        handle_event(Event::click_event(event.GetParameter("mouse_x", 0.0f), event.GetParameter("mouse_y", 0.0f)));
        break;
    case Rml::EventId::Mousedown:
        {
            MouseButton mouse_button = convert_rml_mouse_button(event.GetParameter("button", 3));
            if (mouse_button != MouseButton::Count) {
                handle_event(Event::mousebutton_event(event.GetParameter("mouse_x", 0.0f), event.GetParameter("mouse_y", 0.0f), mouse_button, true));
            }
        }
        break;
    case Rml::EventId::Mouseup:
        {
            MouseButton mouse_button = convert_rml_mouse_button(event.GetParameter("button", 3));
            if (mouse_button != MouseButton::Count) {
                handle_event(Event::mousebutton_event(event.GetParameter("mouse_x", 0.0f), event.GetParameter("mouse_y", 0.0f), mouse_button, false));
            }
        }
        break;
    case Rml::EventId::Keydown:
        switch ((Rml::Input::KeyIdentifier)event.GetParameter<int>("key_identifier", 0)) {
            case Rml::Input::KeyIdentifier::KI_LEFT:
                handle_event(Event::navigate_event(NavDirection::Left));
                break;
            case Rml::Input::KeyIdentifier::KI_UP:
                handle_event(Event::navigate_event(NavDirection::Up));
                break;
            case Rml::Input::KeyIdentifier::KI_RIGHT:
                handle_event(Event::navigate_event(NavDirection::Right));
                break;
            case Rml::Input::KeyIdentifier::KI_DOWN:
                handle_event(Event::navigate_event(NavDirection::Down));
                break;
        }
        break;
    case Rml::EventId::Drag:
        handle_event(Event::drag_event(event.GetParameter("mouse_x", 0.0f), event.GetParameter("mouse_y", 0.0f), DragPhase::Move));
        break;
    default:
        break;
    }

    // Events that are only processed during the Target phase.
    if (event.GetPhase() == Rml::EventPhase::Target) {
        switch (event.GetId()) {
        case Rml::EventId::Mouseover:
            handle_event(Event::hover_event(true));
            break;
        case Rml::EventId::Mouseout:
            handle_event(Event::hover_event(false));
            break;
        case Rml::EventId::Focus:
            handle_event(Event::focus_event(true));
            break;
        case Rml::EventId::Blur:
            handle_event(Event::focus_event(false));
            break;
        case Rml::EventId::Dragstart:
            handle_event(Event::drag_event(event.GetParameter("mouse_x", 0.0f), event.GetParameter("mouse_y", 0.0f), DragPhase::Start));
            break;
        case Rml::EventId::Dragend:
            handle_event(Event::drag_event(event.GetParameter("mouse_x", 0.0f), event.GetParameter("mouse_y", 0.0f), DragPhase::End));
            break;
        case Rml::EventId::Change: {
            if (events_enabled & Events(EventType::Text)) {
                Rml::Variant *value_variant = base->GetAttribute("value");
                if (value_variant != nullptr) {
                    handle_event(Event::text_event(value_variant->Get<std::string>()));
                }
            }

            break;
        }
        default:
            break;
        }
    }

    if (context != ContextId::null() && did_open) {
        context.close();
    }

    if (prev_context != ContextId::null()) {
        prev_context.open();
    }
}

void Element::set_attribute(const Rml::String &attribute_key, const Rml::String &attribute_value) {
    base->SetAttribute(attribute_key, attribute_value);
}

void Element::process_event(const Event &) {
    // Does nothing by default.
}

void Element::enable_focus() {
    set_tab_index_auto();
    set_focusable(true);
    set_nav_auto(NavDirection::Up);
    set_nav_auto(NavDirection::Down);
    set_nav_auto(NavDirection::Left);
    set_nav_auto(NavDirection::Right);
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

bool Element::remove_child(ResourceId child) {
    bool found = false;

    ContextId context = get_current_context();

    for (auto it = children.begin(); it != children.end(); ++it) {
        Element* cur_child = *it;
        if (cur_child->get_resource_id() == child) {
            children.erase(it);
            context.destroy_resource(cur_child);
            found = true;
            break;
        }
    }

    return found;
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

// Adapted from RmlUi's `EncodeRml`.
std::string escape_rml(std::string_view string)
{
	std::string result;
	result.reserve(string.size());
	for (char c : string)
	{
		switch (c)
		{
		case '<': result += "&lt;"; break;
		case '>': result += "&gt;"; break;
		case '&': result += "&amp;"; break;
		case '"': result += "&quot;"; break;
        case '\n': result += "<br/>"; break;
		default: result += c; break;
		}
	}
	return result;
}

void Element::set_text(std::string_view text) {
    if (can_set_text) {
        // Queue the text update. If it's applied immediately, it might happen
        // while the document is being updated or rendered. This can cause a crash
        // due to the child elements being deleted while the document is being updated.
        // Queueing them defers it to the update thread, which prevents that issue.
        // Escape the string into Rml to prevent element injection.
        get_current_context().queue_set_text(this, escape_rml(text));
    }
    else {
        assert(false && "Attempted to set text of an element that cannot have its text set.");
    }
}

std::string Element::get_input_text() {
    return base->GetAttribute("value", std::string{});
}

void Element::set_input_text(std::string_view val) {
    base->SetAttribute("value", std::string{ val });
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

bool Element::is_style_enabled(std::string_view style_name) {
    return style_active_set.contains(style_name);
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

uint32_t Element::get_input_value_u32() {
    ElementValue value = get_element_value();
    
    return std::visit(overloaded {
        [](double d) { return (uint32_t)d; },
        [](float f) { return (uint32_t)f; },
        [](uint32_t u) { return u; },
        [](std::monostate) { return 0U; }
    }, value);
}

float Element::get_input_value_float() {
    ElementValue value = get_element_value();
    
    return std::visit(overloaded {
        [](double d) { return (float)d; },
        [](float f) { return f; },
        [](uint32_t u) { return (float)u; },
        [](std::monostate) { return 0.0f; }
    }, value);
}

double Element::get_input_value_double() {
    ElementValue value = get_element_value();
    
    return std::visit(overloaded {
        [](double d) { return d; },
        [](float f) { return (double)f; },
        [](uint32_t u) { return (double)u; },
        [](std::monostate) { return 0.0; }
    }, value);
}

void Element::focus() {
    base->Focus();
}

void Element::blur() {
    base->Blur();
}

void Element::queue_update() {
    ContextId cur_context = get_current_context();

    // TODO disallow null contexts once the entire UI system has been migrated.
    if (cur_context == ContextId::null()) {
        return;
    }    

    cur_context.queue_element_update(resource_id);
}

void Element::register_callback(ContextId context, PTR(void) callback, PTR(void) userdata) {
    callbacks.emplace_back(UICallback{.context = context, .callback = callback, .userdata = userdata});
}

}