#include "patches.h"
#include "fault.h"
#include "transform_ids.h"
#include "extended_actors.h"
#include "z64actor.h"
#include "actor_funcs.h"

extern FaultClient sActorFaultClient;
void Actor_Destroy(Actor* actor, PlayState* play);
Actor* Actor_Delete(ActorContext* actorCtx, Actor* actor, PlayState* play);
void ZeldaArena_Free(void* ptr);
Actor* Actor_RemoveFromCategory(PlayState* play, ActorContext* actorCtx, Actor* actorToRemove);
void Actor_FreeOverlay(ActorOverlay* entry);

RECOMP_PATCH void Actor_CleanupContext(ActorContext* actorCtx, PlayState* play) {
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

    // @recomp Reset the actor extension data.
    recomp_clear_all_actor_data();

    Play_SaveCycleSceneFlags(&play->state);
    ActorOverlayTable_Cleanup();
}

RECOMP_DECLARE_EVENT(recomp_should_actor_init(PlayState* play, Actor* actor, bool* should));
RECOMP_DECLARE_EVENT(recomp_after_actor_init(PlayState* play, Actor* actor));
RECOMP_DECLARE_EVENT(recomp_should_actor_update(PlayState* play, Actor* actor, bool* should));
RECOMP_DECLARE_EVENT(recomp_after_actor_update(PlayState* play, Actor* actor));

RECOMP_PATCH void Actor_Init(Actor* actor, PlayState* play) {
    // @recomp Allocate the actor's extension data.
    actor_set_slot(actor, recomp_create_actor_data(actor->id));

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
        
        // @recomp Augmented, allowing us to prevent actor init and hook into after init
        bool shouldInit = true;
        recomp_should_actor_init(play, actor, &shouldInit);
        if (shouldInit) {
            actor->init(actor, play);
            actor->init = NULL;
            recomp_after_actor_init(play, actor);
        } else {
            actor->init = NULL;
            Actor_Kill(actor);
        }
    }
}

RECOMP_PATCH Actor* Actor_Delete(ActorContext* actorCtx, Actor* actor, PlayState* play) {
    s32 pad;
    Player* player = GET_PLAYER(play);
    Actor* newHead;
    ActorOverlay* overlayEntry = actor->overlayEntry;

    if ((player != NULL) && (actor == player->lockOnActor)) {
        Player_Untarget(player);
        Camera_ChangeMode(Play_GetCamera(play, Play_GetActiveCamId(play)), CAM_MODE_NORMAL);
    }

    if (actor == actorCtx->targetCtx.fairyActor) {
        actorCtx->targetCtx.fairyActor = NULL;
    }

    if (actor == actorCtx->targetCtx.forcedTargetActor) {
        actorCtx->targetCtx.forcedTargetActor = NULL;
    }

    if (actor == actorCtx->targetCtx.bgmEnemy) {
        actorCtx->targetCtx.bgmEnemy = NULL;
    }

    AudioSfx_StopByPos(&actor->projectedPos);
    Actor_Destroy(actor, play);

    newHead = Actor_RemoveFromCategory(play, actorCtx, actor);

    // @recomp Destroy the actor's extension data.
    recomp_destroy_actor_data(actor_get_slot(actor));
    
    ZeldaArena_Free(actor);

    if (overlayEntry->vramStart != NULL) {
        overlayEntry->numLoaded--;
        Actor_FreeOverlay(overlayEntry);
    }

    return newHead;
}

// @recomp Copied from z_actor.c
typedef struct {
    /* 0x00 */ PlayState* play;
    /* 0x04 */ Actor* actor;
    /* 0x08 */ u32 freezeExceptionFlag;
    /* 0x0C */ u32 canFreezeCategory;
    /* 0x10 */ Actor* talkActor;
    /* 0x14 */ Player* player;
    /* 0x18 */ u32 updateActorFlagsMask; // Actor will update only if at least 1 actor flag is set in this bitmask
} UpdateActor_Params;                    // size = 0x1C

