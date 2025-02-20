#ifndef __UI_FUNCS_H__
#define __UI_FUNCS_H__

// These two enums must be kept in sync with src/ui/elements/ui_types.h!
typedef enum {
    UI_EVENT_NONE,
    UI_EVENT_CLICK,
    UI_EVENT_FOCUS,
    UI_EVENT_HOVER,
    UI_EVENT_ENABLE,
    UI_EVENT_DRAG,
    UI_EVENT_RESERVED1, // Would be UI_EVENT_TEXT but text events aren't usable in mods currently
    UI_EVENT_UPDATE,
    UI_EVENT_COUNT
} RecompuiEventType;

typedef enum {
    UI_DRAG_NONE,
    UI_DRAG_START,
    UI_DRAG_MOVE,
    UI_DRAG_END
} RecompuiDragPhase;

typedef struct {
    RecompuiEventType type;
    union {
        struct {
            float x;
            float y;
        } click;

        struct {
            bool active;
        } focus;

        struct {
            bool active;
        } hover;

        struct {
            bool active;
        } enable;

        struct {
            float x;
            float y;
            RecompuiDragPhase phase;
        } drag;
    } data;
} RecompuiEventData;

#endif
