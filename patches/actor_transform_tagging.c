#include "patches.h"
#include "fault.h"
#include "transform_ids.h"

// Start after G_EX_ID_IGNORE to avoid assigning it to a group.
u32 next_actor_transform = ACTOR_TRANSFORM_ID_START;

// Use 32 bits of compiler-inserted padding to hold the actor's transform ID.
// 0x22 between halfDaysBits and world
#define actorIdByte0(actor) ((u8*)(&(actor)->halfDaysBits))[2]
// 0x23 between halfDaysBits and world
#define actorIdByte1(actor) ((u8*)(&(actor)->halfDaysBits))[3]
// 0x3A between audioFlags and focus
#define actorIdByte2(actor) ((u8*)(&(actor)->audioFlags))[1] 
// 0x3B between audioFlags and focus
#define actorIdByte3(actor) ((u8*)(&(actor)->audioFlags))[2]

extern FaultClient sActorFaultClient;
Actor* Actor_Delete(ActorContext* actorCtx, Actor* actor, PlayState* play);
void ZeldaArena_Free(void* ptr);

void Actor_CleanupContext(ActorContext* actorCtx, PlayState* play) {
    s32 i;

    Fault_RemoveClient(&sActorFaultClient);

    for (i = 0; i < ARRAY_COUNT(actorCtx->actorLists); i++) {
        if (i != ACTORCAT_PLAYER) {
            Actor* actor = actorCtx->actorLists[i].first;

            while (actor != NULL) {
                Actor_Delete(actorCtx, actor, play);
                actor = actorCtx->actorLists[i].first;
            }
        }
    }

    while (actorCtx->actorLists[ACTORCAT_PLAYER].first != NULL) {
        Actor_Delete(actorCtx, actorCtx->actorLists[ACTORCAT_PLAYER].first, play);
    }

    if (actorCtx->absoluteSpace != NULL) {
        ZeldaArena_Free(actorCtx->absoluteSpace);
        actorCtx->absoluteSpace = NULL;
    }

    // @recomp Reset the actor transform IDs as all actors have been deleted.
    next_actor_transform = ACTOR_TRANSFORM_ID_START;

    Play_SaveCycleSceneFlags(&play->state);
    ActorOverlayTable_Cleanup();
}

