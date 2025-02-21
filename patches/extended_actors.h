#ifndef __EXTENDED_ACTORS_H__
#define __EXTENDED_ACTORS_H__

#include "global.h"

typedef u32 ActorExtensionId;

ActorExtensionId z64recomp_extend_actor(s16 actor_id, u32 size);
ActorExtensionId z64recomp_extend_actor_all(u32 size);

void* z64recomp_get_extended_actor_data(Actor* actor, ActorExtensionId extension);
u32 z64recomp_get_actor_spawn_index(Actor* actor);

#endif
