#pragma once

#include "RmlUi/Core.h"

namespace recompui {

struct Color {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
};

enum class Cursor {
    None,
    Pointer
};

enum class EventType {
    None,
    Click,
    Focus,
    Hover,
    Count
};

template <typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
constexpr uint32_t Events(Enum first) {
    return 1u << static_cast<uint32_t>(first);
}

template <typename Enum, typename... Enums, typename = std::enable_if_t<std::is_enum_v<Enum>>>
constexpr uint32_t Events(Enum first, Enums... rest) {
    return Events(first) | Events(rest...);
}

struct Event {
    struct Mouse {
        float x;
        float y;
    };

    EventType type;

    union {
        struct {
            Mouse mouse;
        } click;

        struct {
            bool active;
        } focus;

        struct {
            bool active;
        } hover;
    };

    static Event click_event(float x, float y) {
        Event e = {};
        e.type = EventType::Click;
        e.click.mouse.x = x;
        e.click.mouse.y = y;
        return e;
    }

    static Event focus_event(bool active) {
        Event e = {};
        e.type = EventType::Focus;
        e.focus.active = active;
        return e;
    }

    static Event hover_event(bool active) {
        Event e = {};
        e.type = EventType::Hover;
        e.focus.active = active;
        return e;
    }
};

enum class Display {
    Block,
    Flex
};

enum class Position {
    Absolute,
    Relative
};

enum class JustifyContent {
    FlexStart,
    FlexEnd,
    Center,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
};

enum class FlexDirection {
    Row,
    Column
};

enum class Overflow {
    Visible,
    Hidden,
    Auto,
    Scroll
};

enum class Unit {
    Float,
    Dp,
    Percent
};

enum class AnimationType {
    None,
    Tween
};

enum class FontStyle {
    Normal,
    Italic
};

struct Animation {
    AnimationType type = AnimationType::None;
    float duration = 0.0f;

    static Animation tween(float duration) {
        Animation a;
        a.type = AnimationType::Tween;
        a.duration = duration;
        return a;
    }
};

class Element : public Rml::EventListener {
private:
    void set_property(Rml::PropertyId property_id, const Rml::Property &property, Animation animation = Animation());
    void register_event_listeners(uint32_t events_enabled);

    // Rml::EventListener overrides.
    virtual void ProcessEvent(Rml::Event &event) override;
protected:
    Element *parent;
    Rml::Element *base;
    bool owner;

    virtual void process_event(const Event &e);
public:
    // Used for backwards compatibility with legacy UI elements.
    Element(Rml::Element *base);

    // Used to actually construct elements.
    Element(Element *parent, uint32_t events_enabled = 0, Rml::String base_class = "div");
    virtual ~Element();
    void set_position(Position position);
    void set_left(float left, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_top(float top, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_right(float right, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_bottom(float bottom, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_width(float width, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_width_auto();
    void set_height(float height, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_height_auto();
    void set_padding(float padding, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_padding_left(float padding, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_padding_top(float padding, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_padding_right(float padding, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_padding_bottom(float padding, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_margin(float margin, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_margin_left(float margin, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_margin_top(float margin, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_margin_right(float margin, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_margin_bottom(float margin, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_border_width(float width, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_border_radius(float radius, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_border_top_left_radius(float radius, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_border_top_right_radius(float radius, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_border_bottom_left_radius(float radius, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_border_bottom_right_radius(float radius, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_background_color(const Color &color, Animation animation = Animation());
    void set_border_color(const Color &color, Animation animation = Animation());
    void set_color(const Color &color, Animation animation = Animation());
    void set_cursor(Cursor cursor);
    void set_opacity(float opacity, Animation animation = Animation());
    void set_display(Display display);
    void set_justify_content(JustifyContent justify_content);
    void set_flex_grow(float grow, Animation animation = Animation());
    void set_flex_shrink(float shrink, Animation animation = Animation());
    void set_flex_basis_auto();
    void set_flex_basis_percentage(float basis_percentage, Animation animation = Animation());
    void set_flex(float grow, float shrink, Animation animation = Animation());
    void set_flex(float grow, float shrink, float basis_percentage, Animation animation = Animation());
    void set_flex_direction(FlexDirection flex_direction);
    void set_overflow(Overflow overflow);
    void set_overflow_x(Overflow overflow);
    void set_overflow_y(Overflow overflow);
    void set_font_size(float size, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_letter_spacing(float spacing, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_line_height(float height, Unit unit = Unit::Dp, Animation animation = Animation());
    void set_font_style(FontStyle style);
    void set_font_weight(uint32_t weight, Animation animation = Animation());
    void set_text(const std::string &text);
};

} // namespace recompui