u32 create_actor_transform_id() {
    u32 ret = next_actor_transform;
    next_actor_transform += ACTOR_TRANSFORM_ID_COUNT;

    // If the actor transform ID has overflowed, wrap back to the starting ID.
    if (next_actor_transform < ACTOR_TRANSFORM_ID_START) {
        next_actor_transform = ACTOR_TRANSFORM_ID_START;
        // Pick a new ID to make sure the actor has a valid range of IDs to use.
        ret = next_actor_transform;
        next_actor_transform += ACTOR_TRANSFORM_ID_COUNT;
    }

    // Skip ranges that include the special transform IDs.
    while (G_EX_ID_IGNORE - next_actor_transform < ACTOR_TRANSFORM_ID_COUNT || G_EX_ID_AUTO - next_actor_transform < ACTOR_TRANSFORM_ID_COUNT) {
        next_actor_transform += ACTOR_TRANSFORM_ID_COUNT;
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
    u32 cur_transform_id = create_actor_transform_id();
    actorIdByte0(actor) = (cur_transform_id >>  0) & 0xFF;
    actorIdByte1(actor) = (cur_transform_id >>  8) & 0xFF;
    actorIdByte2(actor) = (cur_transform_id >> 16) & 0xFF;
    actorIdByte3(actor) = (cur_transform_id >> 24) & 0xFF;
}

// Extract the transform ID for this actor, add the limb index and write that as the matrix group to POLY_OPA_DISP.
Gfx* push_limb_matrix_group(Gfx* dlist, Actor* actor, u32 limb_index) {
    if (actor != NULL) {
        u32 cur_transform_id = 
            (actorIdByte0(actor) <<  0) |
            (actorIdByte1(actor) <<  8) |
            (actorIdByte2(actor) << 16) |
            (actorIdByte3(actor) << 24);
        gEXMatrixGroup(dlist++, cur_transform_id + limb_index, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_ORDER_LINEAR);
    }
    return dlist;
}

// Extract the transform ID for this actor, add the limb index and post-limb offset and write that as the matrix group to POLY_OPA_DISP.
Gfx* push_post_limb_matrix_group(Gfx* dlist, Actor* actor, u32 limb_index) {
    if (actor != NULL) {
        u32 cur_transform_id = 
            (actorIdByte0(actor) <<  0) |
            (actorIdByte1(actor) <<  8) |
            (actorIdByte2(actor) << 16) |
            (actorIdByte3(actor) << 24);
        gEXMatrixGroup(dlist++, cur_transform_id + limb_index + ACTOR_TRANSFORM_LIMB_COUNT, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_ORDER_LINEAR);
    }
    return dlist;
}

// Extract the transform ID for this actor, add the limb index and write that as the matrix group to POLY_OPA_DISP.
Gfx* push_skin_limb_matrix_group(Gfx* dlist, Actor* actor, u32 limb_index) {
    if (actor != NULL) {
        u32 cur_transform_id = 
            (actorIdByte0(actor) <<  0) |
            (actorIdByte1(actor) <<  8) |
            (actorIdByte2(actor) << 16) |
            (actorIdByte3(actor) << 24);
        gEXMatrixGroup(dlist++, cur_transform_id + limb_index, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);
    }
    return dlist;
}

Gfx* pop_limb_matrix_group(Gfx* dlist, Actor* actor) {
    if (actor != NULL) {
        gEXPopMatrixGroup(dlist++);
    }
    return dlist;
}

Gfx* pop_post_limb_matrix_group(Gfx* dlist, Actor* actor) {
    if (actor != NULL) {
        gEXPopMatrixGroup(dlist++);
    }
    return dlist;
}

/*
 * Draws the limb at `limbIndex` with a level of detail display lists index by `dListIndex`
 */
void SkelAnime_DrawLimbLod(PlayState* play, s32 limbIndex, void** skeleton, Vec3s* jointTable,
                           OverrideLimbDrawOpa overrideLimbDraw, PostLimbDrawOpa postLimbDraw, Actor* actor, s32 lod) {
    LodLimb* limb;
    Gfx* dList;
    Vec3f pos;
    Vec3s rot;

    OPEN_DISPS(play->state.gfxCtx);

    Matrix_Push();
    limb = Lib_SegmentedToVirtual(skeleton[limbIndex]);
    limbIndex++;
    rot = jointTable[limbIndex];

    pos.x = limb->jointPos.x;
    pos.y = limb->jointPos.y;
    pos.z = limb->jointPos.z;

    // @recomp Push the limb's matrix group.
    POLY_OPA_DISP = push_limb_matrix_group(POLY_OPA_DISP, actor, limbIndex);

    dList = limb->dLists[lod];
    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, limbIndex, &dList, &pos, &rot, actor)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (dList != NULL) {
            Gfx* polyTemp = POLY_OPA_DISP;

            gSPMatrix(&polyTemp[0], Matrix_NewMtx(play->state.gfxCtx), G_MTX_LOAD);

            gSPDisplayList(&polyTemp[1], dList);
            POLY_OPA_DISP = &polyTemp[2];
        }
    }

    // @recomp Pop the limb's matrix group and push the post-limb matrix group.
    POLY_OPA_DISP = pop_limb_matrix_group(POLY_OPA_DISP, actor);
    POLY_OPA_DISP = push_post_limb_matrix_group(POLY_OPA_DISP, actor, limbIndex);

    if (postLimbDraw != NULL) {
        postLimbDraw(play, limbIndex, &dList, &rot, actor);
    }

    // @recomp Pop the post-limb matrix group.
    POLY_OPA_DISP = pop_post_limb_matrix_group(POLY_OPA_DISP, actor);

    if (limb->child != LIMB_DONE) {
        SkelAnime_DrawLimbLod(play, limb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, actor, lod);
    }

    Matrix_Pop();

    if (limb->sibling != LIMB_DONE) {
        SkelAnime_DrawLimbLod(play, limb->sibling, skeleton, jointTable, overrideLimbDraw, postLimbDraw, actor, lod);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

/**
 * Draw all limbs of type `LodLimb` in a given skeleton
 * Near or far display list is specified via `lod`
 */
void SkelAnime_DrawLod(PlayState* play, void** skeleton, Vec3s* jointTable, OverrideLimbDrawOpa overrideLimbDraw,
                       PostLimbDrawOpa postLimbDraw, Actor* actor, s32 lod) {
    LodLimb* rootLimb;
    s32 pad;
    Gfx* dList;
    Vec3f pos;
    Vec3s rot;

    if (skeleton == NULL) {
        return;
    }

    OPEN_DISPS(play->state.gfxCtx);

    Matrix_Push();

    rootLimb = Lib_SegmentedToVirtual(skeleton[0]);
    pos.x = jointTable[0].x;
    pos.y = jointTable[0].y;
    pos.z = jointTable[0].z;

    rot = jointTable[1];
    dList = rootLimb->dLists[lod];

    // @recomp Push the limb's matrix group.
    POLY_OPA_DISP = push_limb_matrix_group(POLY_OPA_DISP, actor, 0);

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, 1, &dList, &pos, &rot, actor)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (dList != NULL) {
            Gfx* polyTemp = POLY_OPA_DISP;

            gSPMatrix(&polyTemp[0], Matrix_NewMtx(play->state.gfxCtx), G_MTX_LOAD);

            gSPDisplayList(&polyTemp[1], dList);

            POLY_OPA_DISP = &polyTemp[2];
        }
    }

    // @recomp Pop the limb's matrix group and push the post-limb matrix group.
    POLY_OPA_DISP = pop_limb_matrix_group(POLY_OPA_DISP, actor);
    POLY_OPA_DISP = push_post_limb_matrix_group(POLY_OPA_DISP, actor, 0);

    if (postLimbDraw != NULL) {
        postLimbDraw(play, 1, &dList, &rot, actor);
    }

    // @recomp Pop the post-limb matrix group.
    POLY_OPA_DISP = pop_post_limb_matrix_group(POLY_OPA_DISP, actor);

    if (rootLimb->child != LIMB_DONE) {
        SkelAnime_DrawLimbLod(play, rootLimb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, actor, lod);
    }

    Matrix_Pop();

    CLOSE_DISPS(play->state.gfxCtx);
}

