#include "concurrentqueue.h"

#include "overloaded.h"
#include "recomp_ui.h"

#include "core/ui_context.h"
#include "core/ui_resource.h"

#include "elements/ui_element.h"
#include "elements/ui_button.h"
#include "elements/ui_clickable.h"
#include "elements/ui_container.h"
#include "elements/ui_image.h"
#include "elements/ui_label.h"
#include "elements/ui_radio.h"
#include "elements/ui_scroll_container.h"
#include "elements/ui_slider.h"
#include "elements/ui_style.h"
#include "elements/ui_text_input.h"
#include "elements/ui_toggle.h"
#include "elements/ui_types.h"

#include "librecomp/overlays.hpp"
#include "librecomp/helpers.hpp"

#include "../patches/ui_funcs.h"

struct QueuedCallback {
    recompui::ResourceId resource;
    recompui::Event event;
    recompui::UICallback callback;
};

moodycamel::ConcurrentQueue<QueuedCallback> queued_callbacks{};

void recompui::queue_ui_callback(recompui::ResourceId resource, const Event& e, const UICallback& callback) {
    queued_callbacks.enqueue(QueuedCallback{ .resource = resource, .event = e, .callback = callback });
}

bool convert_event(const recompui::Event& in, RecompuiEventData& out) {
    bool skip = false;
    out = {};
    out.type = static_cast<RecompuiEventType>(in.type);

    switch (in.type) {
        default:
        case recompui::EventType::None:
        case recompui::EventType::Count:
            skip = true;
            break;
        case recompui::EventType::Click:
            {
                const recompui::EventClick &click = std::get<recompui::EventClick>(in.variant);
                out.data.click.x = click.x;
                out.data.click.y = click.y;
            }
            break;
        case recompui::EventType::Focus:
            {
                const recompui::EventFocus &focus = std::get<recompui::EventFocus>(in.variant);
                out.data.focus.active = focus.active;
            }
            break;
        case recompui::EventType::Hover:
            {
                const recompui::EventHover &hover = std::get<recompui::EventHover>(in.variant);
                out.data.hover.active = hover.active;
            }
            break;
        case recompui::EventType::Enable:
            {
                const recompui::EventEnable &enable = std::get<recompui::EventEnable>(in.variant);
                out.data.enable.active = enable.active;
            }
            break;
        case recompui::EventType::Drag:
            {
                const recompui::EventDrag &drag = std::get<recompui::EventDrag>(in.variant);
                out.data.drag.phase = static_cast<RecompuiDragPhase>(drag.phase);
                out.data.drag.x = drag.x;
                out.data.drag.y = drag.y;
            }
            break;
        case recompui::EventType::Text:
            skip = true; // Text events aren't supported in the UI mod API.
            break;
        case recompui::EventType::Update:
            // No data for an update event.
            break;
    }

    return !skip;
}

extern "C" void recomp_run_ui_callbacks(uint8_t* rdram, recomp_context* ctx) {
    // Allocate the event on the stack.
    gpr stack_frame = ctx->r29;
    ctx->r29 -= sizeof(RecompuiEventData);
    RecompuiEventData* event_data = TO_PTR(RecompuiEventData, stack_frame);

    QueuedCallback cur_callback;

    while (queued_callbacks.try_dequeue(cur_callback)) {
        if (convert_event(cur_callback.event, *event_data)) {
            recompui::ContextId cur_context = cur_callback.callback.context;
            cur_context.open();

            ctx->r4 = static_cast<int32_t>(cur_callback.resource.slot_id);
            ctx->r5 = stack_frame;
            ctx->r6 = cur_callback.callback.userdata;

            LOOKUP_FUNC(cur_callback.callback.callback)(rdram, ctx);
            cur_context.close();
        }
    }

    ctx->r29 += sizeof(RecompuiEventData);
}
