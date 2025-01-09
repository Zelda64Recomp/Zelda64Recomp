#include "ui_element.h"

#include <cassert>

namespace recompui {

static Rml::Unit to_rml(Unit unit) {
    switch (unit) {
    case Unit::Float:
        return Rml::Unit::NUMBER;
    case Unit::Dp:
        return Rml::Unit::DP;
    case Unit::Percent:
        return Rml::Unit::PERCENT;
    default:
        return Rml::Unit::UNKNOWN;
    }
}

static Rml::Style::Overflow to_rml(Overflow overflow) {
    switch (overflow) {
    case Overflow::Visible:
        return Rml::Style::Overflow::Visible;
    case Overflow::Hidden:
        return Rml::Style::Overflow::Hidden;
    case Overflow::Auto:
        return Rml::Style::Overflow::Auto;
    case Overflow::Scroll:
        return Rml::Style::Overflow::Scroll;
    default:
        assert(false && "Unknown overflow.");
        return Rml::Style::Overflow::Visible;
    }
}

Element::Element(Rml::Element *base) {
    assert(base != nullptr);

    this->base = base;
    this->parent = nullptr;
    this->owner = false;
}

Element::Element(Element *parent, uint32_t events_enabled, Rml::String base_class) {
    assert(parent != nullptr);

    this->parent = parent;
    this->owner = true;

    base = parent->base->AppendChild(parent->base->GetOwnerDocument()->CreateElement(base_class));

    register_event_listeners(events_enabled);
}

Element::~Element() {
    if (owner) {
        base->GetParentNode()->RemoveChild(base);
    }
}

void Element::set_property(Rml::PropertyId property_id, const Rml::Property &property, Animation animation) {
    assert(base != nullptr);

    if (animation.type == AnimationType::None) {
        base->SetProperty(property_id, property);
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

void Element::set_position(Position position) {
    switch (position) {
    case Position::Absolute:
        set_property(Rml::PropertyId::Position, Rml::Style::Position::Absolute);
        break;
    case Position::Relative:
        set_property(Rml::PropertyId::Position, Rml::Style::Position::Relative);
        break;
    default:
        assert(false && "Unknown position.");
        break;
    }
}

void Element::set_left(float left, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::Left, Rml::Property(left, to_rml(unit)), animation);
}

void Element::set_top(float top, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::Top, Rml::Property(top, to_rml(unit)), animation);
}

void Element::set_right(float right, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::Right, Rml::Property(right, to_rml(unit)), animation);
}

void Element::set_bottom(float bottom, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::Bottom, Rml::Property(bottom, to_rml(unit)), animation);
}

void Element::set_width(float width, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::Width, Rml::Property(width, to_rml(unit)), animation);
}

void Element::set_width_auto() {
    set_property(Rml::PropertyId::Width, Rml::Property(Rml::Style::FlexBasis::Type::Auto, Rml::Unit::KEYWORD));
}

void Element::set_height(float height, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::Height, Rml::Property(height, to_rml(unit)), animation);
}

void Element::set_height_auto() {
    set_property(Rml::PropertyId::Height, Rml::Property(Rml::Style::FlexBasis::Type::Auto, Rml::Unit::KEYWORD));
}

void Element::set_padding(float padding, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::PaddingLeft, Rml::Property(padding, to_rml(unit)), animation);
    set_property(Rml::PropertyId::PaddingTop, Rml::Property(padding, to_rml(unit)), animation);
    set_property(Rml::PropertyId::PaddingRight, Rml::Property(padding, to_rml(unit)), animation);
    set_property(Rml::PropertyId::PaddingBottom, Rml::Property(padding, to_rml(unit)), animation);
}

void Element::set_padding_left(float padding, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::PaddingLeft, Rml::Property(padding, to_rml(unit)), animation);
}

void Element::set_padding_top(float padding, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::PaddingTop, Rml::Property(padding, to_rml(unit)), animation);
}

void Element::set_padding_right(float padding, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::PaddingRight, Rml::Property(padding, to_rml(unit)), animation);
}

void Element::set_padding_bottom(float padding, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::PaddingBottom, Rml::Property(padding, to_rml(unit)), animation);
}