/**
 * Draw a limb of type `LodLimb` contained within a flexible skeleton
 * Near or far display list is specified via `lod`
 */
void SkelAnime_DrawFlexLimbLod(PlayState* play, s32 limbIndex, void** skeleton, Vec3s* jointTable,
                               OverrideLimbDrawFlex overrideLimbDraw, PostLimbDrawFlex postLimbDraw, Actor* actor,
                               s32 lod, Mtx** mtx) {
    LodLimb* limb;
    Gfx* newDList;
    Gfx* limbDList;
    Vec3f pos;
    Vec3s rot;

    OPEN_DISPS(play->state.gfxCtx);

    Matrix_Push();

    limb = Lib_SegmentedToVirtual(skeleton[limbIndex]);
    limbIndex++;

    rot = jointTable[limbIndex];

    pos.x = limb->jointPos.x;
    pos.y = limb->jointPos.y;
    pos.z = limb->jointPos.z;

    newDList = limbDList = limb->dLists[lod];

    // @recomp Push the limb's matrix group.
    POLY_OPA_DISP = push_limb_matrix_group(POLY_OPA_DISP, actor, limbIndex);

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, limbIndex, &newDList, &pos, &rot, actor)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (newDList != NULL) {
            Matrix_ToMtx(*mtx);
            gSPMatrix(POLY_OPA_DISP++, *mtx, G_MTX_LOAD);
            gSPDisplayList(POLY_OPA_DISP++, newDList);
            (*mtx)++;
        } else if (limbDList != NULL) {
            Matrix_ToMtx(*mtx);
            (*mtx)++;
        }
    }

    // @recomp Pop the limb's matrix group and push the post-limb matrix group.
    POLY_OPA_DISP = pop_limb_matrix_group(POLY_OPA_DISP, actor);
    POLY_OPA_DISP = push_post_limb_matrix_group(POLY_OPA_DISP, actor, limbIndex);

    if (postLimbDraw != NULL) {
        postLimbDraw(play, limbIndex, &newDList, &limbDList, &rot, actor);
    }

    // @recomp Pop the post-limb matrix group.
    POLY_OPA_DISP = pop_post_limb_matrix_group(POLY_OPA_DISP, actor);

    if (limb->child != LIMB_DONE) {
        SkelAnime_DrawFlexLimbLod(play, limb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, actor, lod,
                                  mtx);
    }

    Matrix_Pop();

    if (limb->sibling != LIMB_DONE) {
        SkelAnime_DrawFlexLimbLod(play, limb->sibling, skeleton, jointTable, overrideLimbDraw, postLimbDraw, actor, lod,
                                  mtx);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

/**
 * Draws all limbs of type `LodLimb` in a given flexible skeleton
 * Limbs in a flexible skeleton have meshes that can stretch to line up with other limbs.
 * An array of matrices is dynamically allocated so each limb can access any transform to ensure its meshes line up.
 */
void SkelAnime_DrawFlexLod(PlayState* play, void** skeleton, Vec3s* jointTable, s32 dListCount,
                           OverrideLimbDrawFlex overrideLimbDraw, PostLimbDrawFlex postLimbDraw, Actor* actor,
                           s32 lod) {
    LodLimb* rootLimb;
    s32 pad;
    Gfx* newDList;
    Gfx* limbDList;
    Vec3f pos;
    Vec3s rot;
    Mtx* mtx = GRAPH_ALLOC(play->state.gfxCtx, dListCount * sizeof(Mtx));

    if (skeleton == NULL) {
        return;
    }

    OPEN_DISPS(play->state.gfxCtx);

    gSPSegment(POLY_OPA_DISP++, 0x0D, mtx);
    Matrix_Push();

    rootLimb = Lib_SegmentedToVirtual(skeleton[0]);
    pos.x = jointTable[0].x;
    pos.y = jointTable[0].y;
    pos.z = jointTable[0].z;

    rot = jointTable[1];

    newDList = limbDList = rootLimb->dLists[lod];

    // @recomp Push the limb's matrix group.
    POLY_OPA_DISP = push_limb_matrix_group(POLY_OPA_DISP, actor, 0);

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, 1, &newDList, &pos, &rot, actor)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (newDList != NULL) {
            Gfx* polyTemp = POLY_OPA_DISP;

            gSPMatrix(&polyTemp[0], Matrix_ToMtx(mtx), G_MTX_LOAD);
            gSPDisplayList(&polyTemp[1], newDList);
            POLY_OPA_DISP = &polyTemp[2];
            mtx++;
        } else if (limbDList != NULL) {
            Matrix_ToMtx(mtx);
            mtx++;
        }
    }

    // @recomp Pop the limb's matrix group and push the post-limb matrix group.
    POLY_OPA_DISP = pop_limb_matrix_group(POLY_OPA_DISP, actor);
    POLY_OPA_DISP = push_post_limb_matrix_group(POLY_OPA_DISP, actor, 0);

    if (postLimbDraw != NULL) {
        postLimbDraw(play, 1, &newDList, &limbDList, &rot, actor);
    }

    // @recomp Pop the post-limb matrix group.
    POLY_OPA_DISP = pop_post_limb_matrix_group(POLY_OPA_DISP, actor);

    if (rootLimb->child != LIMB_DONE) {
        SkelAnime_DrawFlexLimbLod(play, rootLimb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, actor,
                                  lod, &mtx);
    }

    Matrix_Pop();

    CLOSE_DISPS(play->state.gfxCtx);
}

