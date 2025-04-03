#include "patches.h"
#include "extended_actors.h"
#include "transform_ids.h"
#include "actor_funcs.h"

// Use 32 bits of compiler-inserted padding to hold the actor's slot.
// 0x22 between halfDaysBits and world
#define actorIdByte0(actor) ((u8*)(actor))[0x22]
// 0x23 between halfDaysBits and world
#define actorIdByte1(actor) ((u8*)(actor))[0x23]
// 0x3A between audioFlags and focus
#define actorIdByte2(actor) ((u8*)(actor))[0x3A]
// 0x3B between audioFlags and focus
#define actorIdByte3(actor) ((u8*)(actor))[0x3B]

u32 actor_get_slot(Actor* actor) {
    return (actorIdByte0(actor) << 24) | (actorIdByte1(actor) << 16) | (actorIdByte2(actor) << 8) | (actorIdByte3(actor) << 0);
}

void actor_set_slot(Actor* actor, ActorExtensionId slot) {
    u32 b0 = (slot >> 24) & 0xFF;
    u32 b1 = (slot >> 16) & 0xFF;
    u32 b2 = (slot >>  8) & 0xFF;
    u32 b3 = (slot >>  0) & 0xFF;

    actorIdByte0(actor) = b0;
    actorIdByte1(actor) = b1;
    actorIdByte2(actor) = b2;
    actorIdByte3(actor) = b3;
}

RECOMP_EXPORT ActorExtensionId z64recomp_extend_actor(s16 actor_id, u32 size) {
    return recomp_register_actor_extension(actor_id, size);
}

RECOMP_EXPORT ActorExtensionId z64recomp_extend_actor_all(u32 size) {
    return recomp_register_actor_extension_generic(size);
}

RECOMP_EXPORT void* z64recomp_get_extended_actor_data(Actor* actor, ActorExtensionId extension) {
    return recomp_get_actor_data(actor_get_slot(actor), extension, actor->id);
}

RECOMP_EXPORT u32 z64recomp_get_actor_spawn_index(Actor* actor) {
    return recomp_get_actor_spawn_index(actor_get_slot(actor));
}

RECOMP_EXPORT u32 actor_transform_id(Actor* actor) {
    u32 spawn_index = z64recomp_get_actor_spawn_index(actor);

    return (spawn_index * ACTOR_TRANSFORM_ID_COUNT) + ACTOR_TRANSFORM_ID_START;
}

typedef enum {
    ACTOR_TRANSFORM_FLAG_INTERPOLATION_SKIPPED = 1 << 0,
    ACTOR_CUSTOM_FLAG_1                        = 1 << 1,
} CustomActorFlags;

typedef struct {
    CustomActorFlags flags;
} BaseActorExtensionData;

ActorExtensionId base_actor_extension_handle;

void register_base_actor_extensions() {
    base_actor_extension_handle = z64recomp_extend_actor_all(sizeof(BaseActorExtensionData));
}

BaseActorExtensionData* get_base_extension_data(Actor* actor) {
    return (BaseActorExtensionData*)z64recomp_get_extended_actor_data(actor, base_actor_extension_handle);
}

RECOMP_EXPORT u32 actor_get_interpolation_skipped(Actor* actor) {
    return (get_base_extension_data(actor)->flags & ACTOR_TRANSFORM_FLAG_INTERPOLATION_SKIPPED) != 0;
}

RECOMP_EXPORT void actor_set_interpolation_skipped(Actor* actor) {
    get_base_extension_data(actor)->flags |= ACTOR_TRANSFORM_FLAG_INTERPOLATION_SKIPPED;
}

RECOMP_EXPORT void actor_clear_interpolation_skipped(Actor* actor) {
    get_base_extension_data(actor)->flags &= ~ACTOR_TRANSFORM_FLAG_INTERPOLATION_SKIPPED;
}

void actor_set_custom_flag_1(Actor* actor) {
    get_base_extension_data(actor)->flags |= ACTOR_CUSTOM_FLAG_1;
}

bool actor_get_custom_flag_1(Actor* actor) {
    return (get_base_extension_data(actor)->flags & ACTOR_CUSTOM_FLAG_1) != 0;
}
