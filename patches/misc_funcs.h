#ifndef __RECOMP_FUNCS_H__
#define __RECOMP_FUNCS_H__

#include "patch_helpers.h"

DECLARE_FUNC(void, recomp_load_overlays, u32 rom, void* ram, u32 size);
DECLARE_FUNC(void, recomp_puts, const char* data, u32 size);
DECLARE_FUNC(void, recomp_exit);
DECLARE_FUNC(void, recomp_handle_quicksave_actions, OSMesgQueue* enter_mq, OSMesgQueue* exit_mq);
DECLARE_FUNC(void, recomp_handle_quicksave_actions_main, OSMesgQueue* enter_mq, OSMesgQueue* exit_mq);
DECLARE_FUNC(u16, recomp_get_pending_warp);
DECLARE_FUNC(u32, recomp_get_pending_set_time);
DECLARE_FUNC(s32, recomp_get_autosave_enabled);

#endif