void Element::set_margin(float margin, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::MarginLeft, Rml::Property(margin, to_rml(unit)), animation);
    set_property(Rml::PropertyId::MarginTop, Rml::Property(margin, to_rml(unit)), animation);
    set_property(Rml::PropertyId::MarginRight, Rml::Property(margin, to_rml(unit)), animation);
    set_property(Rml::PropertyId::MarginBottom, Rml::Property(margin, to_rml(unit)), animation);
}

void Element::set_margin_left(float margin, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::MarginLeft, Rml::Property(margin, to_rml(unit)), animation);
}

void Element::set_margin_top(float margin, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::MarginTop, Rml::Property(margin, to_rml(unit)), animation);
}

void Element::set_margin_right(float margin, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::MarginRight, Rml::Property(margin, to_rml(unit)), animation);
}

void Element::set_margin_bottom(float margin, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::MarginBottom, Rml::Property(margin, to_rml(unit)), animation);
}

void Element::set_border_width(float width, Unit unit, Animation animation) {
    Rml::Property property(width, to_rml(unit));
    set_property(Rml::PropertyId::BorderTopWidth, property, animation);
    set_property(Rml::PropertyId::BorderBottomWidth, property, animation);
    set_property(Rml::PropertyId::BorderLeftWidth, property, animation);
    set_property(Rml::PropertyId::BorderRightWidth, property, animation);
}

void Element::set_border_radius(float radius, Unit unit, Animation animation) {
    Rml::Property property(radius, to_rml(unit));
    set_property(Rml::PropertyId::BorderTopLeftRadius, property, animation);
    set_property(Rml::PropertyId::BorderTopRightRadius, property, animation);
    set_property(Rml::PropertyId::BorderBottomLeftRadius, property, animation);
    set_property(Rml::PropertyId::BorderBottomRightRadius, property, animation);
}

void Element::set_border_top_left_radius(float radius, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::BorderTopLeftRadius, Rml::Property(radius, to_rml(unit)), animation);
}

void Element::set_border_top_right_radius(float radius, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::BorderTopRightRadius, Rml::Property(radius, to_rml(unit)), animation);
}

void Element::set_border_bottom_left_radius(float radius, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::BorderBottomLeftRadius, Rml::Property(radius, to_rml(unit)), animation);
}

void Element::set_border_bottom_right_radius(float radius, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::BorderBottomRightRadius, Rml::Property(radius, to_rml(unit)), animation);
}

void Element::set_background_color(const Color &color, Animation animation) {
    Rml::Property property(Rml::Colourb(color.r, color.g, color.b, color.a), Rml::Unit::COLOUR);
    set_property(Rml::PropertyId::BackgroundColor, property, animation);
}

void Element::set_border_color(const Color &color, Animation animation) {
    Rml::Property property(Rml::Colourb(color.r, color.g, color.b, color.a), Rml::Unit::COLOUR);
    set_property(Rml::PropertyId::BorderTopColor, property, animation);
    set_property(Rml::PropertyId::BorderBottomColor, property, animation);
    set_property(Rml::PropertyId::BorderLeftColor, property, animation);
    set_property(Rml::PropertyId::BorderRightColor, property, animation);
}

void Element::set_color(const Color &color, Animation animation) {
    Rml::Property property(Rml::Colourb(color.r, color.g, color.b, color.a), Rml::Unit::COLOUR);
    set_property(Rml::PropertyId::Color, property, animation);
}

void Element::set_cursor(Cursor cursor) {
    switch (cursor) {
    case Cursor::None:
        assert(false && "Unimplemented.");
        break;
    case Cursor::Pointer:
        set_property(Rml::PropertyId::Cursor, Rml::Property("pointer", Rml::Unit::STRING));
        break;
    default:
        assert(false && "Unknown cursor.");
        break;
    }
}

void Element::set_opacity(float opacity, Animation animation) {
    set_property(Rml::PropertyId::Opacity, Rml::Property(opacity, Rml::Unit::NUMBER), animation);
}

void Element::set_display(Display display) {
    switch (display) {
    case Display::Block:
        set_property(Rml::PropertyId::Display, Rml::Style::Display::Block);
        break;
    case Display::Flex:
        set_property(Rml::PropertyId::Display, Rml::Style::Display::Flex);
        break;
    default:
        assert(false && "Unknown display.");
        break;
    }
}

