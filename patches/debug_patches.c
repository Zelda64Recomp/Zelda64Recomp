#include "patches.h"
#include "misc_funcs.h"

void Message_FindMessage(PlayState* play, u16 textId);
extern SceneEntranceTableEntry sSceneEntranceTable[];

char text_buffer[1024];

void do_warp(PlayState* play, u16 entrance){
    play->nextEntrance = entrance;
    gSaveContext.save.entrance = play->nextEntrance;
    play->state.running = false;
    play->state.init = gGameStateOverlayTable[GAMESTATE_PLAY].init;
    play->state.size = gGameStateOverlayTable[GAMESTATE_PLAY].instanceSize;

    // Gets the ingame name, may be useful for mods later on
    // SceneEntranceTableEntry* scene_entrance_table_entry = &sSceneEntranceTable[entrance >> 9];
    // if (scene_entrance_table_entry->table) {
    //     EntranceTableEntry* entrance_table_entry = scene_entrance_table_entry->table[0];
    //     recomp_printf("entrance_table_entry: 0x%08X\n", (uintptr_t)entrance_table_entry);
    //     int scene_id = ABS(entrance_table_entry->sceneId);
    //     recomp_printf("scene_id: %d\n", scene_id);

    //     SceneTableEntry* scene = &gSceneTable[scene_id];
    //     int title_text_id = scene->titleTextId;
    //     recomp_printf("title_text_id: %d\n", title_text_id);
        
    //     recomp_printf("play->msgCtx.messageEntryTable: 0x%08X\n", (uintptr_t)play->msgCtx.messageEntryTableNes);

    //     Message_FindMessageNES(play, title_text_id);

    //     recomp_printf("font->messageStart: 0x%08X font->messageEnd: 0x%08X\n", play->msgCtx.font.messageStart, play->msgCtx.font.messageEnd);

    //     DmaRequest req;
    //     req.vromAddr = play->msgCtx.font.messageStart + SEGMENT_ROM_START(message_data_static);
    //     req.dramAddr = text_buffer;
    //     req.size = play->msgCtx.font.messageEnd;
    //     recomp_printf("dma from vrom 0x%08X to vram 0x%08x of 0x%04X bytes\n", req.vromAddr, req.dramAddr, req.size);
    //     DmaMgr_ProcessMsg(&req);

    //     if (text_buffer[2] != (char)0xFE) {
    //         recomp_printf("Invalid text\n");
    //         *(volatile int*)0 = 0;
    //     }

    //     recomp_printf("text_buffer: %s\n", text_buffer + 11);
    // }
    // int scene = (entrance & 0xFF00) >> 8;
}

void debug_play_update(PlayState* play) {
    u16 pending_warp = recomp_get_pending_warp();
    if (pending_warp != 0xFFFF) {
        do_warp(play, pending_warp);
    }

    u32 pending_set_time = recomp_get_pending_set_time();
    if (pending_set_time != 0xFFFF) {
        u8 day    = (pending_set_time >> 16) & 0xFF;
        u8 hour   = (pending_set_time >>  8) & 0xFF;
        u8 minute = (pending_set_time >>  0) & 0xFF;

        gSaveContext.save.time = CLOCK_TIME(hour, minute);
        gSaveContext.save.day = day;
    }
}