RECOMP_PATCH Actor* Actor_UpdateActor(UpdateActor_Params* params) {
    PlayState* play = params->play;
    Actor* actor = params->actor;
    Actor* nextActor;

    if (actor->world.pos.y < -25000.0f) {
        actor->world.pos.y = -25000.0f;
    }

    actor->sfxId = 0;
    actor->audioFlags &= ~(((1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0)) | ((1 << 6) | (1 << 5))); // ACTOR_AUDIO_FLAG_ALL

    if (actor->init != NULL) {
        if (Object_IsLoaded(&play->objectCtx, actor->objectSlot)) {
            Actor_SetObjectDependency(play, actor);

            // @recomp Augmented, allowing us to prevent actor init and hook into after init
            bool shouldInit = true;
            recomp_should_actor_init(play, actor, &shouldInit);
            if (shouldInit) {
                actor->init(actor, play);
                actor->init = NULL;
                recomp_after_actor_init(play, actor);
            } else {
                actor->init = NULL;
                Actor_Kill(actor);
            }
        }
        nextActor = actor->next;
    } else if (actor->update == NULL) {
        if (!actor->isDrawn) {
            nextActor = Actor_Delete(&play->actorCtx, actor, play);
        } else {
            Actor_Destroy(actor, play);
            nextActor = actor->next;
        }
    } else {
        if (!Object_IsLoaded(&play->objectCtx, actor->objectSlot)) {
            Actor_Kill(actor);
        } else if (((params->freezeExceptionFlag != 0) && !(actor->flags & params->freezeExceptionFlag)) ||
                   (((!params->freezeExceptionFlag) != 0) &&
                    (!(actor->flags & (1 << 20)) ||
                     ((actor->category == ACTORCAT_EXPLOSIVES) && (params->player->stateFlags1 & PLAYER_STATE1_200))) &&
                    params->canFreezeCategory && (actor != params->talkActor) && (actor != params->player->heldActor) &&
                    (actor->parent != &params->player->actor))) {
            CollisionCheck_ResetDamage(&actor->colChkInfo);
        } else {
            Math_Vec3f_Copy(&actor->prevPos, &actor->world.pos);
            actor->xzDistToPlayer = Actor_WorldDistXZToActor(actor, &params->player->actor);
            actor->playerHeightRel = Actor_HeightDiff(actor, &params->player->actor);
            actor->xyzDistToPlayerSq = SQ(actor->xzDistToPlayer) + SQ(actor->playerHeightRel);

            actor->yawTowardsPlayer = Actor_WorldYawTowardActor(actor, &params->player->actor);
            actor->flags &= ~(1 << 24);

            if ((DECR(actor->freezeTimer) == 0) && (actor->flags & params->updateActorFlagsMask)) {
                if (actor == params->player->lockOnActor) {
                    actor->isLockedOn = true;
                } else {
                    actor->isLockedOn = false;
                }

                if ((actor->targetPriority != 0) && (params->player->lockOnActor == NULL)) {
                    actor->targetPriority = 0;
                }

                Actor_SetObjectDependency(play, actor);

                if (actor->colorFilterTimer != 0) {
                    actor->colorFilterTimer--;
                }

                // @recomp Augmented, allowing us to prevent actor update and hook into after update
                bool shouldUpdate = true;
                recomp_should_actor_update(play, actor, &shouldUpdate);
                if (shouldUpdate) {
                    actor->update(actor, play);
                    recomp_after_actor_update(play, actor);
                }
                DynaPoly_UnsetAllInteractFlags(play, &play->colCtx.dyna, actor);
            }

            CollisionCheck_ResetDamage(&actor->colChkInfo);
        }
        nextActor = actor->next;
    }
    return nextActor;
}

