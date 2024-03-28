#include "patches.h"

void room_load_hook(PlayState* play, Room* room) {
    if (play->sceneId == SCENE_00KEIKOKU && room->num == 0) {
        // Patch the branch commands that cause Clock Town geometry to disappear when forcing gbi branches.
        extern Gfx Z2_00KEIKOKU_room_00DL_00D490[];
        extern Gfx Z2_00KEIKOKU_room_00DL_00CD70[];
        extern Gfx Z2_00KEIKOKU_room_00DL_00D9C8[];
        Gfx* command = (Gfx*)SEGMENTED_TO_K0(Z2_00KEIKOKU_room_00DL_00D490 + 1);

        if ((command[0].words.w0 >> 24) == G_RDPHALF_1 && (command[1].words.w0 >> 24) == G_BRANCH_Z) {
            gSPNoOp(command + 0);
            gSPNoOp(command + 1);
        }
        
        command = (Gfx*)SEGMENTED_TO_K0(Z2_00KEIKOKU_room_00DL_00CD70 + 1);

        if ((command[0].words.w0 >> 24) == G_RDPHALF_1 && (command[1].words.w0 >> 24) == G_BRANCH_Z) {
            gSPNoOp(command + 0);
            gSPNoOp(command + 1);
        }
        
        command = (Gfx*)SEGMENTED_TO_K0(Z2_00KEIKOKU_room_00DL_00D9C8 + 1);

        if ((command[0].words.w0 >> 24) == G_RDPHALF_1 && (command[1].words.w0 >> 24) == G_BRANCH_Z) {
            gSPNoOp(command + 0);
            gSPNoOp(command + 1);
        }
    }
}