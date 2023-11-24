__start = 0x80000000;

/* Dummy addresses that get recompiled into function calls */
recomp_get_item_inputs = 0x81000000;
recomp_puts = 0x81000004;

/* TODO pull these symbols from the elf file directly */
Player_PostLimbDrawGameplay = 0x80128BD0;
Player_DrawImpl = 0x801246F4;
gRegEditor = 0x801f3f60;
Audio_PlaySfx = 0x8019f0c8;
gSaveContext = 0x801ef670;
Interface_SetHudVisibility = 0x8010ef68;
PadMgr_GetInput = 0x80175f98;
Inventory_GetBtnBItem = 0x8012ec80;
gItemSlots = 0x801c2078;
_Printf = 0x8008e050;