/*
 * Draws the limb of the Skeleton `skeleton` at `limbIndex`
 */
void SkelAnime_DrawLimbOpa(PlayState* play, s32 limbIndex, void** skeleton, Vec3s* jointTable,
                           OverrideLimbDrawOpa overrideLimbDraw, PostLimbDrawOpa postLimbDraw, Actor* actor) {
    StandardLimb* limb;
    Gfx* dList;
    Vec3f pos;
    Vec3s rot;

    OPEN_DISPS(play->state.gfxCtx);

    Matrix_Push();

    limb = Lib_SegmentedToVirtual(skeleton[limbIndex]);
    limbIndex++;
    rot = jointTable[limbIndex];
    pos.x = limb->jointPos.x;
    pos.y = limb->jointPos.y;
    pos.z = limb->jointPos.z;
    dList = limb->dList;

    // @recomp Push the limb's matrix group.
    POLY_OPA_DISP = push_limb_matrix_group(POLY_OPA_DISP, actor, limbIndex);

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, limbIndex, &dList, &pos, &rot, actor)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (dList != NULL) {
            Gfx* polyTemp = POLY_OPA_DISP;

            gSPMatrix(&polyTemp[0], Matrix_NewMtx(play->state.gfxCtx), G_MTX_LOAD);
            gSPDisplayList(&polyTemp[1], dList);
            POLY_OPA_DISP = &polyTemp[2];
        }
    }

    // @recomp Pop the limb's matrix group and push the post-limb matrix group.
    POLY_OPA_DISP = pop_limb_matrix_group(POLY_OPA_DISP, actor);
    POLY_OPA_DISP = push_post_limb_matrix_group(POLY_OPA_DISP, actor, limbIndex);

    if (postLimbDraw != NULL) {
        postLimbDraw(play, limbIndex, &dList, &rot, actor);
    }

    // @recomp Pop the post-limb matrix group.
    POLY_OPA_DISP = pop_post_limb_matrix_group(POLY_OPA_DISP, actor);

    if (limb->child != LIMB_DONE) {
        SkelAnime_DrawLimbOpa(play, limb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, actor);
    }

    Matrix_Pop();

    if (limb->sibling != LIMB_DONE) {
        SkelAnime_DrawLimbOpa(play, limb->sibling, skeleton, jointTable, overrideLimbDraw, postLimbDraw, actor);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

/**
 * Draw all limbs of type `StandardLimb` in a given skeleton to the polyOpa buffer
 */
void SkelAnime_DrawOpa(PlayState* play, void** skeleton, Vec3s* jointTable, OverrideLimbDrawOpa overrideLimbDraw,
                       PostLimbDrawOpa postLimbDraw, Actor* actor) {
    StandardLimb* rootLimb;
    s32 pad;
    Gfx* dList;
    Vec3f pos;
    Vec3s rot;

    if (skeleton == NULL) {
        return;
    }

    OPEN_DISPS(play->state.gfxCtx);

    Matrix_Push();
    rootLimb = Lib_SegmentedToVirtual(skeleton[0]);

    pos.x = jointTable[0].x;
    pos.y = jointTable[0].y;
    pos.z = jointTable[0].z;

    rot = jointTable[1];
    dList = rootLimb->dList;

    // @recomp Push the limb's matrix group.
    POLY_OPA_DISP = push_limb_matrix_group(POLY_OPA_DISP, actor, 0);

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, 1, &dList, &pos, &rot, actor)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (dList != NULL) {
            Gfx* polyTemp = POLY_OPA_DISP;

            gSPMatrix(&polyTemp[0], Matrix_NewMtx(play->state.gfxCtx), G_MTX_LOAD);
            gSPDisplayList(&polyTemp[1], dList);
            POLY_OPA_DISP = &polyTemp[2];
        }
    }

    // @recomp Pop the limb's matrix group and push the post-limb matrix group.
    POLY_OPA_DISP = pop_limb_matrix_group(POLY_OPA_DISP, actor);
    POLY_OPA_DISP = push_post_limb_matrix_group(POLY_OPA_DISP, actor, 0);

    if (postLimbDraw != NULL) {
        postLimbDraw(play, 1, &dList, &rot, actor);
    }

    // @recomp Pop the post-limb matrix group.
    POLY_OPA_DISP = pop_post_limb_matrix_group(POLY_OPA_DISP, actor);

    if (rootLimb->child != LIMB_DONE) {
        SkelAnime_DrawLimbOpa(play, rootLimb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, actor);
    }

    Matrix_Pop();

    CLOSE_DISPS(play->state.gfxCtx);
}

