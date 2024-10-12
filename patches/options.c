#include "patches.h"
#include "misc_funcs.h"
#include "z64shrink_window.h"
#include "overlays/gamestates/ovl_file_choose/z_file_select.h"

#define FS_BTN_CONFIRM_REWIND 2

INCBIN(rewind_button_texture, "rewind.ia16.bin");

extern u64 gFileSelFileNameBoxTex[];
extern u64 gFileSelConnectorTex[];
extern u64 gFileSelBlankButtonTex[];
extern u64 gFileSelBigButtonHighlightTex[];
extern u64 gFileSelFileInfoBox0Tex[];
extern u64 gFileSelFileInfoBox1Tex[];
extern u64 gFileSelFileInfoBox2Tex[];
extern u64 gFileSelFileInfoBox3Tex[];
extern u64 gFileSelFileInfoBox4Tex[];
extern u64 gFileSelFileExtraInfoBox0Tex[];
extern u64 gFileSelFileExtraInfoBox1Tex[];
extern u64 gFileSelOptionsButtonENGTex[];
extern u64 gFileSelPleaseSelectAFileENGTex[];
extern u64 gFileSelOpenThisFileENGTex[];
extern u64 gFileSelCopyWhichFileENGTex[];
extern u64 gFileSelCopyToWhichFileENGTex[];
extern u64 gFileSelAreYouSureCopyENGTex[];
extern u64 gFileSelFileCopiedENGTex[];
extern u64 gFileSelEraseWhichFileENGTex[];
extern u64 gFileSelAreYouSureEraseENGTex[];
extern u64 gFileSelFileErasedENGTex[];
extern u64 gFileSelNoFileToCopyENGTex[];
extern u64 gFileSelNoFileToEraseENGTex[];
extern u64 gFileSelNoEmptyFileENGTex[];
extern u64 gFileSelFileEmptyENGTex[];
extern u64 gFileSelFileInUseENGTex[];
extern u64 gFileSelFile1ButtonENGTex[];
extern u64 gFileSelFile2ButtonENGTex[];
extern u64 gFileSelFile3ButtonENGTex[];
extern u64 gFileSelCopyButtonENGTex[];
extern u64 gFileSelEraseButtonENGTex[];
extern u64 gFileSelYesButtonENGTex[];
extern u64 gFileSelQuitButtonENGTex[];
extern s16 D_80814280[];
extern s16 sWindowContentColors[];
extern TexturePtr sFileInfoBoxTextures[];
extern TexturePtr sTitleLabels[];
extern TexturePtr sWarningLabels[];
extern TexturePtr sFileButtonTextures[];
extern TexturePtr sActionButtonTextures[];
extern s16 sFileInfoBoxPartWidths[];
extern s16 sWalletFirstDigit[];
extern s16 D_80814620[];
extern s16 D_80814628[];
extern s16 D_80814630[];
extern s16 D_80814638[];
extern s16 D_80814644[];
extern s16 D_8081464C[];

// @recomp Added a third position for the rewind button.
s16 D_80814650_patched[] = { 940, 944, 948 };

void FileSelect_Main(GameState* thisx);
void FileSelect_InitContext(GameState* thisx);
void FileSelect_DrawFileInfo(GameState *thisx, s16 fileIndex);
void FileSelect_SplitNumber(u16 value, u16 *hundreds, u16 *tens, u16 *ones);

// @recomp The options button is now the quit button, so close recomp instead of opening the options.
RECOMP_PATCH void FileSelect_RotateToOptions(GameState* thisx) {
    recomp_exit();
}

