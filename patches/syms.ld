__start = 0x80000000;

/* Static symbols that aren't in the elf */
sSceneEntranceTable = 0x801C5720;
D_808DE5B0 = 0x808DE5B0;
sHappyMaskSalesmanAnimationInfo = 0x80AD22C0;

/* Dummy addresses that get recompiled into function calls */
recomp_puts = 0x8F000000;
recomp_exit = 0x8F000004;
recomp_handle_quicksave_actions = 0x8F000008;
recomp_handle_quicksave_actions_main = 0x8F00000C;
osRecvMesg_recomp = 0x8F000010;
osSendMesg_recomp = 0x8F000014;
recomp_get_gyro_deltas = 0x8F000018;
recomp_get_aspect_ratio = 0x8F00001C;
recomp_get_pending_warp = 0x8F000020;
recomp_powf = 0x8F000024;
recomp_get_target_framerate = 0x8F000028;
recomp_get_targeting_mode = 0x8F00002C;
recomp_get_bgm_volume = 0x8F000030;
recomp_get_low_health_beeps_enabled = 0x8F000034;
__sinf_recomp = 0x8F000038;
__cosf_recomp = 0x8F00003C;
osCreateMesgQueue_recomp = 0x8F000048;
recomp_set_current_frame_poll_id = 0x8F00004C;
recomp_time_us = 0x8F000050;
recomp_measure_latency = 0x8F000054;
osViGetCurrentFramebuffer_recomp = 0x8F000058;
bzero_recomp = 0x8F00005C;
osFlashWriteArray_recomp = 0x8F000060;
osFlashWriteBuffer_recomp = 0x8F000064;
osWritebackDCache_recomp = 0x8F000068;
