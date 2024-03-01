__start = 0x80000000;

/* Static symbols that aren't in the elf */
sSceneEntranceTable = 0x801C5720;

/* Dummy addresses that get recompiled into function calls */
recomp_puts = 0x81000000;
recomp_exit = 0x81000004;
recomp_handle_quicksave_actions = 0x81000008;
recomp_handle_quicksave_actions_main = 0x8100000C;
osRecvMesg_recomp = 0x81000010;
osSendMesg_recomp = 0x81000014;
recomp_get_gyro_deltas = 0x81000018;
recomp_get_aspect_ratio = 0x8100001C;
recomp_get_pending_warp = 0x81000020;