RECOMP_PATCH void FileSelect_Init(GameState* thisx) {
    s32 pad;
    FileSelectState* this = (FileSelectState*)thisx;
    size_t size;

    GameState_SetFramerateDivisor(&this->state, 1);
    Matrix_Init(&this->state);
    ShrinkWindow_Init();
    View_Init(&this->view, this->state.gfxCtx);

    this->state.main = FileSelect_Main;
    this->state.destroy = FileSelect_Destroy;

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


RECOMP_PATCH void FileSelect_SetWindowContentVtx(GameState *thisx) {
    FileSelectState *this = (FileSelectState *)thisx;
    u16 vtxId;
    s16 j;
    s16 i;
    s16 spAC;
    u16 spA4[3];
    u16 *ptr;
    s32 posY;
    s32 relPosY;
    s32 tempPosY;
    s32 posX;
    s32 index;

    this->windowContentVtx = GRAPH_ALLOC(this->state.gfxCtx, 960 * sizeof(Vtx));

    // Initialize all windowContentVtx
    for (vtxId = 0; vtxId < 960; vtxId += 4) {
        // x-coord (left)
        this->windowContentVtx[vtxId + 0].v.ob[0] = this->windowContentVtx[vtxId + 2].v.ob[0] = 0x12C;
        // x-coord (right)
        this->windowContentVtx[vtxId + 1].v.ob[0] = this->windowContentVtx[vtxId + 3].v.ob[0] =
            this->windowContentVtx[vtxId + 0].v.ob[0] + 16;

        // y-coord (top)
        this->windowContentVtx[vtxId + 0].v.ob[1] = this->windowContentVtx[vtxId + 1].v.ob[1] = 0;
        // y-coord (bottom)
        this->windowContentVtx[vtxId + 2].v.ob[1] = this->windowContentVtx[vtxId + 3].v.ob[1] =
            this->windowContentVtx[vtxId + 0].v.ob[1] - 16;

        // z-coordinate
        this->windowContentVtx[vtxId + 0].v.ob[2] = this->windowContentVtx[vtxId + 1].v.ob[2] =
            this->windowContentVtx[vtxId + 2].v.ob[2] = this->windowContentVtx[vtxId + 3].v.ob[2] = 0;

        // flag
        this->windowContentVtx[vtxId + 0].v.flag = this->windowContentVtx[vtxId + 1].v.flag =
            this->windowContentVtx[vtxId + 2].v.flag = this->windowContentVtx[vtxId + 3].v.flag = 0;

        // texture coordinates
        this->windowContentVtx[vtxId + 0].v.tc[0] = this->windowContentVtx[vtxId + 0].v.tc[1] =
            this->windowContentVtx[vtxId + 1].v.tc[1] = this->windowContentVtx[vtxId + 2].v.tc[0] = 0;
        this->windowContentVtx[vtxId + 1].v.tc[0] = this->windowContentVtx[vtxId + 2].v.tc[1] =
            this->windowContentVtx[vtxId + 3].v.tc[0] = this->windowContentVtx[vtxId + 3].v.tc[1] = 0x200;

        // alpha
        this->windowContentVtx[vtxId + 0].v.cn[0] = this->windowContentVtx[vtxId + 1].v.cn[0] =
            this->windowContentVtx[vtxId + 2].v.cn[0] = this->windowContentVtx[vtxId + 3].v.cn[0] =
            this->windowContentVtx[vtxId + 0].v.cn[1] = this->windowContentVtx[vtxId + 1].v.cn[1] =
            this->windowContentVtx[vtxId + 2].v.cn[1] = this->windowContentVtx[vtxId + 3].v.cn[1] =
            this->windowContentVtx[vtxId + 0].v.cn[2] = this->windowContentVtx[vtxId + 1].v.cn[2] =
            this->windowContentVtx[vtxId + 2].v.cn[2] = this->windowContentVtx[vtxId + 3].v.cn[2] =
            this->windowContentVtx[vtxId + 0].v.cn[3] = this->windowContentVtx[vtxId + 1].v.cn[3] =
            this->windowContentVtx[vtxId + 2].v.cn[3] =
            this->windowContentVtx[vtxId + 3].v.cn[3] = 255;
    }

    /** Title Label **/

    // x-coord (left)
    this->windowContentVtx[0].v.ob[0] = this->windowContentVtx[2].v.ob[0] = this->windowPosX;
    // x-coord (right)
    this->windowContentVtx[1].v.ob[0] = this->windowContentVtx[3].v.ob[0] = this->windowContentVtx[0].v.ob[0] + 0x80;
    // y-coord (top)
    this->windowContentVtx[0].v.ob[1] = this->windowContentVtx[1].v.ob[1] = 0x48;
    // y-coord (bottom)
    this->windowContentVtx[2].v.ob[1] = this->windowContentVtx[3].v.ob[1] = this->windowContentVtx[0].v.ob[1] - 0x10;
    // texture coordinates
    this->windowContentVtx[1].v.tc[0] = this->windowContentVtx[3].v.tc[0] = 0x1000;

    /** File InfoBox **/

    // Loop through 3 files
    for (vtxId = 4, i = 0; i < 3; i++) {
        posX = this->windowPosX - 6;

        // Loop through 7 textures
        for (j = 0; j < 7; j++, vtxId += 4) {
            // x-coord (left)
            this->windowContentVtx[vtxId + 0].v.ob[0] = this->windowContentVtx[vtxId + 2].v.ob[0] = posX;
            // x-coord (right)
            this->windowContentVtx[vtxId + 1].v.ob[0] = this->windowContentVtx[vtxId + 3].v.ob[0] =
                this->windowContentVtx[vtxId + 0].v.ob[0] + sFileInfoBoxPartWidths[j];

            // y-coord(top)
            this->windowContentVtx[vtxId + 0].v.ob[1] = this->windowContentVtx[vtxId + 1].v.ob[1] =
                this->fileNamesY[i] + 0x2C;
            // y-coord (bottom)
            this->windowContentVtx[vtxId + 2].v.ob[1] = this->windowContentVtx[vtxId + 3].v.ob[1] =
                this->windowContentVtx[vtxId + 0].v.ob[1] - 0x38;

            // texture coordinates
            this->windowContentVtx[vtxId + 1].v.tc[0] = this->windowContentVtx[vtxId + 3].v.tc[0] =
                sFileInfoBoxPartWidths[j] << 5;
            this->windowContentVtx[vtxId + 2].v.tc[1] = this->windowContentVtx[vtxId + 3].v.tc[1] = 0x700;

            // Update X position
            posX += sFileInfoBoxPartWidths[j];
        }
    }

    // File Buttons

    posX = this->windowPosX - 6;
    posY = 44;

    // Loop through 3 files
    for (j = 0; j < 3; j++, vtxId += 16, posY -= 0x10) {

        /* File Button */

        // x-coord (left)
        this->windowContentVtx[vtxId + 0].v.ob[0] = this->windowContentVtx[vtxId + 2].v.ob[0] = posX;
        // x-coord (right)
        this->windowContentVtx[vtxId + 1].v.ob[0] = this->windowContentVtx[vtxId + 3].v.ob[0] =
            this->windowContentVtx[vtxId + 0].v.ob[0] + 0x40;

        // y-coord(top)
        this->windowContentVtx[vtxId + 0].v.ob[1] = this->windowContentVtx[vtxId + 1].v.ob[1] =
            this->buttonYOffsets[j] + posY;
        // y-coord (bottom)
        this->windowContentVtx[vtxId + 2].v.ob[1] = this->windowContentVtx[vtxId + 3].v.ob[1] =
            this->windowContentVtx[vtxId + 0].v.ob[1] - 0x10;

        // texture coordinates
        this->windowContentVtx[vtxId + 1].v.tc[0] = this->windowContentVtx[vtxId + 3].v.tc[0] = 0x800;

        /* File Name Box */

        // x-coord (left)
        this->windowContentVtx[vtxId + 4].v.ob[0] = this->windowContentVtx[vtxId + 6].v.ob[0] = posX + 0x40;
        // x-coord (right)
        this->windowContentVtx[vtxId + 5].v.ob[0] = this->windowContentVtx[vtxId + 7].v.ob[0] =
            this->windowContentVtx[vtxId + 4].v.ob[0] + 0x6C;

        // y-coord(top)
        this->windowContentVtx[vtxId + 4].v.ob[1] = this->windowContentVtx[vtxId + 5].v.ob[1] =
            this->buttonYOffsets[j] + posY;
        // y-coord (bottom)
        this->windowContentVtx[vtxId + 6].v.ob[1] = this->windowContentVtx[vtxId + 7].v.ob[1] =
            this->windowContentVtx[vtxId + 4].v.ob[1] - 0x10;

        // texture coordinates
        this->windowContentVtx[vtxId + 5].v.tc[0] = this->windowContentVtx[vtxId + 7].v.tc[0] = 0xD80;

        /* Connectors */

        // x-coord (left)
        this->windowContentVtx[vtxId + 8].v.ob[0] = this->windowContentVtx[vtxId + 10].v.ob[0] = posX + 0x34;
        // x-coord (right)
        this->windowContentVtx[vtxId + 9].v.ob[0] = this->windowContentVtx[vtxId + 11].v.ob[0] =
            this->windowContentVtx[vtxId + 8].v.ob[0] + 0x18;

        // y-coord(top)
        this->windowContentVtx[vtxId + 8].v.ob[1] = this->windowContentVtx[vtxId + 9].v.ob[1] =
            this->buttonYOffsets[j] + posY;
        // y-coord (bottom)
        this->windowContentVtx[vtxId + 10].v.ob[1] = this->windowContentVtx[vtxId + 11].v.ob[1] =
            this->windowContentVtx[vtxId + 8].v.ob[1] - 0x10;

        // texture coordinates
        this->windowContentVtx[vtxId + 9].v.tc[0] = this->windowContentVtx[vtxId + 11].v.tc[0] = 0x300;

        /* Blank Button (Owl Save) */

        // x-coord (left)
        this->windowContentVtx[vtxId + 12].v.ob[0] = this->windowContentVtx[vtxId + 14].v.ob[0] = posX + 0xA9;
        // x-coord (right)
        this->windowContentVtx[vtxId + 13].v.ob[0] = this->windowContentVtx[vtxId + 15].v.ob[0] =
            this->windowContentVtx[vtxId + 12].v.ob[0] + 0x34;

        // y-coord(top)
        this->windowContentVtx[vtxId + 12].v.ob[1] = this->windowContentVtx[vtxId + 13].v.ob[1] =
            this->buttonYOffsets[j] + posY;
        // y-coord (bottom)
        this->windowContentVtx[vtxId + 14].v.ob[1] = this->windowContentVtx[vtxId + 15].v.ob[1] =
            this->windowContentVtx[vtxId + 12].v.ob[1] - 0x10;

        // texture coordinates
        this->windowContentVtx[vtxId + 13].v.tc[0] = this->windowContentVtx[vtxId + 15].v.tc[0] = 0x680;
    }

    posY = 44;

    // Loop through 3 files
    for (j = 0; j < 3; j++, posY -= 16) {
        if (!gSaveContext.flashSaveAvailable) {
            // Should skip vtxId
            // vtxId += 268;
            continue;
        }

        // Account for owl-save offset

        spAC = j;
        if (this->isOwlSave[j + 2]) {
            spAC = j + 2;
        }

        /* File name */

        posX = this->windowPosX - 6;

        if ((this->configMode == 0x10) && (j == this->copyDestFileIndex)) {
            relPosY = this->fileNamesY[j] + 0x2C;
        }
        else if (((this->configMode == 0x11) || (this->configMode == 0x12)) && (j == this->copyDestFileIndex)) {
            relPosY = this->buttonYOffsets[j] + posY;
        }
        else {
            relPosY = posY + this->buttonYOffsets[j] + this->fileNamesY[j];
        }

        tempPosY = relPosY - 2;

        // Loop through 8 characters of file name
        for (i = 0; i < 8; i++, vtxId += 4) {

            index = this->fileNames[j][i];

            /* File Name */

            // x-coord (left)
            this->windowContentVtx[vtxId + 0].v.ob[0] = this->windowContentVtx[vtxId + 2].v.ob[0] =
                D_80814280[index] + posX + 0x4E;
            // x-coord (right)
            this->windowContentVtx[vtxId + 1].v.ob[0] = this->windowContentVtx[vtxId + 3].v.ob[0] =
                this->windowContentVtx[vtxId + 0].v.ob[0] + 0xB;

            // y-coord(top)
            this->windowContentVtx[vtxId + 0].v.ob[1] = this->windowContentVtx[vtxId + 1].v.ob[1] = tempPosY;
            // y-coord (bottom)
            this->windowContentVtx[vtxId + 2].v.ob[1] = this->windowContentVtx[vtxId + 3].v.ob[1] =
                this->windowContentVtx[vtxId + 0].v.ob[1] - 0xC;

            /* File Name Shadow */

            // x-coord (left)
            this->windowContentVtx[vtxId + 32].v.ob[0] = this->windowContentVtx[vtxId + 34].v.ob[0] =
                D_80814280[index] + posX + 0x4F;
            // x-coord (right)
            this->windowContentVtx[vtxId + 33].v.ob[0] = this->windowContentVtx[vtxId + 35].v.ob[0] =
                this->windowContentVtx[vtxId + 32].v.ob[0] + 0xB;

            // y-coord(top)
            this->windowContentVtx[vtxId + 32].v.ob[1] = this->windowContentVtx[vtxId + 33].v.ob[1] = tempPosY - 1;
            // y-coord (bottom)
            this->windowContentVtx[vtxId + 34].v.ob[1] = this->windowContentVtx[vtxId + 35].v.ob[1] =
                this->windowContentVtx[vtxId + 32].v.ob[1] - 0xC;

            // Update X position
            posX += 10;
        }
        // Account for the shadow
        vtxId += 32;

        /* Rupee Digits */

        posX = this->windowPosX + 14;
        tempPosY = relPosY - 0x18;

        FileSelect_SplitNumber(this->rupees[spAC], &spA4[0], &spA4[1], &spA4[2]);

        index = sWalletFirstDigit[this->walletUpgrades[spAC]];

        ptr = &spA4[index];

        for (i = 0; i < 3; i++, vtxId += 4, ptr++) {

            /* Rupee Digits */

            // x-coord (left)
            this->windowContentVtx[vtxId + 0].v.ob[0] = this->windowContentVtx[vtxId + 2].v.ob[0] =
                D_80814280[*ptr] + posX;
            // x-coord (right)
            this->windowContentVtx[vtxId + 1].v.ob[0] = this->windowContentVtx[vtxId + 3].v.ob[0] =
                this->windowContentVtx[vtxId + 0].v.ob[0] + D_80814628[i];

            // y-coord(top)
            this->windowContentVtx[vtxId + 0].v.ob[1] = this->windowContentVtx[vtxId + 1].v.ob[1] = tempPosY;
            // y-coord (bottom)
            this->windowContentVtx[vtxId + 2].v.ob[1] = this->windowContentVtx[vtxId + 3].v.ob[1] =
                this->windowContentVtx[vtxId + 0].v.ob[1] - D_80814630[i];

            /* Rupee Digits Shadow */

            // x-coord (left)
            this->windowContentVtx[vtxId + 12].v.ob[0] = this->windowContentVtx[vtxId + 14].v.ob[0] =
                this->windowContentVtx[vtxId + 0].v.ob[0] + 1;
            // x-coord (right)
            this->windowContentVtx[vtxId + 13].v.ob[0] = this->windowContentVtx[vtxId + 15].v.ob[0] =
                this->windowContentVtx[vtxId + 12].v.ob[0] + D_80814628[i];

            // y-coord(top)
            this->windowContentVtx[vtxId + 12].v.ob[1] = this->windowContentVtx[vtxId + 13].v.ob[1] = tempPosY - 1;
            // y-coord (bottom)
            this->windowContentVtx[vtxId + 14].v.ob[1] = this->windowContentVtx[vtxId + 15].v.ob[1] =
                this->windowContentVtx[vtxId + 12].v.ob[1] - D_80814630[i];

            // Update X position
            posX += D_80814620[i];
        }

        // Account for the shadow
        vtxId += 12;

        /* Mask Count */

        posX = this->windowPosX + 42;
        tempPosY = relPosY - 0x2A;

        FileSelect_SplitNumber(this->maskCount[spAC], &spA4[0], &spA4[1], &spA4[2]);

        for (i = 1; i < 3; i++, vtxId += 4) {

            /* Mask Count */

            // x-coord (left)
            this->windowContentVtx[vtxId + 0].v.ob[0] = this->windowContentVtx[vtxId + 2].v.ob[0] =
                D_80814280[spA4[i]] + posX;
            // x-coord (right)
            this->windowContentVtx[vtxId + 1].v.ob[0] = this->windowContentVtx[vtxId + 3].v.ob[0] =
                this->windowContentVtx[vtxId + 0].v.ob[0] + D_80814628[i];

            // y-coord(top)
            this->windowContentVtx[vtxId + 0].v.ob[1] = this->windowContentVtx[vtxId + 1].v.ob[1] = tempPosY;
            // y-coord (bottom)
            this->windowContentVtx[vtxId + 2].v.ob[1] = this->windowContentVtx[vtxId + 3].v.ob[1] =
                this->windowContentVtx[vtxId + 0].v.ob[1] - D_80814630[i];

            /* Mask Count Shadow */

            // x-coord (left)
            this->windowContentVtx[vtxId + 8].v.ob[0] = this->windowContentVtx[vtxId + 10].v.ob[0] =
                this->windowContentVtx[vtxId + 0].v.ob[0] + 1;
            // x-coord (right)
            this->windowContentVtx[vtxId + 9].v.ob[0] = this->windowContentVtx[vtxId + 11].v.ob[0] =
                this->windowContentVtx[vtxId + 8].v.ob[0] + D_80814628[i];

            // y-coord(top)
            this->windowContentVtx[vtxId + 8].v.ob[1] = this->windowContentVtx[vtxId + 9].v.ob[1] = tempPosY - 1;
            // y-coord (bottom)
            this->windowContentVtx[vtxId + 10].v.ob[1] = this->windowContentVtx[vtxId + 11].v.ob[1] =
                this->windowContentVtx[vtxId + 8].v.ob[1] - D_80814630[i];

            // Update X position
            posX += D_80814620[i];
        }

        // Account for the shadow
        vtxId += 8;

        /* Hearts */

        posX = this->windowPosX + 63;
        tempPosY = relPosY - 0x10;

        // Loop through 20 hearts
        for (i = 0; i < 20; i++, vtxId += 4, posX += 9) {
            // x-coord (left)
            this->windowContentVtx[vtxId + 0].v.ob[0] = this->windowContentVtx[vtxId + 2].v.ob[0] = posX;
            // x-coord (right)
            this->windowContentVtx[vtxId + 1].v.ob[0] = this->windowContentVtx[vtxId + 3].v.ob[0] =
                this->windowContentVtx[vtxId + 0].v.ob[0] + 0xA;

            // y-coord(top)
            this->windowContentVtx[vtxId + 0].v.ob[1] = this->windowContentVtx[vtxId + 1].v.ob[1] = tempPosY;
            // y-coord (bottom)
            this->windowContentVtx[vtxId + 2].v.ob[1] = this->windowContentVtx[vtxId + 3].v.ob[1] =
                this->windowContentVtx[vtxId + 0].v.ob[1] - 0xA;

            // New row of hearts next iteration
            if (i == 9) {
                posX = this->windowPosX + (63 - 9);
                tempPosY -= 8;
            }
        }

        /* Quest Remains */

        posX = this->windowPosX + 64;
        tempPosY = relPosY - 0x20;

        // Loop through 4 Remains
        for (i = 0; i < 4; i++, vtxId += 4, posX += 0x18) {
            // x-coord (left)
            this->windowContentVtx[vtxId + 0].v.ob[0] = this->windowContentVtx[vtxId + 2].v.ob[0] = posX;
            // x-coord (right)
            this->windowContentVtx[vtxId + 1].v.ob[0] = this->windowContentVtx[vtxId + 3].v.ob[0] =
                this->windowContentVtx[vtxId + 0].v.ob[0] + 0x14;

            // y-coord(top)
            this->windowContentVtx[vtxId + 0].v.ob[1] = this->windowContentVtx[vtxId + 1].v.ob[1] = tempPosY;
            // y-coord (bottom)
            this->windowContentVtx[vtxId + 2].v.ob[1] = this->windowContentVtx[vtxId + 3].v.ob[1] =
                this->windowContentVtx[vtxId + 0].v.ob[1] - 0x14;

            // texture coordinates
            this->windowContentVtx[vtxId + 1].v.tc[0] = this->windowContentVtx[vtxId + 2].v.tc[1] =
                this->windowContentVtx[vtxId + 3].v.tc[0] = this->windowContentVtx[vtxId + 3].v.tc[1] = 0x400;
        }

        /* Rupee Icon */

        // posX = this->windowPosX - 1;
        tempPosY = relPosY - 0x15;

        // x-coord (left)
        this->windowContentVtx[vtxId + 0].v.ob[0] = this->windowContentVtx[vtxId + 2].v.ob[0] = this->windowPosX - 1;
        // x-coord (right)
        this->windowContentVtx[vtxId + 1].v.ob[0] = this->windowContentVtx[vtxId + 3].v.ob[0] =
            this->windowContentVtx[vtxId + 0].v.ob[0] + 0x10;

        // y-coord(top)
        this->windowContentVtx[vtxId + 0].v.ob[1] = this->windowContentVtx[vtxId + 1].v.ob[1] = tempPosY;
        // y-coord (bottom)
        this->windowContentVtx[vtxId + 2].v.ob[1] = this->windowContentVtx[vtxId + 3].v.ob[1] =
            this->windowContentVtx[vtxId + 0].v.ob[1] - 0x10;

        // texture coordinates
        this->windowContentVtx[vtxId + 1].v.tc[0] = this->windowContentVtx[vtxId + 3].v.tc[0] = 0x200;
        this->windowContentVtx[vtxId + 2].v.tc[1] = this->windowContentVtx[vtxId + 3].v.tc[1] = 0x200;

        vtxId += 4;

        /* Heart Piece Count */

        // posX = this->windowPosX + 0x27;
        tempPosY = relPosY - 0x15;

        // x-coord (left)
        this->windowContentVtx[vtxId + 0].v.ob[0] = this->windowContentVtx[vtxId + 2].v.ob[0] = this->windowPosX + 0x27;
        // x-coord (right)
        this->windowContentVtx[vtxId + 1].v.ob[0] = this->windowContentVtx[vtxId + 3].v.ob[0] =
            this->windowContentVtx[vtxId + 0].v.ob[0] + 0x18;

        // y-coord(top)
        this->windowContentVtx[vtxId + 0].v.ob[1] = this->windowContentVtx[vtxId + 1].v.ob[1] = tempPosY;
        // y-coord (bottom)
        this->windowContentVtx[vtxId + 2].v.ob[1] = this->windowContentVtx[vtxId + 3].v.ob[1] =
            this->windowContentVtx[vtxId + 0].v.ob[1] - 0x10;

        // texture coordinates
        this->windowContentVtx[vtxId + 1].v.tc[0] = this->windowContentVtx[vtxId + 3].v.tc[0] = 0x300;
        this->windowContentVtx[vtxId + 2].v.tc[1] = this->windowContentVtx[vtxId + 3].v.tc[1] = 0x200;

        vtxId += 4;

        /* Mask Text */

        // posX = this->windowPosX - 10;
        tempPosY = relPosY - 0x27;

        this->windowContentVtx[vtxId + 0].v.ob[0] = this->windowContentVtx[vtxId + 2].v.ob[0] = this->windowPosX - 10;

        this->windowContentVtx[vtxId + 1].v.ob[0] = this->windowContentVtx[vtxId + 3].v.ob[0] =
            this->windowContentVtx[vtxId + 0].v.ob[0] + 0x40;

        this->windowContentVtx[vtxId + 0].v.ob[1] = this->windowContentVtx[vtxId + 1].v.ob[1] = tempPosY;

        this->windowContentVtx[vtxId + 2].v.ob[1] = this->windowContentVtx[vtxId + 3].v.ob[1] =
            this->windowContentVtx[vtxId + 0].v.ob[1] - 0x10;

        this->windowContentVtx[vtxId + 1].v.tc[0] = this->windowContentVtx[vtxId + 3].v.tc[0] = 0x800;
        this->windowContentVtx[vtxId + 2].v.tc[1] = this->windowContentVtx[vtxId + 3].v.tc[1] = 0x200;

        this->windowContentVtx[vtxId + 4].v.ob[0] = this->windowContentVtx[vtxId + 6].v.ob[0] =
            this->windowContentVtx[vtxId + 0].v.ob[0] + 1;

        this->windowContentVtx[vtxId + 5].v.ob[0] = this->windowContentVtx[vtxId + 7].v.ob[0] =
            this->windowContentVtx[vtxId + 4].v.ob[0] + 0x40;

        this->windowContentVtx[vtxId + 4].v.ob[1] = this->windowContentVtx[vtxId + 5].v.ob[1] = tempPosY - 1;

        this->windowContentVtx[vtxId + 6].v.ob[1] = this->windowContentVtx[vtxId + 7].v.ob[1] =
            this->windowContentVtx[vtxId + 4].v.ob[1] - 0x10;

        this->windowContentVtx[vtxId + 5].v.tc[0] = this->windowContentVtx[vtxId + 7].v.tc[0] = 0x800;
        this->windowContentVtx[vtxId + 6].v.tc[1] = this->windowContentVtx[vtxId + 7].v.tc[1] = 0x200;

        vtxId += 8;

        /* Owl Save Icon */

        posX = this->windowPosX + 0xA3;

        if ((this->configMode == 0x10) && (j == this->copyDestFileIndex)) {
            tempPosY = this->fileNamesY[j] + 0x2C;
        }
        else if (((this->configMode == 0x11) || (this->configMode == 0x12)) && (j == this->copyDestFileIndex)) {
            tempPosY = this->buttonYOffsets[j] + posY;
        }
        else {
            tempPosY = posY + this->buttonYOffsets[j] + this->fileNamesY[j];
        }

        this->windowContentVtx[vtxId + 0].v.ob[0] = this->windowContentVtx[vtxId + 2].v.ob[0] = posX + 0xE;

        this->windowContentVtx[vtxId + 1].v.ob[0] = this->windowContentVtx[vtxId + 3].v.ob[0] =
            this->windowContentVtx[vtxId + 0].v.ob[0] + 0x18;

        this->windowContentVtx[vtxId + 0].v.ob[1] = this->windowContentVtx[vtxId + 1].v.ob[1] = tempPosY - 2;

        this->windowContentVtx[vtxId + 2].v.ob[1] = this->windowContentVtx[vtxId + 3].v.ob[1] =
            this->windowContentVtx[vtxId + 0].v.ob[1] - 0xC;

        this->windowContentVtx[vtxId + 1].v.tc[0] = this->windowContentVtx[vtxId + 3].v.tc[0] = 0x300;
        this->windowContentVtx[vtxId + 2].v.tc[1] = this->windowContentVtx[vtxId + 3].v.tc[1] = 0x180;

        vtxId += 4;

        /* Day Text */

        for (i = 0; i < 2; i++, vtxId += 4) {

            this->windowContentVtx[vtxId + 0].v.ob[0] = this->windowContentVtx[vtxId + 2].v.ob[0] = 2 + posX + i;

            this->windowContentVtx[vtxId + 1].v.ob[0] = this->windowContentVtx[vtxId + 3].v.ob[0] =
                this->windowContentVtx[vtxId + 0].v.ob[0] + 0x30;

            this->windowContentVtx[vtxId + 0].v.ob[1] = this->windowContentVtx[vtxId + 1].v.ob[1] = tempPosY - i - 0x12;

            this->windowContentVtx[vtxId + 2].v.ob[1] = this->windowContentVtx[vtxId + 3].v.ob[1] =
                this->windowContentVtx[vtxId + 0].v.ob[1] - 0x18;

            this->windowContentVtx[vtxId + 1].v.tc[0] = this->windowContentVtx[vtxId + 3].v.tc[0] = 0x600;

            this->windowContentVtx[vtxId + 2].v.tc[1] = this->windowContentVtx[vtxId + 3].v.tc[1] = 0x300;
        }

        /* Time Digits */

        posX += 6;
        index = vtxId;

        for (i = 0; i < 5; i++, vtxId += 4, posX += 8) {

            this->windowContentVtx[vtxId + 0].v.ob[0] = this->windowContentVtx[vtxId + 2].v.ob[0] = posX;

            this->windowContentVtx[vtxId + 1].v.ob[0] = this->windowContentVtx[vtxId + 3].v.ob[0] =
                this->windowContentVtx[vtxId + 0].v.ob[0] + 0xC;

            this->windowContentVtx[vtxId + 0].v.ob[1] = this->windowContentVtx[vtxId + 1].v.ob[1] = tempPosY - 0x2A;

            this->windowContentVtx[vtxId + 2].v.ob[1] = this->windowContentVtx[vtxId + 3].v.ob[1] =
                this->windowContentVtx[vtxId + 0].v.ob[1] - 0xC;

            this->windowContentVtx[vtxId + 0x14].v.ob[0] = this->windowContentVtx[vtxId + 0x16].v.ob[0] = posX + 1;

            this->windowContentVtx[vtxId + 0x15].v.ob[0] = this->windowContentVtx[vtxId + 0x17].v.ob[0] =
                this->windowContentVtx[vtxId + 0x14].v.ob[0] + 0xC;

            this->windowContentVtx[vtxId + 0x14].v.ob[1] = this->windowContentVtx[vtxId + 0x15].v.ob[1] =
                tempPosY - 0x2B;

            this->windowContentVtx[vtxId + 0x16].v.ob[1] = this->windowContentVtx[vtxId + 0x17].v.ob[1] =
                this->windowContentVtx[vtxId + 0x14].v.ob[1] - 0xC;
        }

        // Adjust the colon to the right
        this->windowContentVtx[index + 8].v.ob[0] = this->windowContentVtx[index + 10].v.ob[0] =
            this->windowContentVtx[index + 8].v.ob[0] + 3;

        this->windowContentVtx[index + 9].v.ob[0] = this->windowContentVtx[index + 11].v.ob[0] =
            this->windowContentVtx[index + 8].v.ob[0] + 0xC;

        this->windowContentVtx[index + 0x1C].v.ob[0] = this->windowContentVtx[index + 0x1E].v.ob[0] =
            this->windowContentVtx[index + 8].v.ob[0] + 1;

        this->windowContentVtx[index + 0x1D].v.ob[0] = this->windowContentVtx[index + 0x1F].v.ob[0] =
            this->windowContentVtx[index + 0x1C].v.ob[0] + 0xC;

        vtxId += 20;
    }

    posX = this->windowPosX - 6;
    posY = -0xC;

    // @recomp Check if the rewind button is visible based on whether there's an owl save for the current slot and what mode the file select is currently in.
    bool rewind_visible = this->menuMode == FS_MENU_MODE_SELECT && this->isOwlSave[this->buttonIndex + 2] && (this->selectMode == SM_FADE_IN_FILE_INFO || this->selectMode == SM_CONFIRM_FILE || this->selectMode == SM_FADE_OUT_FILE_INFO || this->selectMode == SM_FADE_OUT);
    for (j = 0; j < 2; j++, vtxId += 4, posY -= 0x10) {

        this->windowContentVtx[vtxId + 0].v.ob[0] = this->windowContentVtx[vtxId + 2].v.ob[0] = posX;

        this->windowContentVtx[vtxId + 1].v.ob[0] = this->windowContentVtx[vtxId + 3].v.ob[0] =
            this->windowContentVtx[vtxId + 0].v.ob[0] + 0x40;

        this->windowContentVtx[vtxId + 0].v.ob[1] = this->windowContentVtx[vtxId + 1].v.ob[1] =
            this->buttonYOffsets[j + 3] + posY;

        this->windowContentVtx[vtxId + 2].v.ob[1] = this->windowContentVtx[vtxId + 3].v.ob[1] =
            this->windowContentVtx[vtxId + 0].v.ob[1] - 0x10;

        this->windowContentVtx[vtxId + 1].v.tc[0] = this->windowContentVtx[vtxId + 3].v.tc[0] = 0x800;

        // @recomp Move the Yes and Quit buttons up by one if the Rewind button is visible.
        if (rewind_visible) {
            this->windowContentVtx[vtxId + 0].v.ob[1] += 16;
            this->windowContentVtx[vtxId + 1].v.ob[1] += 16;
            this->windowContentVtx[vtxId + 2].v.ob[1] += 16;
            this->windowContentVtx[vtxId + 3].v.ob[1] += 16;
        }
    }

    this->windowContentVtx[vtxId + 0].v.ob[0] = this->windowContentVtx[vtxId + 2].v.ob[0] = posX;

    this->windowContentVtx[vtxId + 1].v.ob[0] = this->windowContentVtx[vtxId + 3].v.ob[0] =
        this->windowContentVtx[vtxId + 0].v.ob[0] + 0x40;

    this->windowContentVtx[vtxId + 0].v.ob[1] = this->windowContentVtx[vtxId + 1].v.ob[1] =
        this->buttonYOffsets[5] - 0x34;

    this->windowContentVtx[vtxId + 2].v.ob[1] = this->windowContentVtx[vtxId + 3].v.ob[1] =
        this->windowContentVtx[vtxId + 0].v.ob[1] - 0x10;

    this->windowContentVtx[vtxId + 1].v.tc[0] = this->windowContentVtx[vtxId + 3].v.tc[0] = 0x800;

    vtxId += 4;

    if (((this->menuMode == FS_MENU_MODE_CONFIG) && (this->configMode >= 2)) ||
        ((this->menuMode == FS_MENU_MODE_SELECT) && (this->selectMode == 3))) {
        if (this->menuMode == FS_MENU_MODE_CONFIG) {
            if ((this->configMode == 4) || (this->configMode == 7) || (this->configMode == 0x16)) {
                j = D_80814644[this->buttonIndex];
            }
            else if ((this->configMode == 0x19) || (this->configMode == 0xC)) {
                j = D_8081464C[this->buttonIndex];
            }
            else {
                j = D_80814638[this->buttonIndex];
            }
        }
        else {
            j = D_80814650_patched[this->confirmButtonIndex];
        }

        this->windowContentVtx[vtxId + 0].v.ob[0] = this->windowContentVtx[vtxId + 2].v.ob[0] = this->windowPosX - 0xA;

        this->windowContentVtx[vtxId + 1].v.ob[0] = this->windowContentVtx[vtxId + 3].v.ob[0] =
            this->windowContentVtx[vtxId + 0].v.ob[0] + 0x48;

        this->windowContentVtx[vtxId + 0].v.ob[1] = this->windowContentVtx[vtxId + 1].v.ob[1] =
            this->windowContentVtx[j].v.ob[1] + 4;

        this->windowContentVtx[vtxId + 2].v.ob[1] = this->windowContentVtx[vtxId + 3].v.ob[1] =
            this->windowContentVtx[vtxId + 0].v.ob[1] - 0x18;

        this->windowContentVtx[vtxId + 1].v.tc[0] = this->windowContentVtx[vtxId + 3].v.tc[0] = 0x900;

        this->windowContentVtx[vtxId + 2].v.tc[1] = this->windowContentVtx[vtxId + 3].v.tc[1] = 0x300;
    }

    this->windowContentVtx[vtxId + 4].v.ob[0] = this->windowContentVtx[vtxId + 6].v.ob[0] = this->windowPosX + 0x3A;

    this->windowContentVtx[vtxId + 5].v.ob[0] = this->windowContentVtx[vtxId + 7].v.ob[0] =
        this->windowContentVtx[vtxId + 4].v.ob[0] + 0x80;

    this->windowContentVtx[vtxId + 4].v.ob[1] = this->windowContentVtx[vtxId + 5].v.ob[1] =
        this->windowContentVtx[D_80814638[this->warningButtonIndex]].v.ob[1];

    this->windowContentVtx[vtxId + 6].v.ob[1] = this->windowContentVtx[vtxId + 7].v.ob[1] =
        this->windowContentVtx[vtxId + 4].v.ob[1] - 0x10;

    this->windowContentVtx[vtxId + 5].v.tc[0] = this->windowContentVtx[vtxId + 7].v.tc[0] = 0x1000;

    // @recomp Copy the vertices for the Rewind button from the Yes button and move it down 2 buttons.
    if (rewind_visible) {
        for (u16 j = 0; j < 4; j++) {
            this->windowContentVtx[vtxId + 4 + j] = this->windowContentVtx[0x3AC + j];
            this->windowContentVtx[vtxId + 4 + j].v.ob[1] -= 32;
        }
    }
}

/**
 * Draw most window contents including buttons, labels, and icons.
 * Does not include anything from the keyboard and settings windows.
 */
RECOMP_PATCH void FileSelect_DrawWindowContents(GameState *thisx) {
    FileSelectState *this = (FileSelectState *)thisx;
    s16 fileIndex;
    s16 temp;
    s16 i;
    s16 quadVtxIndex;

    if (1) {}

    OPEN_DISPS(this->state.gfxCtx);

    // draw title label
    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetCombineLERP(POLY_OPA_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0, PRIMITIVE,
        ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, this->titleAlpha[FS_TITLE_CUR]);
    gDPSetEnvColor(POLY_OPA_DISP++, 0, 0, 0, 0);

    gSPVertex(POLY_OPA_DISP++, &this->windowContentVtx[0], 4, 0);
    gDPLoadTextureBlock(POLY_OPA_DISP++, sTitleLabels[this->titleLabel], G_IM_FMT_IA, G_IM_SIZ_8b, 128, 16, 0,
        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
        G_TX_NOLOD);
    gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);

    // draw next title label
    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, this->titleAlpha[FS_TITLE_NEXT]);
    gDPLoadTextureBlock(POLY_OPA_DISP++, sTitleLabels[this->nextTitleLabel], G_IM_FMT_IA, G_IM_SIZ_8b, 128, 16, 0,
        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
        G_TX_NOLOD);
    gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);

    temp = 4;

    gDPPipeSync(POLY_OPA_DISP++);

    // @recomp Check if the Rewind button is currently selected to know whether to display the regular save instead of the owl save.
    u8 hide_owl_save = (this->menuMode == FS_MENU_MODE_SELECT) && (this->confirmButtonIndex == FS_BTN_CONFIRM_REWIND);

    // draw file info box (large box when a file is selected)
    for (fileIndex = 0; fileIndex < 3; fileIndex++, temp += 28) {
        if (fileIndex < 2) {
            gDPPipeSync(POLY_OPA_DISP++);
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, this->windowColor[0], this->windowColor[1], this->windowColor[2],
                this->fileInfoAlpha[fileIndex]);
            gSPVertex(POLY_OPA_DISP++, &this->windowContentVtx[temp], 28, 0);

            for (quadVtxIndex = 0, i = 0; i < 7; i++, quadVtxIndex += 4) {
                // @recomp Don't draw the box on the right that displays owl save information if the Rewind button is selected.
                if ((i < 5) || (this->isOwlSave[fileIndex + 2] && (i >= 5) && !hide_owl_save)) {
                    gDPLoadTextureBlock(POLY_OPA_DISP++, sFileInfoBoxTextures[i], G_IM_FMT_IA, G_IM_SIZ_16b,
                        sFileInfoBoxPartWidths[i], 56, 0, G_TX_NOMIRROR | G_TX_WRAP,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                    gSP1Quadrangle(POLY_OPA_DISP++, quadVtxIndex, quadVtxIndex + 2, quadVtxIndex + 3, quadVtxIndex + 1,
                        0);
                }
            }
        }
    }

    gDPPipeSync(POLY_OPA_DISP++);

    for (i = 0; i < 3; i++, temp += 16) {
        if (i < 2) {
            // draw file button
            gSPVertex(POLY_OPA_DISP++, &this->windowContentVtx[temp], 16, 0);

            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, sWindowContentColors[0], sWindowContentColors[1],
                sWindowContentColors[2], this->fileButtonAlpha[i]);
            gDPLoadTextureBlock(POLY_OPA_DISP++, sFileButtonTextures[i], G_IM_FMT_IA, G_IM_SIZ_16b, 64, 16, 0,
                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK,
                G_TX_NOLOD, G_TX_NOLOD);
            gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);

            // draw file name box
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, sWindowContentColors[0], sWindowContentColors[1],
                sWindowContentColors[2], this->nameBoxAlpha[i]);
            gDPLoadTextureBlock(POLY_OPA_DISP++, gFileSelFileNameBoxTex, G_IM_FMT_IA, G_IM_SIZ_16b, 108, 16, 0,
                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK,
                G_TX_NOLOD, G_TX_NOLOD);
            gSP1Quadrangle(POLY_OPA_DISP++, 4, 6, 7, 5, 0);

            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, sWindowContentColors[0], sWindowContentColors[1],
                sWindowContentColors[2], this->connectorAlpha[i]);
            gDPLoadTextureBlock(POLY_OPA_DISP++, gFileSelConnectorTex, G_IM_FMT_IA, G_IM_SIZ_8b, 24, 16, 0,
                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK,
                G_TX_NOLOD, G_TX_NOLOD);
            gSP1Quadrangle(POLY_OPA_DISP++, 8, 10, 11, 9, 0);

            // @recomp Skip drawing the box to hold the owl save icon if the Rewind button is currently selected.
            if (this->isOwlSave[i + 2] && !hide_owl_save) {
                gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, sWindowContentColors[0], sWindowContentColors[1],
                    sWindowContentColors[2], this->nameBoxAlpha[i]);
                gDPLoadTextureBlock(POLY_OPA_DISP++, gFileSelBlankButtonTex, G_IM_FMT_IA, G_IM_SIZ_16b, 52, 16, 0,
                    G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK,
                    G_TX_NOLOD, G_TX_NOLOD);
                gSP1Quadrangle(POLY_OPA_DISP++, 12, 14, 15, 13, 0);
            }
        }
    }

    // draw file info
    for (fileIndex = 0; fileIndex < 2; fileIndex++) {
        // @recomp Record the save's owl save status and clear it if the rewind button is currently selected.
        u8 *this_owl_save = &this->isOwlSave[fileIndex + 2];
        u8 owl_save_old = *this_owl_save;
        if (hide_owl_save) {
            *this_owl_save = false;
        }

        FileSelect_DrawFileInfo(&this->state, fileIndex);

        // @recomp Reset the save's owl save status.
        *this_owl_save = owl_save_old;
    }

    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetCombineLERP(POLY_OPA_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0, PRIMITIVE,
        ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);
    gDPSetEnvColor(POLY_OPA_DISP++, 0, 0, 0, 0);
    // @recomp Load an extra 4 vertices for the rewind button.
    gSPVertex(POLY_OPA_DISP++, &this->windowContentVtx[0x3AC], 24, 0);

    // draw primary action buttons (copy/erase)
    for (quadVtxIndex = 0, i = 0; i < 2; i++, quadVtxIndex += 4) {
        gDPPipeSync(POLY_OPA_DISP++);
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, this->windowColor[0], this->windowColor[1], this->windowColor[2],
            this->actionButtonAlpha[i]);
        gDPLoadTextureBlock(POLY_OPA_DISP++, sActionButtonTextures[i], G_IM_FMT_IA, G_IM_SIZ_16b, 64, 16, 0,
            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
            G_TX_NOLOD);
        gSP1Quadrangle(POLY_OPA_DISP++, quadVtxIndex, quadVtxIndex + 2, quadVtxIndex + 3, quadVtxIndex + 1, 0);
    }

    gDPPipeSync(POLY_OPA_DISP++);

    // draw confirm buttons (yes/quit)
    for (quadVtxIndex = 0, i = FS_BTN_CONFIRM_YES; i <= FS_BTN_CONFIRM_QUIT; i++, quadVtxIndex += 4) {
        temp = this->confirmButtonTexIndices[i];
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, this->windowColor[0], this->windowColor[1], this->windowColor[2],
            this->confirmButtonAlpha[i]);
        gDPLoadTextureBlock(POLY_OPA_DISP++, sActionButtonTextures[temp], G_IM_FMT_IA, G_IM_SIZ_16b, 64, 16, 0,
            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
            G_TX_NOLOD);
        gSP1Quadrangle(POLY_OPA_DISP++, quadVtxIndex, quadVtxIndex + 2, quadVtxIndex + 3, quadVtxIndex + 1, 0);
    }

    // @recomp Draw the Rewind button.
    if (this->menuMode == FS_MENU_MODE_SELECT && this->isOwlSave[this->buttonIndex + 2]) {
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, this->windowColor[0], this->windowColor[1], this->windowColor[2],
            this->confirmButtonAlpha[FS_BTN_CONFIRM_YES]);
        gDPLoadTextureBlock(POLY_OPA_DISP++, rewind_button_texture, G_IM_FMT_IA, G_IM_SIZ_16b, 64, 16, 0,
            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
            G_TX_NOLOD);
        gSP1Quadrangle(POLY_OPA_DISP++, 16, 18, 19, 17, 0);
    }

    // draw options button
    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, this->windowColor[0], this->windowColor[1], this->windowColor[2],
        this->optionButtonAlpha);
    gDPLoadTextureBlock(POLY_OPA_DISP++, gFileSelOptionsButtonENGTex, G_IM_FMT_IA, G_IM_SIZ_16b, 64, 16, 0,
        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
        G_TX_NOLOD);
    gSP1Quadrangle(POLY_OPA_DISP++, 8, 10, 11, 9, 0);

    // draw highlight over currently selected button
    if (((this->menuMode == FS_MENU_MODE_CONFIG) &&
        ((this->configMode == CM_MAIN_MENU) || (this->configMode == CM_SELECT_COPY_SOURCE) ||
            (this->configMode == CM_SELECT_COPY_DEST) || (this->configMode == CM_COPY_CONFIRM) ||
            (this->configMode == CM_ERASE_SELECT) || (this->configMode == CM_ERASE_CONFIRM))) ||
        ((this->menuMode == FS_MENU_MODE_SELECT) && (this->selectMode == SM_CONFIRM_FILE))) {
        gDPPipeSync(POLY_OPA_DISP++);

        gDPSetCombineLERP(POLY_OPA_DISP++, 1, 0, PRIMITIVE, 0, TEXEL0, 0, PRIMITIVE, 0, 1, 0, PRIMITIVE, 0, TEXEL0, 0,
            PRIMITIVE, 0);
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, this->highlightColor[0], this->highlightColor[1],
            this->highlightColor[2], this->highlightColor[3]);
        gDPLoadTextureBlock(POLY_OPA_DISP++, gFileSelBigButtonHighlightTex, G_IM_FMT_I, G_IM_SIZ_8b, 72, 24, 0,
            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
            G_TX_NOLOD);
        gSP1Quadrangle(POLY_OPA_DISP++, 12, 14, 15, 13, 0);
    }

    // draw warning labels
    if (this->warningLabel > FS_WARNING_NONE) {
        gDPPipeSync(POLY_OPA_DISP++);

        gDPSetCombineLERP(POLY_OPA_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0,
            PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, this->emptyFileTextAlpha);
        gDPSetEnvColor(POLY_OPA_DISP++, 0, 0, 0, 0);
        gDPLoadTextureBlock(POLY_OPA_DISP++, sWarningLabels[this->warningLabel], G_IM_FMT_IA, G_IM_SIZ_8b, 128, 16, 0,
            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
            G_TX_NOLOD);
        gSP1Quadrangle(POLY_OPA_DISP++, 16, 18, 19, 17, 0);
    }

    gDPPipeSync(POLY_OPA_DISP++);

    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIDECALA, G_CC_MODULATEIDECALA);

    CLOSE_DISPS(this->state.gfxCtx);
}