void Element::set_justify_content(JustifyContent justify_content) {
    switch (justify_content) {
    case JustifyContent::FlexStart:
        set_property(Rml::PropertyId::JustifyContent, Rml::Style::JustifyContent::FlexStart);
        break;
    case JustifyContent::FlexEnd:
        set_property(Rml::PropertyId::JustifyContent, Rml::Style::JustifyContent::FlexEnd);
        break;
    case JustifyContent::Center:
        set_property(Rml::PropertyId::JustifyContent, Rml::Style::JustifyContent::Center);
        break;
    case JustifyContent::SpaceBetween:
        set_property(Rml::PropertyId::JustifyContent, Rml::Style::JustifyContent::SpaceBetween);
        break;
    case JustifyContent::SpaceAround:
        set_property(Rml::PropertyId::JustifyContent, Rml::Style::JustifyContent::SpaceAround);
        break;
    case JustifyContent::SpaceEvenly:
        set_property(Rml::PropertyId::JustifyContent, Rml::Style::JustifyContent::SpaceEvenly);
        break;
    default:
        assert(false && "Unknown justify content.");
        break;
    }
}

void Element::set_flex_grow(float grow, Animation animation) {
    set_property(Rml::PropertyId::FlexGrow, Rml::Property(grow, Rml::Unit::NUMBER), animation);
}

void Element::set_flex_shrink(float shrink, Animation animation) {
    set_property(Rml::PropertyId::FlexShrink, Rml::Property(shrink, Rml::Unit::NUMBER), animation);
}

void Element::set_flex_basis_auto() {
    set_property(Rml::PropertyId::FlexBasis, Rml::Property(Rml::Style::FlexBasis::Type::Auto, Rml::Unit::KEYWORD));
}

void Element::set_flex_basis_percentage(float basis, Animation animation) {
    set_property(Rml::PropertyId::FlexBasis, Rml::Property(basis, Rml::Unit::PERCENT), animation);
}

void Element::set_flex(float grow, float shrink, Animation animation) {
    set_flex_grow(grow, animation);
    set_flex_shrink(shrink, animation);
    set_flex_basis_auto();
}

void Element::set_flex(float grow, float shrink, float basis_percentage, Animation animation) {
    set_flex_grow(grow, animation);
    set_flex_shrink(shrink, animation);
    set_flex_basis_percentage(basis_percentage, animation);
}

void Element::set_flex_direction(FlexDirection flex_direction) {
    switch (flex_direction) {
        case FlexDirection::Row:
            set_property(Rml::PropertyId::FlexDirection, Rml::Style::FlexDirection::Row);
            break;
        case FlexDirection::Column:
            set_property(Rml::PropertyId::FlexDirection, Rml::Style::FlexDirection::Column);
            break;
        default:
            assert(false && "Unknown flex direction.");
            break;
    }
}

void Element::set_overflow(Overflow overflow) {
    set_property(Rml::PropertyId::OverflowX, to_rml(overflow));
    set_property(Rml::PropertyId::OverflowY, to_rml(overflow));
}

void Element::set_overflow_x(Overflow overflow) {
    set_property(Rml::PropertyId::OverflowX, to_rml(overflow));
}

void Element::set_overflow_y(Overflow overflow) {
    set_property(Rml::PropertyId::OverflowY, to_rml(overflow));
}

void Element::set_font_size(float size, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::FontSize, Rml::Property(size, to_rml(unit)), animation);
}

void Element::set_letter_spacing(float spacing, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::LetterSpacing, Rml::Property(spacing, to_rml(unit)), animation);
}

void Element::set_line_height(float height, Unit unit, Animation animation) {
    set_property(Rml::PropertyId::LineHeight, Rml::Property(height, to_rml(unit)), animation);
}

void Element::set_font_style(FontStyle style) {
    switch (style) {
    case FontStyle::Normal:
        set_property(Rml::PropertyId::FontStyle, Rml::Style::FontStyle::Normal);
        break;
    case FontStyle::Italic:
        set_property(Rml::PropertyId::FontStyle, Rml::Style::FontStyle::Italic);
        break;
    default:
        assert(false && "Unknown font style.");
        break;
    }
}

void Element::set_font_weight(uint32_t weight, Animation animation) {
    set_property(Rml::PropertyId::FontWeight, Rml::Style::FontWeight(weight), animation);
}

void Element::set_text(const std::string &text) {
    base->SetInnerRML(text);
}

};