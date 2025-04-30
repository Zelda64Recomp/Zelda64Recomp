#pragma once

#include <stdint.h>
#include <variant>

namespace recompui {

    constexpr std::string_view checked_state = "checked";
    constexpr std::string_view hover_state = "hover";
    constexpr std::string_view focus_state = "focus";
    constexpr std::string_view disabled_state = "disabled";

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

    // These two enums must be kept in sync with patches/recompui_event_structs.h!
    enum class EventType {
        None,
        Click,
        Focus,
        Hover,
        Enable,
        Drag,
        Text,
        Update,
        Navigate,
        MouseButton,
        Count
    };

    enum class DragPhase {
        None,
        Start,
        Move,
        End
    };

    enum class NavDirection {
        Up,
        Right,
        Down,
        Left
    };

    enum class MouseButton {
        Left,
        Right,
        Middle,
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

    struct EventNavigate {
        NavDirection direction;
    };

    struct EventMouseButton {
        float x;
        float y;
        MouseButton button;
        bool pressed;
    };

    using EventVariant = std::variant<EventClick, EventFocus, EventHover, EventEnable, EventDrag, EventText, EventNavigate, EventMouseButton, std::monostate>;

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

        static Event update_event() {
            Event e;
            e.type = EventType::Update;
            e.variant = std::monostate{};
            return e;
        }

        static Event navigate_event(NavDirection direction) {
            Event e;
            e.type = EventType::Navigate;
            e.variant = EventNavigate{ direction };
            return e;
        }

        static Event mousebutton_event(float x, float y, MouseButton button, bool pressed) {
            Event e;
            e.type = EventType::MouseButton;
            e.variant = EventMouseButton{ x, y, button, pressed };
            return e;
        }
    };

    enum class Display {
        None,
        Block,
        Inline,
        InlineBlock,
        FlowRoot,
        Flex,
        InlineFlex,
        Table,
        InlineTable,
        TableRow,
        TableRowGroup,
        TableColumn,
        TableColumnGroup,
        TableCell
    };

    enum class Visibility {
        Visible,
        Hidden
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
        Column,
        RowReverse,
        ColumnReverse
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
        Px,
        Dp,
        Percent
    };

    enum class AnimationType : uint32_t {
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

    enum class TextTransform {
        None,
        Capitalize,
        Uppercase,
        Lowercase
    };

    enum class Drag {
        None,
        Drag,
        DragDrop,
        Block,
        Clone
    };

    enum class TabIndex {
        None,
        Auto
    };

} // namespace recompui