void SkelAnime_DrawFlexLimbOpa(PlayState* play, s32 limbIndex, void** skeleton, Vec3s* jointTable,
                               OverrideLimbDrawOpa overrideLimbDraw, PostLimbDrawOpa postLimbDraw, Actor* actor,
                               Mtx** limbMatricies) {
    StandardLimb* limb;
    Gfx* newDList;
    Gfx* limbDList;
    Vec3f pos;
    Vec3s rot;

    OPEN_DISPS(play->state.gfxCtx);

    Matrix_Push();

    limb = Lib_SegmentedToVirtual(skeleton[limbIndex]);
    limbIndex++;
    rot = jointTable[limbIndex];

    pos.x = limb->jointPos.x;
    pos.y = limb->jointPos.y;
    pos.z = limb->jointPos.z;

    newDList = limbDList = limb->dList;

    // @recomp Push the limb's matrix group.
    POLY_OPA_DISP = push_limb_matrix_group(POLY_OPA_DISP, actor, limbIndex);

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, limbIndex, &newDList, &pos, &rot, actor)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (newDList != NULL) {
            Matrix_ToMtx(*limbMatricies);
            gSPMatrix(POLY_OPA_DISP++, *limbMatricies, G_MTX_LOAD);
            gSPDisplayList(POLY_OPA_DISP++, newDList);
            (*limbMatricies)++;
        } else if (limbDList != NULL) {
            Matrix_ToMtx(*limbMatricies);
            (*limbMatricies)++;
        }
    }

    // @recomp Pop the limb's matrix group and push the post-limb matrix group.
    POLY_OPA_DISP = pop_limb_matrix_group(POLY_OPA_DISP, actor);
    POLY_OPA_DISP = push_post_limb_matrix_group(POLY_OPA_DISP, actor, limbIndex);

    if (postLimbDraw != NULL) {
        postLimbDraw(play, limbIndex, &limbDList, &rot, actor);
    }

    // @recomp Pop the post-limb matrix group.
    POLY_OPA_DISP = pop_post_limb_matrix_group(POLY_OPA_DISP, actor);

    if (limb->child != LIMB_DONE) {
        SkelAnime_DrawFlexLimbOpa(play, limb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, actor,
                                  limbMatricies);
    }

    Matrix_Pop();

    if (limb->sibling != LIMB_DONE) {
        SkelAnime_DrawFlexLimbOpa(play, limb->sibling, skeleton, jointTable, overrideLimbDraw, postLimbDraw, actor,
                                  limbMatricies);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

/**
 * Draw all limbs of type `StandardLimb` in a given flexible skeleton to the polyOpa buffer
 * Limbs in a flexible skeleton have meshes that can stretch to line up with other limbs.
 * An array of matrices is dynamically allocated so each limb can access any transform to ensure its meshes line up.
 */
void SkelAnime_DrawFlexOpa(PlayState* play, void** skeleton, Vec3s* jointTable, s32 dListCount,
                           OverrideLimbDrawOpa overrideLimbDraw, PostLimbDrawOpa postLimbDraw, Actor* actor) {
    StandardLimb* rootLimb;
    s32 pad;
    Gfx* newDList;
    Gfx* limbDList;
    Vec3f pos;
    Vec3s rot;
    Mtx* mtx = GRAPH_ALLOC(play->state.gfxCtx, dListCount * sizeof(Mtx));

    if (skeleton == NULL) {
        return;
    }

    OPEN_DISPS(play->state.gfxCtx);

    gSPSegment(POLY_OPA_DISP++, 0x0D, mtx);

    Matrix_Push();

    rootLimb = Lib_SegmentedToVirtual(skeleton[0]);

    pos.x = jointTable[0].x;
    pos.y = jointTable[0].y;
    pos.z = jointTable[0].z;
    rot = jointTable[1];

    newDList = limbDList = rootLimb->dList;

    // @recomp Push the limb's matrix group.
    POLY_OPA_DISP = push_limb_matrix_group(POLY_OPA_DISP, actor, 0);

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, 1, &newDList, &pos, &rot, actor)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (newDList != NULL) {
            Gfx* polyTemp = POLY_OPA_DISP;

            gSPMatrix(&polyTemp[0], Matrix_ToMtx(mtx), G_MTX_LOAD);
            gSPDisplayList(&polyTemp[1], newDList);
            POLY_OPA_DISP = &polyTemp[2];
            mtx++;
        } else {
            if (limbDList != NULL) {
                Matrix_ToMtx(mtx);
                mtx++;
            }
        }
    }

    // @recomp Pop the limb's matrix group and push the post-limb matrix group.
    POLY_OPA_DISP = pop_limb_matrix_group(POLY_OPA_DISP, actor);
    POLY_OPA_DISP = push_post_limb_matrix_group(POLY_OPA_DISP, actor, 0);

    if (postLimbDraw != NULL) {
        postLimbDraw(play, 1, &limbDList, &rot, actor);
    }

    // @recomp Pop the post-limb matrix group.
    POLY_OPA_DISP = pop_post_limb_matrix_group(POLY_OPA_DISP, actor);

    if (rootLimb->child != LIMB_DONE) {
        SkelAnime_DrawFlexLimbOpa(play, rootLimb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, actor,
                                  &mtx);
    }

    Matrix_Pop();

    CLOSE_DISPS(play->state.gfxCtx);
}

