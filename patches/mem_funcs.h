#ifndef __MEM_FUNCS_H__
#define __MEM_FUNCS_H__

#include "patch_helpers.h"

DECLARE_FUNC(u32, recomp_register_actor_extension, u32 actor_type, u32 size);
DECLARE_FUNC(u32, recomp_register_actor_extension_generic, u32 size);
DECLARE_FUNC(void, recomp_clear_all_actor_data);
DECLARE_FUNC(u32, recomp_create_actor_data, u32 actor_type);
DECLARE_FUNC(void, recomp_destroy_actor_data, u32 actor_handle);
DECLARE_FUNC(void*, recomp_get_actor_data, u32 actor_handle, u32 extension_handle, u32 actor_type);
DECLARE_FUNC(u32, recomp_get_actor_spawn_index, u32 actor_handle);

#endif