// Extract the transform ID for this actor, add the limb index and write that as the matrix group to POLY_OPA_DISP.
Gfx* push_limb_matrix_group(Gfx* dlist, Actor* actor, u32 limb_index) {
    if (actor != NULL) {
        u32 cur_transform_id = actor_transform_id(actor);
        if (actor_get_interpolation_skipped(actor)) {
            gEXMatrixGroupDecomposedSkipAll(dlist++, cur_transform_id + limb_index, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
        } else {
            gEXMatrixGroupDecomposedNormal(dlist++, cur_transform_id + limb_index, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
        }
    }
    return dlist;
}

// Extract the transform ID for this actor, add the limb index and post-limb offset and write that as the matrix group to POLY_OPA_DISP.
Gfx* push_post_limb_matrix_group(Gfx* dlist, Actor* actor, u32 limb_index) {
    if (actor != NULL) {
        u32 cur_transform_id = actor_transform_id(actor);
        if (actor_get_interpolation_skipped(actor)) {
            gEXMatrixGroupDecomposedSkipAll(dlist++, cur_transform_id + limb_index, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
        } else {
            gEXMatrixGroupDecomposedNormal(dlist++, cur_transform_id + limb_index + ACTOR_TRANSFORM_LIMB_COUNT, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
        }
    }
    return dlist;
}

// Extract the transform ID for this actor, add the limb index and write that as the matrix group to POLY_OPA_DISP.
Gfx* push_skin_limb_matrix_group(Gfx* dlist, Actor* actor, u32 limb_index) {
    if (actor != NULL) {
        u32 cur_transform_id = actor_transform_id(actor);
        if (actor_get_interpolation_skipped(actor)) {
            gEXMatrixGroupDecomposedSkipAll(dlist++, cur_transform_id + limb_index, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
        } else {
            gEXMatrixGroupDecomposedVerts(dlist++, cur_transform_id + limb_index, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
        }
    }
    return dlist;
}

Gfx* pop_limb_matrix_group(Gfx* dlist, Actor* actor) {
    if (actor != NULL) {
        gEXPopMatrixGroup(dlist++, G_MTX_MODELVIEW);
    }
    return dlist;
}

Gfx* pop_post_limb_matrix_group(Gfx* dlist, Actor* actor) {
    if (actor != NULL) {
        gEXPopMatrixGroup(dlist++, G_MTX_MODELVIEW);
    }
    return dlist;
}

/*
 * Draws the limb at `limbIndex` with a level of detail display lists index by `dListIndex`
 */
RECOMP_PATCH void SkelAnime_DrawLimbLod(PlayState* play, s32 limbIndex, void** skeleton, Vec3s* jointTable,
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
RECOMP_PATCH void SkelAnime_DrawLod(PlayState* play, void** skeleton, Vec3s* jointTable, OverrideLimbDrawOpa overrideLimbDraw,
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
RECOMP_PATCH void SkelAnime_DrawFlexLimbLod(PlayState* play, s32 limbIndex, void** skeleton, Vec3s* jointTable,
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
RECOMP_PATCH void SkelAnime_DrawFlexLod(PlayState* play, void** skeleton, Vec3s* jointTable, s32 dListCount,
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
RECOMP_PATCH void SkelAnime_DrawLimbOpa(PlayState* play, s32 limbIndex, void** skeleton, Vec3s* jointTable,
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
RECOMP_PATCH void SkelAnime_DrawOpa(PlayState* play, void** skeleton, Vec3s* jointTable, OverrideLimbDrawOpa overrideLimbDraw,
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

RECOMP_PATCH void SkelAnime_DrawFlexLimbOpa(PlayState* play, s32 limbIndex, void** skeleton, Vec3s* jointTable,
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
RECOMP_PATCH void SkelAnime_DrawFlexOpa(PlayState* play, void** skeleton, Vec3s* jointTable, s32 dListCount,
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

RECOMP_PATCH void SkelAnime_DrawTransformFlexLimbOpa(PlayState* play, s32 limbIndex, void** skeleton, Vec3s* jointTable,
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
RECOMP_PATCH void SkelAnime_DrawTransformFlexOpa(PlayState* play, void** skeleton, Vec3s* jointTable, s32 dListCount,
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

/*
 * Draws the Skeleton `skeleton`'s limb at index `limbIndex`.  Appends all generated graphics commands to
 * `gfx`.  Returns a pointer to the next gfx to be appended to.
 */
RECOMP_PATCH Gfx* SkelAnime_DrawLimb(PlayState* play, s32 limbIndex, void** skeleton, Vec3s* jointTable,
                        OverrideLimbDraw overrideLimbDraw, PostLimbDraw postLimbDraw, Actor* actor, Gfx* gfx) {
    StandardLimb* limb;
    Gfx* dList;
    Vec3f pos;
    Vec3s rot;

    Matrix_Push();

    limb = Lib_SegmentedToVirtual(skeleton[limbIndex]);
    limbIndex++;

    rot = jointTable[limbIndex];
    pos.x = limb->jointPos.x;
    pos.y = limb->jointPos.y;
    pos.z = limb->jointPos.z;

    dList = limb->dList;

    // @recomp Push the limb's matrix group.
    gfx = push_limb_matrix_group(gfx, actor, limbIndex);

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, limbIndex, &dList, &pos, &rot, actor, &gfx)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (dList != NULL) {
            gSPMatrix(&gfx[0], Matrix_NewMtx(play->state.gfxCtx), G_MTX_LOAD);
            gSPDisplayList(&gfx[1], dList);
            gfx = &gfx[2];
        }
    }

    // @recomp Pop the limb's matrix group and push the post-limb matrix group.
    gfx = pop_limb_matrix_group(gfx, actor);
    gfx = push_post_limb_matrix_group(gfx, actor, limbIndex);

    if (postLimbDraw != NULL) {
        postLimbDraw(play, limbIndex, &dList, &rot, actor, &gfx);
    }

    // @recomp Pop the post-limb matrix group.
    gfx = pop_post_limb_matrix_group(gfx, actor);

    if (limb->child != LIMB_DONE) {
        gfx = SkelAnime_DrawLimb(play, limb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, actor, gfx);
    }

    Matrix_Pop();

    if (limb->sibling != LIMB_DONE) {
        gfx = SkelAnime_DrawLimb(play, limb->sibling, skeleton, jointTable, overrideLimbDraw, postLimbDraw, actor, gfx);
    }

    return gfx;
}

/*
 * Draws the Skeleton `skeleton`  Appends all generated graphics to `gfx`, and returns a pointer to the
 * next gfx to be appended to.
 */
RECOMP_PATCH Gfx* SkelAnime_Draw(PlayState* play, void** skeleton, Vec3s* jointTable, OverrideLimbDraw overrideLimbDraw,
                    PostLimbDraw postLimbDraw, Actor* actor, Gfx* gfx) {
    StandardLimb* rootLimb;
    s32 pad;
    Gfx* dList;
    Vec3f pos;
    Vec3s rot;

    if (skeleton == NULL) {
        return NULL;
    }

    Matrix_Push();

    rootLimb = Lib_SegmentedToVirtual(skeleton[0]);

    pos.x = jointTable[0].x;
    pos.y = jointTable[0].y;
    pos.z = jointTable[0].z;

    rot = jointTable[1];

    dList = rootLimb->dList;

    // @recomp Push the limb's matrix group.
    gfx = push_limb_matrix_group(gfx, actor, 0);

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, 1, &dList, &pos, &rot, actor, &gfx)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (dList != NULL) {
            gSPMatrix(&gfx[0], Matrix_NewMtx(play->state.gfxCtx), G_MTX_LOAD);
            gSPDisplayList(&gfx[1], dList);
            gfx = &gfx[2];
        }
    }

    // @recomp Pop the limb's matrix group and push the post-limb matrix group.
    gfx = pop_limb_matrix_group(gfx, actor);
    gfx = push_post_limb_matrix_group(gfx, actor, 0);

    if (postLimbDraw != NULL) {
        postLimbDraw(play, 1, &dList, &rot, actor, &gfx);
    }

    // @recomp Pop the post-limb matrix group.
    gfx = pop_post_limb_matrix_group(gfx, actor);

    if (rootLimb->child != LIMB_DONE) {
        gfx =
            SkelAnime_DrawLimb(play, rootLimb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, actor, gfx);
    }

    Matrix_Pop();

    return gfx;
}

/**
 * Draw a limb of type `StandardLimb` contained within a flexible skeleton to the specified display buffer
 */
RECOMP_PATCH Gfx* SkelAnime_DrawFlexLimb(PlayState* play, s32 limbIndex, void** skeleton, Vec3s* jointTable,
                            OverrideLimbDraw overrideLimbDraw, PostLimbDraw postLimbDraw, Actor* actor, Mtx** mtx,
                            Gfx* gfx) {
    StandardLimb* limb;
    Gfx* newDList;
    Gfx* limbDList;
    Vec3f pos;
    Vec3s rot;

    Matrix_Push();

    limb = Lib_SegmentedToVirtual(skeleton[limbIndex]);
    limbIndex++;
    rot = jointTable[limbIndex];

    pos.x = limb->jointPos.x;
    pos.y = limb->jointPos.y;
    pos.z = limb->jointPos.z;

    newDList = limbDList = limb->dList;
    
    // @recomp Push the limb's matrix group.
    gfx = push_limb_matrix_group(gfx, actor, limbIndex);

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, limbIndex, &newDList, &pos, &rot, actor, &gfx)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (newDList != NULL) {
            gSPMatrix(&gfx[0], Matrix_ToMtx(*mtx), G_MTX_LOAD);
            gSPDisplayList(&gfx[1], newDList);
            gfx = &gfx[2];
            (*mtx)++;
        } else {
            if (limbDList != NULL) {
                Matrix_ToMtx(*mtx);
                (*mtx)++;
            }
        }
    }

    // @recomp Pop the limb's matrix group and push the post-limb matrix group.
    gfx = pop_limb_matrix_group(gfx, actor);
    gfx = push_post_limb_matrix_group(gfx, actor, limbIndex);

    if (postLimbDraw != NULL) {
        postLimbDraw(play, limbIndex, &limbDList, &rot, actor, &gfx);
    }

    // @recomp Pop the post-limb matrix group.
    gfx = pop_post_limb_matrix_group(gfx, actor);

    if (limb->child != LIMB_DONE) {
        gfx = SkelAnime_DrawFlexLimb(play, limb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, actor,
                                     mtx, gfx);
    }

    Matrix_Pop();

    if (limb->sibling != LIMB_DONE) {
        gfx = SkelAnime_DrawFlexLimb(play, limb->sibling, skeleton, jointTable, overrideLimbDraw, postLimbDraw, actor,
                                     mtx, gfx);
    }

    return gfx;
}

/**
 * Draw all limbs of type `StandardLimb` in a given flexible skeleton to the specified display buffer
 * Limbs in a flexible skeleton have meshes that can stretch to line up with other limbs.
 * An array of matrices is dynamically allocated so each limb can access any transform to ensure its meshes line up.
 */
RECOMP_PATCH Gfx* SkelAnime_DrawFlex(PlayState* play, void** skeleton, Vec3s* jointTable, s32 dListCount,
                        OverrideLimbDraw overrideLimbDraw, PostLimbDraw postLimbDraw, Actor* actor, Gfx* gfx) {
    StandardLimb* rootLimb;
    s32 pad;
    Gfx* newDList;
    Gfx* limbDList;
    Vec3f pos;
    Vec3s rot;
    Mtx* mtx;

    if (skeleton == NULL) {
        return NULL;
    }

    mtx = GRAPH_ALLOC(play->state.gfxCtx, dListCount * sizeof(Mtx));

    gSPSegment(gfx++, 0x0D, mtx);

    Matrix_Push();

    rootLimb = Lib_SegmentedToVirtual(skeleton[0]);

    pos.x = jointTable[0].x;
    pos.y = jointTable[0].y;
    pos.z = jointTable[0].z;

    rot = jointTable[1];

    newDList = limbDList = rootLimb->dList;

    // @recomp Push the limb's matrix group.
    gfx = push_limb_matrix_group(gfx, actor, 0);

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, 1, &newDList, &pos, &rot, actor, &gfx)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (newDList != NULL) {
            gSPMatrix(&gfx[0], Matrix_ToMtx(mtx), G_MTX_LOAD);
            gSPDisplayList(&gfx[1], newDList);
            gfx = &gfx[2];
            mtx++;
        } else {
            if (limbDList != NULL) {
                Matrix_ToMtx(mtx);
                mtx++;
            }
        }
    }

    // @recomp Pop the limb's matrix group and push the post-limb matrix group.
    gfx = pop_limb_matrix_group(gfx, actor);
    gfx = push_post_limb_matrix_group(gfx, actor, 0);

    if (postLimbDraw != NULL) {
        postLimbDraw(play, 1, &limbDList, &rot, actor, &gfx);
    }

    // @recomp Pop the post-limb matrix group.
    gfx = pop_post_limb_matrix_group(gfx, actor);

    if (rootLimb->child != LIMB_DONE) {
        gfx = SkelAnime_DrawFlexLimb(play, rootLimb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, actor,
                                     &mtx, gfx);
    }

    Matrix_Pop();

    return gfx;
}

extern MtxF gSkinLimbMatrices[];

RECOMP_PATCH void Skin_DrawImpl(Actor* actor, PlayState* play, Skin* skin, SkinPostDraw postDraw,
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

s32 scan_for_matrices(Gfx* start, Gfx* end) {
    s32 matrix_count = 0;
    Gfx* cur = start;
    // Count any G_MTX commands between the start and end commands.
    while (cur != end) {
        if ((cur->words.w0 >> 24) == G_MTX) {
            matrix_count++;
        }
        cur++;
    }
    return matrix_count;
}

void tag_actor_displaylists(Actor* actor, PlayState* play, Gfx* opa_start, Gfx* xlu_start) {
    OPEN_DISPS(play->state.gfxCtx);

    // Scan the commands written by the actor to see how many matrices were added.
    s32 opa_matrices = scan_for_matrices(opa_start, POLY_OPA_DISP);
    s32 xlu_matrices = scan_for_matrices(xlu_start, POLY_XLU_DISP);
    bool skipped = actor_get_interpolation_skipped(actor);

    // If the actor wrote at least one matrix in total and at most one matrix to each list, tag that matrix with the actor's id.
    if ((opa_matrices == 1 || xlu_matrices == 1) && opa_matrices <= 1 && xlu_matrices <= 1) {
        u32 cur_transform_id = actor_transform_id(actor);
        
        if (opa_matrices == 1) {
            // Fill in the slot that was reserved for a transform id.
            if (skipped) {
                gEXMatrixGroupDecomposedSkipPosRot(opa_start, cur_transform_id, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
            }
            else {
                gEXMatrixGroupDecomposedNormal(opa_start, cur_transform_id, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
            }
            
            // Pop the matrix group.
            gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);
        }

        if (xlu_matrices == 1) {
            // Fill in the slot that was reserved for a transform id.
            if (skipped) {
                gEXMatrixGroupDecomposedSkipPosRot(xlu_start, cur_transform_id + 1, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
            }
            else {
                gEXMatrixGroupDecomposedNormal(xlu_start, cur_transform_id + 1, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
            }

            // Pop the matrix groups.
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
        }
    }

    CLOSE_DISPS();
}

// @recomp Patched to automatically add transform tagging to actor matrices based on what DL commands they write in their draw function
RECOMP_PATCH void Actor_Draw(PlayState* play, Actor* actor) {
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

    // @recomp Add two noops into the opa and xlu displaylists to reserve space for a transform tag before the actor is drawn.
    Gfx* opa_tag_slot = POLY_OPA_DISP;
    Gfx* xlu_tag_slot = POLY_XLU_DISP;
    gDPNoOp(POLY_OPA_DISP++);
    gDPNoOp(POLY_OPA_DISP++);
    gDPNoOp(POLY_XLU_DISP++);
    gDPNoOp(POLY_XLU_DISP++);

    actor->draw(actor, play);

    tag_actor_displaylists(actor, play, opa_tag_slot, xlu_tag_slot);

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

    // @recomp Clear the actor's interpolation skipped flag.
    actor_clear_interpolation_skipped(actor);

    CLOSE_DISPS(play->state.gfxCtx);
}

ActorExtensionId z64recomp_extend_actor(s16 actor_id, u32 size);
ActorExtensionId z64recomp_extend_actor_all(u32 size);

void* z64recomp_get_extended_actor_data(Actor* actor, ActorExtensionId extension);
u32 z64recomp_get_actor_spawn_index(Actor* actor);