RECOMP_PATCH void FileSelect_ConfirmFile(GameState *thisx) {
    FileSelectState *this = (FileSelectState *)thisx;
    Input *input = CONTROLLER1(&this->state);

    if (CHECK_BTN_ALL(input->press.button, BTN_START) || (CHECK_BTN_ALL(input->press.button, BTN_A))) {
        // @recomp Handle the Rewind button being pressed.
        if (this->confirmButtonIndex == FS_BTN_CONFIRM_YES || this->confirmButtonIndex == FS_BTN_CONFIRM_REWIND) {
            Rumble_Request(300.0f, 180, 20, 100);
            Audio_PlaySfx(NA_SE_SY_FSEL_DECIDE_L);
            this->selectMode = SM_FADE_OUT;
            Audio_MuteAllSeqExceptSystemAndOcarina(15);
        }
        else if (this->confirmButtonIndex == FS_BTN_CONFIRM_QUIT) {
            Audio_PlaySfx(NA_SE_SY_FSEL_CLOSE);
            this->selectMode++; // SM_FADE_OUT_FILE_INFO
        }
    }
    else if (CHECK_BTN_ALL(input->press.button, BTN_B)) {
        Audio_PlaySfx(NA_SE_SY_FSEL_CLOSE);
        this->selectMode++; // SM_FADE_OUT_FILE_INFO
    }
    else if (ABS_ALT(this->stickAdjY) >= 30) {
        Audio_PlaySfx(NA_SE_SY_FSEL_CURSOR);

        // @recomp Allow the cursor to navigate to the rewind button if this save slot has an owl save.
        if (this->isOwlSave[this->buttonIndex + 2]) {
            if (this->stickAdjY > 0) {
                this->confirmButtonIndex--;
                if (this->confirmButtonIndex < 0) {
                    this->confirmButtonIndex = FS_BTN_CONFIRM_REWIND;
                }
            }
            else {
                this->confirmButtonIndex++;
                if (this->confirmButtonIndex > FS_BTN_CONFIRM_REWIND) {
                    this->confirmButtonIndex = 0;
                }
            }
        }
        else {
            this->confirmButtonIndex ^= 1;
        }
    }
}

