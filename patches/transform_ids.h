#ifndef __TRANSFORM_IDS_H__
#define __TRANSFORM_IDS_H__

#define CAMERA_TRANSFORM_ID 0x10U
#define CIRCLE_OVERLAY_TRANSFORM_ID 0x11U
#define CIRCLE_OVERLAY_TRANSFORM_PROJECTION_ID 0x12U
#define TEXTBOX_TRANSFORM_ID 0x13U
#define TEXTBOX_TRANSFORM_PROJECTION_ID 0x14U
#define LETTERBOX_TRANSFORM_ID 0x15U
#define LETTERBOX_TRANSFORM_PROJECTION_ID 0x16U
#define PAUSE_CURSOR_TRANSFORM_ID 0x17U
#define PAUSE_INFO_PANEL_TRANSFORM_ID 0x18U

#define SKYBOX_TRANSFORM_ID_START 0x100U

#define PARTICLE_TRANSFORM_ID_START 0x200U

#define SOARING_WINGS_TRANSFORM_ID 0x300U
#define SOARING_CAPSULE_TRANSFORM_ID 0x301U
#define SOARING_LENS_FLARE_TRANSFORM_ID 0x302U

#define BOWSTRING_TRANSFORM_ID 0x400U
#define HOOKSHOT_RETICLE_TRANSFORM_ID 0x401U

#define CUSTOM_LENS_FLARE_TRANSFORM_ID_START 0x500 // Count: 2
#define SUN_LENS_FLARE_TRANSFORM_ID 0x502 // Count: 1

#define GOHT_ROCKS_TRANSFORM_ID_START 0x600U // Count: 0x200
#define SPECIAL_EFFECTS_TRANSFORM_ID_START 0x800U

#define STAR_TRANSFORM_ID_START 0x1000U

#define EFFECT_TRANSFORM_ID_COUNT                 0x800U
#define EFFECT_SPARK_TRANSFORM_ID_START           0x2000U
#define EFFECT_BLURE_TRANSFORM_ID_START           (EFFECT_SPARK_TRANSFORM_ID_START + 2 * EFFECT_TRANSFORM_ID_COUNT) 
#define EFFECT_SHIELD_PARTICLE_TRANSFORM_ID_START (EFFECT_BLURE_TRANSFORM_ID_START + 2 * EFFECT_TRANSFORM_ID_COUNT)
#define EFFECT_TIRE_MARK_TRANSFORM_ID_START       (EFFECT_SHIELD_PARTICLE_TRANSFORM_ID_START + 2 * EFFECT_TRANSFORM_ID_COUNT)

#define ACTOR_TRANSFORM_LIMB_COUNT 256
#define ACTOR_TRANSFORM_ID_COUNT (ACTOR_TRANSFORM_LIMB_COUNT * 2) // One ID for each limb and another for each post-draw
#define ACTOR_TRANSFORM_ID_START 0x1000000U

// Use 16 bits of compiler-inserted padding to hold the actor's transform ID.
// 0x22 between halfDaysBits and world
#define actorIdByte0(actor) ((u8*)(actor))[0x22]
// 0x23 between halfDaysBits and world
#define actorIdByte1(actor) ((u8*)(actor))[0x23]
// 0x3A between audioFlags and focus
#define actorIdByte2(actor) ((u8*)(actor))[0x3A]

// Other unused padding:
// 0x3B between audioFlags and focus

static inline u32 actor_transform_id(Actor* actor) {
    u32 actor_id = 
        (actorIdByte0(actor) <<  0) |
        (actorIdByte1(actor) <<  8);

    return (actor_id * ACTOR_TRANSFORM_ID_COUNT) + ACTOR_TRANSFORM_ID_START;
}

typedef enum {
    ACTOR_TRANSFORM_FLAG_INTERPOLATION_SKIPPED = 1 << 0,
    ACTOR_CUSTOM_FLAG_1                        = 1 << 1,
} CustomActorFlags;

static inline u32 actor_get_interpolation_skipped(Actor* actor) {
    return (actorIdByte2(actor) & ACTOR_TRANSFORM_FLAG_INTERPOLATION_SKIPPED) != 0;
}

static inline void actor_set_interpolation_skipped(Actor* actor) {
    actorIdByte2(actor) |= ACTOR_TRANSFORM_FLAG_INTERPOLATION_SKIPPED;
}

static inline void actor_clear_interpolation_skipped(Actor* actor) {
    actorIdByte2(actor) &= ~ACTOR_TRANSFORM_FLAG_INTERPOLATION_SKIPPED;
}

static inline void actor_set_custom_flag_1(Actor* actor) {
    actorIdByte2(actor) |= ACTOR_CUSTOM_FLAG_1;
}

static inline void actor_clear_custom_flag_1(Actor* actor) {
    actorIdByte2(actor) &= ~ACTOR_CUSTOM_FLAG_1;
}

static inline bool actor_get_custom_flag_1(Actor* actor) {
    return (actorIdByte2(actor) & ACTOR_CUSTOM_FLAG_1) != 0;
}

void force_camera_interpolation();
void force_camera_skip_interpolation();

#endif
