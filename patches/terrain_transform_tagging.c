#include "patches.h"
#include "transform_ids.h"

static Vec3f sZeroVec = { 0.0f, 0.0f, 0.0f };

extern RoomDrawHandler sRoomDrawHandlers[];

void Room_Draw(PlayState* play, Room* room, u32 flags) {
    if (room->segment != NULL) {
        gSegments[3] = OS_K0_TO_PHYSICAL(room->segment);
        
        OPEN_DISPS(play->state.gfxCtx);

        // @recomp Tag the room's matrices if applicable.
        // Tag terrain as being ignored for interpolation, which prevents interpolation glitches where some pieces of terrain swap places when one comes into view.
        if (flags & ROOM_DRAW_OPA) {
            gEXMatrixGroup(POLY_OPA_DISP++, G_EX_ID_IGNORE, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_ORDER_LINEAR);
        }
        if (flags & ROOM_DRAW_XLU) {
            gEXMatrixGroup(POLY_XLU_DISP++, G_EX_ID_IGNORE, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_ORDER_LINEAR);
        }

        CLOSE_DISPS(play->state.gfxCtx);

        sRoomDrawHandlers[room->roomShape->base.type](play, room, flags);
        
        OPEN_DISPS(play->state.gfxCtx);

        // @recomp Pop the room's matrix tags if applicable.
        if (flags & ROOM_DRAW_OPA) {
            gEXPopMatrixGroup(POLY_OPA_DISP++);
        }
        if (flags & ROOM_DRAW_XLU) {
            gEXPopMatrixGroup(POLY_XLU_DISP++);
        }

        CLOSE_DISPS(play->state.gfxCtx);
    }
    return;
}