// Non-relocatable reference to the original address of Play_Init.
void Play_Init_NORELOCATE(GameState*);

/**
 * Load the save for the appropriate file and start the game.
 * Update function for `SM_LOAD_GAME`
 */
RECOMP_PATCH void FileSelect_LoadGame(GameState* thisx) {
    FileSelectState* this = (FileSelectState*)thisx;
    u16 i;

    gSaveContext.fileNum = this->buttonIndex;

    // @recomp Temporarily disable the owl save for this slot if the Rewind button was pressed.
    u8 was_owl_save = this->isOwlSave[gSaveContext.fileNum + 2];
    if (this->confirmButtonIndex == FS_BTN_CONFIRM_REWIND) {
        this->isOwlSave[gSaveContext.fileNum + 2] = false;
    }

    Sram_OpenSave(this, &this->sramCtx);

    // @recomp Re-enable the owl save for this slot after the file has been loaded.
    this->isOwlSave[gSaveContext.fileNum + 2] = was_owl_save;

    gSaveContext.gameMode = GAMEMODE_NORMAL;

    STOP_GAMESTATE(&this->state);
    SET_NEXT_GAMESTATE(&this->state, Play_Init_NORELOCATE, sizeof(PlayState));

    gSaveContext.respawnFlag = 0;
    gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance = ENTR_LOAD_OPENING;
    gSaveContext.seqId = (u8)NA_BGM_DISABLED;
    gSaveContext.ambienceId = AMBIENCE_ID_DISABLED;
    gSaveContext.showTitleCard = true;
    gSaveContext.dogParams = 0;

    for (i = 0; i < TIMER_ID_MAX; i++) {
        gSaveContext.timerStates[i] = TIMER_STATE_OFF;
    }

    gSaveContext.prevHudVisibility = HUD_VISIBILITY_ALL;
    gSaveContext.nayrusLoveTimer = 0;
    gSaveContext.healthAccumulator = 0;
    gSaveContext.magicFlag = 0;
    gSaveContext.forcedSeqId = 0;
    gSaveContext.skyboxTime = CLOCK_TIME(0, 0);
    gSaveContext.nextTransitionType = TRANS_NEXT_TYPE_DEFAULT;
    gSaveContext.cutsceneTrigger = 0;
    gSaveContext.chamberCutsceneNum = 0;
    gSaveContext.nextDayTime = NEXT_TIME_NONE;
    gSaveContext.retainWeatherMode = false;

    gSaveContext.buttonStatus[EQUIP_SLOT_B] = BTN_ENABLED;
    gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT] = BTN_ENABLED;
    gSaveContext.buttonStatus[EQUIP_SLOT_C_DOWN] = BTN_ENABLED;
    gSaveContext.buttonStatus[EQUIP_SLOT_C_RIGHT] = BTN_ENABLED;
    gSaveContext.buttonStatus[EQUIP_SLOT_A] = BTN_ENABLED;

    gSaveContext.hudVisibilityForceButtonAlphasByStatus = false;
    gSaveContext.nextHudVisibility = HUD_VISIBILITY_IDLE;
    gSaveContext.hudVisibility = HUD_VISIBILITY_IDLE;
    gSaveContext.hudVisibilityTimer = 0;

    gSaveContext.save.saveInfo.playerData.tatlTimer = 0;
}
