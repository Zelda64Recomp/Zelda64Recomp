#include "patches.h"
#include "input.h"
#include "z64shrink_window.h"
#include "overlays/gamestates/ovl_file_choose/z_file_select.h"

extern u64 gFileSelOptionsButtonENGTex[];
extern u64 gFileSelQuitButtonENGTex[];

void FileSelect_Main(GameState* thisx);
void FileSelect_InitContext(GameState* thisx);

// @recomp The options button is now the quit button, so close recomp instead of opening the options.
void FileSelect_RotateToOptions(GameState* thisx) {
    recomp_exit();
}

void FileSelect_Init(GameState* thisx) {
    s32 pad;
    FileSelectState* this = (FileSelectState*)thisx;
    size_t size;

    GameState_SetFramerateDivisor(&this->state, 1);
    Matrix_Init(&this->state);
    ShrinkWindow_Init();
    View_Init(&this->view, this->state.gfxCtx);

    // @recomp manually relocate these symbols as the recompiler doesn't do this automatically for patches yet.
    GameStateOverlay* ovl = &gGameStateOverlayTable[GAMESTATE_FILE_SELECT];
    this->state.main = (void*)((u32)FileSelect_Main - (u32)ovl->vramStart + (u32)ovl->loadedRamAddr);
    this->state.destroy = (void*)((u32)FileSelect_Destroy - (u32)ovl->vramStart + (u32)ovl->loadedRamAddr);

    FileSelect_InitContext(&this->state);
    Font_LoadOrderedFont(&this->font);

    size = SEGMENT_ROM_SIZE(title_static);
    this->staticSegment = THA_AllocTailAlign16(&this->state.tha, size);
    DmaMgr_SendRequest0(this->staticSegment, SEGMENT_ROM_START(title_static), size);

    size = SEGMENT_ROM_SIZE(parameter_static);
    this->parameterSegment = THA_AllocTailAlign16(&this->state.tha, size);
    DmaMgr_SendRequest0(this->parameterSegment, SEGMENT_ROM_START(parameter_static), size);

    size = gObjectTable[OBJECT_MAG].vromEnd - gObjectTable[OBJECT_MAG].vromStart;
    this->titleSegment = THA_AllocTailAlign16(&this->state.tha, size);
    DmaMgr_SendRequest0(this->titleSegment, gObjectTable[OBJECT_MAG].vromStart, size);

    Audio_SetSpec(0xA);
    // Setting ioData to 1 and writing it to ioPort 7 will skip the harp intro
    Audio_PlaySequenceWithSeqPlayerIO(SEQ_PLAYER_BGM_MAIN, NA_BGM_FILE_SELECT, 0, 7, 1);

    // @recomp Replace the contents of the options button's texture with the quit button texture.
    // Lower impact than replacing the entire `FileSelect_DrawWindowContents` function.
    void* options_button_texture = (void*)(this->staticSegment + (u32)gFileSelOptionsButtonENGTex - 0x01000000);
    void* quit_button_texture = (void*)(this->staticSegment + (u32)gFileSelQuitButtonENGTex - 0x01000000);
    Lib_MemCpy(options_button_texture, quit_button_texture, 64 * 16 * sizeof(u16));
}
