#pragma once

#include <stdint.h>

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

            struct {
                bool enable;
            } enable;
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

        static Event enable_event(bool enable) {
            Event e = {};
            e.type = EventType::Enable;
            e.enable.enable = enable;
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

} // namespace recompui