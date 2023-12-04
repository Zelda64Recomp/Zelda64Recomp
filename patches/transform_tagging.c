#include "patches.h"

// Start after G_EX_ID_AUTO to avoid assigning it to a group
u32 total_transforms = G_EX_ID_AUTO + 1;

// Use 24 bits of compiler-inserted padding to hold the actor's transform ID.
// 0x3A
#define actorIdByte0(actor) ((u8*)(&(actor)->audioFlags))[1] 
// 0x3B
#define actorIdByte1(actor) ((u8*)(&(actor)->audioFlags))[2]

u32 create_transform_id() {
    u32 ret = total_transforms;
    total_transforms = (total_transforms + 1) & 0xFFFF;

    // Skip the auto transform ID
    if (total_transforms == G_EX_ID_AUTO) {
        total_transforms++;
    }

    return ret;
}

void Actor_Init(Actor* actor, PlayState* play) {
    Actor_SetWorldToHome(actor);
    Actor_SetShapeRotToWorld(actor);
    Actor_SetFocus(actor, 0.0f);
    Math_Vec3f_Copy(&actor->prevPos, &actor->world.pos);
    Actor_SetScale(actor, 0.01f);
    actor->targetMode = TARGET_MODE_3;
    actor->terminalVelocity = -20.0f;

    actor->xyzDistToPlayerSq = FLT_MAX;
    actor->uncullZoneForward = 1000.0f;
    actor->uncullZoneScale = 350.0f;
    actor->uncullZoneDownward = 700.0f;

    actor->hintId = TATL_HINT_ID_NONE;

    CollisionCheck_InitInfo(&actor->colChkInfo);
    actor->floorBgId = BGCHECK_SCENE;

    ActorShape_Init(&actor->shape, 0.0f, NULL, 0.0f);
    if (Object_IsLoaded(&play->objectCtx, actor->objectSlot)) {
        Actor_SetObjectDependency(play, actor);
        actor->init(actor, play);
        actor->init = NULL;
    }
    
    // @recomp Pick a transform ID for this actor and encode it into struct padding
    u32 cur_transform_id = create_transform_id();
    actorIdByte0(actor) = (cur_transform_id >>  0) & 0xFF;
    actorIdByte1(actor) = (cur_transform_id >>  8) & 0xFF;
}

void Actor_Draw(PlayState* play, Actor* actor) {
    Lights* light;

    OPEN_DISPS(play->state.gfxCtx);

    light = LightContext_NewLights(&play->lightCtx, play->state.gfxCtx);
    if ((actor->flags & ACTOR_FLAG_10000000) && (play->roomCtx.curRoom.enablePosLights || (MREG(93) != 0))) {
        light->enablePosLights = true;
    }

    Lights_BindAll(light, play->lightCtx.listHead,
                   (actor->flags & (ACTOR_FLAG_10000000 | ACTOR_FLAG_400000)) ? NULL : &actor->world.pos, play);
    Lights_Draw(light, play->state.gfxCtx);

    if (actor->flags & ACTOR_FLAG_IGNORE_QUAKE) {
        Matrix_SetTranslateRotateYXZ(actor->world.pos.x + play->mainCamera.quakeOffset.x,
                                     actor->world.pos.y +
                                         ((actor->shape.yOffset * actor->scale.y) + play->mainCamera.quakeOffset.y),
                                     actor->world.pos.z + play->mainCamera.quakeOffset.z, &actor->shape.rot);
    } else {
        Matrix_SetTranslateRotateYXZ(actor->world.pos.x, actor->world.pos.y + (actor->shape.yOffset * actor->scale.y),
                                     actor->world.pos.z, &actor->shape.rot);
    }

    Matrix_Scale(actor->scale.x, actor->scale.y, actor->scale.z, MTXMODE_APPLY);
    Actor_SetObjectDependency(play, actor);

    gSPSegment(POLY_OPA_DISP++, 0x06, play->objectCtx.slots[actor->objectSlot].segment);
    gSPSegment(POLY_XLU_DISP++, 0x06, play->objectCtx.slots[actor->objectSlot].segment);
    
    // @recomp Extract the transform ID for this actor and write it to both lists
    u32 cur_transform_id =
        (actorIdByte0(actor) <<  0) |
        (actorIdByte1(actor) <<  8);
    gEXMatrixGroup(POLY_OPA_DISP++, cur_transform_id, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE);
    gEXMatrixGroup(POLY_XLU_DISP++, cur_transform_id, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE);

    if (actor->colorFilterTimer != 0) {
        s32 colorFlag = COLORFILTER_GET_COLORFLAG(actor->colorFilterParams);
        Color_RGBA8 actorDefaultHitColor = { 0, 0, 0, 255 };

        if (colorFlag == COLORFILTER_COLORFLAG_GRAY) {
            actorDefaultHitColor.r = actorDefaultHitColor.g = actorDefaultHitColor.b =
                COLORFILTER_GET_COLORINTENSITY(actor->colorFilterParams) | 7;
        } else if (colorFlag == COLORFILTER_COLORFLAG_RED) {
            actorDefaultHitColor.r = COLORFILTER_GET_COLORINTENSITY(actor->colorFilterParams) | 7;
        } else if (colorFlag == COLORFILTER_COLORFLAG_NONE) {
            actorDefaultHitColor.b = actorDefaultHitColor.g = actorDefaultHitColor.r = 0;
        } else {
            actorDefaultHitColor.b = COLORFILTER_GET_COLORINTENSITY(actor->colorFilterParams) | 7;
        }

        if (actor->colorFilterParams & COLORFILTER_BUFFLAG_XLU) {
            func_800AE778(play, &actorDefaultHitColor, actor->colorFilterTimer,
                          COLORFILTER_GET_DURATION(actor->colorFilterParams));
        } else {
            func_800AE434(play, &actorDefaultHitColor, actor->colorFilterTimer,
                          COLORFILTER_GET_DURATION(actor->colorFilterParams));
        }
    }

    actor->draw(actor, play);

    if (actor->colorFilterTimer != 0) {
        if (actor->colorFilterParams & COLORFILTER_BUFFLAG_XLU) {
            func_800AE8EC(play);
        } else {
            func_800AE5A0(play);
        }
    }

    if (actor->shape.shadowDraw != NULL) {
        actor->shape.shadowDraw(actor, light, play);
    }
    actor->isDrawn = true;

    gEXPopMatrixGroup(POLY_OPA_DISP++);
    gEXPopMatrixGroup(POLY_XLU_DISP++);

    CLOSE_DISPS(play->state.gfxCtx);
}
