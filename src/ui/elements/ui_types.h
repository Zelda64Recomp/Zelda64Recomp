#pragma once

#include <stdint.h>
#include <variant>

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
        Enable,
        Drag,
        Text,
        Count
    };

    enum class DragPhase {
        None,
        Start,
        Move,
        End
    };

    template <typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
    constexpr uint32_t Events(Enum first) {
        return 1u << static_cast<uint32_t>(first);
    }

    template <typename Enum, typename... Enums, typename = std::enable_if_t<std::is_enum_v<Enum>>>
    constexpr uint32_t Events(Enum first, Enums... rest) {
        return Events(first) | Events(rest...);
    }

    struct EventClick {
        float x;
        float y;
    };

    struct EventFocus {
        bool active;
    };

    struct EventHover {
        bool active;
    };

    struct EventEnable {
        bool active;
    };

    struct EventDrag {
        float x;
        float y;
        DragPhase phase;
    };

    struct EventText {
        std::string text;
    };

    using EventVariant = std::variant<EventClick, EventFocus, EventHover, EventEnable, EventDrag, EventText>;

    struct Event {
        EventType type;
        EventVariant variant;

        // Factory methods for creating specific events
        static Event click_event(float x, float y) {
            Event e;
            e.type = EventType::Click;
            e.variant = EventClick{ x, y };
            return e;
        }

        static Event focus_event(bool active) {
            Event e;
            e.type = EventType::Focus;
            e.variant = EventFocus{ active };
            return e;
        }

        static Event hover_event(bool active) {
            Event e;
            e.type = EventType::Hover;
            e.variant = EventHover{ active };
            return e;
        }

        static Event enable_event(bool enable) {
            Event e;
            e.type = EventType::Enable;
            e.variant = EventEnable{ enable };
            return e;
        }

        static Event drag_event(float x, float y, DragPhase phase) {
            Event e;
            e.type = EventType::Drag;
            e.variant = EventDrag{ x, y, phase };
            return e;
        }

        static Event text_event(const std::string &text) {
            Event e;
            e.type = EventType::Text;
            e.variant = EventText{ text };
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

    enum class AlignItems {
        FlexStart,
        FlexEnd,
        Center,
        Baseline,
        Stretch
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
        Set,
        Tween
    };

    enum class FontStyle {
        Normal,
        Italic
    };

    enum class TextAlign {
        Left,
        Right,
        Center,
        Justify
    };

    enum class Drag {
        None,
        Drag,
        DragDrop,
        Block,
        Clone
    };

    struct Animation {
        AnimationType type = AnimationType::None;
        float duration = 0.0f;

        static Animation set() {
            Animation a;
            a.type = AnimationType::Set;
            return a;
        }

        static Animation tween(float duration) {
            Animation a;
            a.type = AnimationType::Tween;
            a.duration = duration;
            return a;
        }
    };

} // namespace recompui