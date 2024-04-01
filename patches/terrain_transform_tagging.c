#include "patches.h"
#include "transform_ids.h"
#include "overlays/actors/ovl_Dm_Opstage/z_dm_opstage.h"

static Vec3f sZeroVec = { 0.0f, 0.0f, 0.0f };

extern RoomDrawHandler sRoomDrawHandlers[];

void Room_Draw(PlayState* play, Room* room, u32 flags) {
    if (room->segment != NULL) {
        gSegments[3] = OS_K0_TO_PHYSICAL(room->segment);
        
        OPEN_DISPS(play->state.gfxCtx);

        // @recomp Tag the room's matrices if applicable.
        // Tag terrain as being ignored for interpolation, which prevents interpolation glitches where some pieces of terrain swap places when one comes into view.
        if (flags & ROOM_DRAW_OPA) {
            gEXMatrixGroupInterpolateOnlyTiles(POLY_OPA_DISP++, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
        }
        if (flags & ROOM_DRAW_XLU) {
            gEXMatrixGroupInterpolateOnlyTiles(POLY_XLU_DISP++, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
        }

        CLOSE_DISPS(play->state.gfxCtx);

        sRoomDrawHandlers[room->roomShape->base.type](play, room, flags);
        
        OPEN_DISPS(play->state.gfxCtx);

        // @recomp Pop the room's matrix tags if applicable.
        if (flags & ROOM_DRAW_OPA) {
            gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);
        }
        if (flags & ROOM_DRAW_XLU) {
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        }

        CLOSE_DISPS(play->state.gfxCtx);
    }
    return;
}

extern Gfx gKeikokuDemoFloorDL[];
extern Gfx gKeikokuDemoFloorEmptyDL[];
extern Gfx gKeikokuDemoTallTreeWithRootBaseDL[];
extern Gfx gKeikokuDemoTallTreeWithRootBaseEmptyDL[];
extern Gfx gKeikokuDemoTallTreeCutDL[];
extern Gfx gKeikokuDemoTallTreeCutEmptyDL[];
extern Gfx gKeikokuDemoTallTreeStraightDL[];
extern Gfx gKeikokuDemoTallTreeStraightEmptyDL[];

// @recomp Tag the ground in the intro cutscene to not interpolate rotation.
void DmOpstage_Draw(Actor* thisx, PlayState* play) {
    DmOpstage* this = (DmOpstage*)thisx;

    if (DMOPSTAGE_GET_TYPE(&this->dyna.actor) > DMOPSTAGE_TYPE_GROUND) {
        // Assumption: worldPos is being manipulated by cutscene
        Matrix_Translate(this->dyna.actor.world.pos.x + this->drawOffset.x,
                         this->dyna.actor.world.pos.y + this->drawOffset.y,
                         this->dyna.actor.world.pos.z + this->drawOffset.z, MTXMODE_NEW);
        Matrix_RotateYS(this->dyna.actor.world.rot.y, MTXMODE_APPLY);
        Matrix_Scale(0.1f, 0.1f, 0.1f, MTXMODE_APPLY);
    }

    switch (DMOPSTAGE_GET_TYPE(&this->dyna.actor)) {
        case DMOPSTAGE_TYPE_GROUND:
            OPEN_DISPS(play->state.gfxCtx);
            
            // @recomp Tag the ground to skip rotation.
            gEXMatrixGroupDecomposedSkipRot(POLY_OPA_DISP++, actor_transform_id(thisx) + 0, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);

            Gfx_DrawDListOpa(play, gKeikokuDemoFloorDL);
            Gfx_DrawDListXlu(play, gKeikokuDemoFloorEmptyDL);

            // @recomp Pop the tag.
            gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);

            CLOSE_DISPS(play->state.gfxCtx);
            break;

        case DMOPSTAGE_TYPE_ROOT_TREE:
            Gfx_DrawDListOpa(play, gKeikokuDemoTallTreeWithRootBaseDL);
            Gfx_DrawDListXlu(play, gKeikokuDemoTallTreeWithRootBaseEmptyDL);
            break;

        case DMOPSTAGE_TYPE_CUT_TREE:
            Gfx_DrawDListOpa(play, gKeikokuDemoTallTreeCutDL);
            Gfx_DrawDListXlu(play, gKeikokuDemoTallTreeCutEmptyDL);
            break;

        case DMOPSTAGE_TYPE_STRAIGHT_TREE:
            Gfx_DrawDListOpa(play, gKeikokuDemoTallTreeStraightDL);
            Gfx_DrawDListXlu(play, gKeikokuDemoTallTreeStraightEmptyDL);
            break;

        default:
            break;
    }
}