void SkelAnime_DrawTransformFlexLimbOpa(PlayState* play, s32 limbIndex, void** skeleton, Vec3s* jointTable,
                                        OverrideLimbDrawOpa overrideLimbDraw, PostLimbDrawOpa postLimbDraw,
                                        TransformLimbDrawOpa transformLimbDraw, Actor* actor, Mtx** mtx) {
    StandardLimb* limb;
    Gfx* newDList;
    Gfx* limbDList;
    Vec3f pos;
    Vec3s rot;

    OPEN_DISPS(play->state.gfxCtx);

    Matrix_Push();

    limb = Lib_SegmentedToVirtual(skeleton[limbIndex]);
    limbIndex++;

    rot = jointTable[limbIndex];
    pos.x = limb->jointPos.x;
    pos.y = limb->jointPos.y;
    pos.z = limb->jointPos.z;

    newDList = limbDList = limb->dList;

    // @recomp Push the limb's matrix group.
    POLY_OPA_DISP = push_limb_matrix_group(POLY_OPA_DISP, actor, limbIndex);

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, limbIndex, &newDList, &pos, &rot, actor)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        Matrix_Push();

        transformLimbDraw(play, limbIndex, actor);

        if (newDList != NULL) {
            Gfx* polyTemp = POLY_OPA_DISP;

            gSPMatrix(&polyTemp[0], Matrix_ToMtx(*mtx), G_MTX_LOAD);
            gSPDisplayList(&polyTemp[1], newDList);
            POLY_OPA_DISP = &polyTemp[2];
            (*mtx)++;
        } else {
            if (limbDList != NULL) {
                Matrix_ToMtx(*mtx);
                (*mtx)++;
            }
        }
        Matrix_Pop();
    }

    // @recomp Pop the limb's matrix group and push the post-limb matrix group.
    POLY_OPA_DISP = pop_limb_matrix_group(POLY_OPA_DISP, actor);
    POLY_OPA_DISP = push_post_limb_matrix_group(POLY_OPA_DISP, actor, limbIndex);

    if (postLimbDraw != NULL) {
        postLimbDraw(play, limbIndex, &limbDList, &rot, actor);
    }

    // @recomp Pop the post-limb matrix group.
    POLY_OPA_DISP = pop_post_limb_matrix_group(POLY_OPA_DISP, actor);

    if (limb->child != LIMB_DONE) {
        SkelAnime_DrawTransformFlexLimbOpa(play, limb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw,
                                           transformLimbDraw, actor, mtx);
    }

    Matrix_Pop();

    if (limb->sibling != LIMB_DONE) {
        SkelAnime_DrawTransformFlexLimbOpa(play, limb->sibling, skeleton, jointTable, overrideLimbDraw, postLimbDraw,
                                           transformLimbDraw, actor, mtx);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

/**
 * Draw all limbs of type `StandardLimb` in a given flexible skeleton to the polyOpa buffer
 * Limbs in a flexible skeleton have meshes that can stretch to line up with other limbs.
 * An array of matrices is dynamically allocated so each limb can access any transform to ensure its meshes line up.
 *
 * Also makes use of a `TransformLimbDraw`, which transforms limbs based on world coordinates, as opposed to local limb
 * coordinates.
 * Note that the `TransformLimbDraw` does not have a NULL check, so must be provided even if empty.
 */
void SkelAnime_DrawTransformFlexOpa(PlayState* play, void** skeleton, Vec3s* jointTable, s32 dListCount,
                                    OverrideLimbDrawOpa overrideLimbDraw, PostLimbDrawOpa postLimbDraw,
                                    TransformLimbDrawOpa transformLimbDraw, Actor* actor) {
    StandardLimb* rootLimb;
    s32 pad;
    Gfx* newDList;
    Gfx* limbDList;
    Vec3f pos;
    Vec3s rot;
    Mtx* mtx;

    if (skeleton == NULL) {
        return;
    }

    OPEN_DISPS(play->state.gfxCtx);

    mtx = GRAPH_ALLOC(play->state.gfxCtx, dListCount * sizeof(Mtx));

    gSPSegment(POLY_OPA_DISP++, 0x0D, mtx);

    Matrix_Push();

    rootLimb = Lib_SegmentedToVirtual(skeleton[0]);

    pos.x = jointTable[0].x;
    pos.y = jointTable[0].y;
    pos.z = jointTable[0].z;
    rot = jointTable[1];

    newDList = limbDList = rootLimb->dList;

    // @recomp Push the limb's matrix group.
    POLY_OPA_DISP = push_limb_matrix_group(POLY_OPA_DISP, actor, 0);

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, 1, &newDList, &pos, &rot, actor)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        Matrix_Push();

        transformLimbDraw(play, 1, actor);

        if (newDList != NULL) {
            Gfx* polyTemp = POLY_OPA_DISP;

            gSPMatrix(&polyTemp[0], Matrix_ToMtx(mtx), G_MTX_LOAD);
            gSPDisplayList(&polyTemp[1], newDList);
            POLY_OPA_DISP = &polyTemp[2];
            mtx++;
        } else {
            if (limbDList != NULL) {
                Matrix_ToMtx(mtx++);
            }
        }
        Matrix_Pop();
    }

    // @recomp Pop the limb's matrix group and push the post-limb matrix group.
    POLY_OPA_DISP = pop_limb_matrix_group(POLY_OPA_DISP, actor);
    POLY_OPA_DISP = push_post_limb_matrix_group(POLY_OPA_DISP, actor, 0);

    if (postLimbDraw != NULL) {
        postLimbDraw(play, 1, &limbDList, &rot, actor);
    }

    // @recomp Pop the post-limb matrix group.
    POLY_OPA_DISP = pop_post_limb_matrix_group(POLY_OPA_DISP, actor);

    if (rootLimb->child != LIMB_DONE) {
        SkelAnime_DrawTransformFlexLimbOpa(play, rootLimb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw,
                                           transformLimbDraw, actor, &mtx);
    }

    Matrix_Pop();

    CLOSE_DISPS(play->state.gfxCtx);
}

extern MtxF gSkinLimbMatrices[];

void Skin_DrawImpl(Actor* actor, PlayState* play, Skin* skin, SkinPostDraw postDraw,
                   SkinOverrideLimbDraw overrideLimbDraw, s32 setTranslation, s32 arg6, s32 drawFlags) {
    s32 i;
    SkinLimb** skeleton;
    GraphicsContext* gfxCtx = play->state.gfxCtx;

    OPEN_DISPS(gfxCtx);

    if (!(drawFlags & SKIN_DRAW_FLAG_CUSTOM_TRANSFORMS)) {
        Skin_ApplyAnimTransformations(skin, gSkinLimbMatrices, actor, setTranslation);
    }

    skeleton = Lib_SegmentedToVirtual(skin->skeletonHeader->segment);

    if (!(drawFlags & SKIN_DRAW_FLAG_CUSTOM_MATRIX)) {
        Mtx* mtx;

        gSPMatrix(POLY_OPA_DISP++, &gIdentityMtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        mtx = SkinMatrix_MtxFToNewMtx(gfxCtx, &skin->mtx);
        if (mtx == NULL) {
            goto close_disps;
        }
        gSPMatrix(POLY_OPA_DISP++, mtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    }

    for (i = 0; i < skin->skeletonHeader->limbCount; i++) {
        s32 shouldDraw = true;
        s32 segmentType;

        if (overrideLimbDraw != NULL) {
            shouldDraw = overrideLimbDraw(actor, play, i, skin);
        }

        segmentType = ((SkinLimb*)Lib_SegmentedToVirtual(skeleton[i]))->segmentType;

        // @recomp Push a new matrix by multiplying in the identity matrix. This doesn't change the actual matrix contents,
        // but allows a unique matrix group ID to be applied to this limb.
        gSPMatrix(POLY_OPA_DISP++, &gIdentityMtx, G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
        // Tag the matrix.
        POLY_OPA_DISP = push_skin_limb_matrix_group(POLY_OPA_DISP, actor, i);

        if (segmentType == SKIN_LIMB_TYPE_ANIMATED && shouldDraw) {
            Skin_DrawAnimatedLimb(gfxCtx, skin, i, arg6, drawFlags);
        } else if (segmentType == SKIN_LIMB_TYPE_NORMAL && shouldDraw) {
            Skin_DrawLimb(gfxCtx, skin, i, NULL, drawFlags);
        }

        // @recomp Pop the matrix and matrix group that were made earlier.
        gSPPopMatrix(POLY_OPA_DISP++, G_MTX_MODELVIEW);
        POLY_OPA_DISP = pop_limb_matrix_group(POLY_OPA_DISP, actor);
    }

    if (postDraw != NULL) {
        postDraw(actor, play, skin);
    }

close_disps:;
    CLOSE_DISPS(gfxCtx);
}
