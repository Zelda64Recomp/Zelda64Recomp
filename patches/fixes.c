#include "patches.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"
#include "overlays/actors/ovl_En_Fall/z_en_fall.h"
#include "overlays/actors/ovl_Demo_Effect/z_demo_effect.h"
#include "overlays/gamestates/ovl_daytelop/z_daytelop.h"
#include "z64shrink_window.h"

#define PAGE_BG_WIDTH (PAGE_BG_COLS * PAGE_BG_QUAD_WIDTH)
#define PAGE_BG_HEIGHT (PAGE_BG_ROWS * PAGE_BG_QUAD_HEIGHT)

#define RECOMP_PAGE_ROW_HEIGHT 15
#define RECOMP_PAGE_ROW_COUNT ((PAGE_BG_HEIGHT + RECOMP_PAGE_ROW_HEIGHT - 1) / RECOMP_PAGE_ROW_HEIGHT)

extern s16* sVtxPageQuadsX[VTX_PAGE_MAX];
extern s16* sVtxPageQuadsWidth[VTX_PAGE_MAX];
extern s16* sVtxPageQuadsY[VTX_PAGE_MAX];
extern s16* sVtxPageQuadsHeight[VTX_PAGE_MAX];

s16 sVtxPageGameOverSaveQuadsY[VTX_PAGE_SAVE_QUADS] = {
    14,  // promptPageVtx[60] QUAD_PROMPT_MESSAGE
    -2,  // promptPageVtx[64] QUAD_PROMPT_CURSOR_LEFT
    -2,  // promptPageVtx[68] QUAD_PROMPT_CURSOR_RIGHT
    -18, // promptPageVtx[72] QUAD_PROMPT_CHOICE_YES
    -18, // promptPageVtx[76] QUAD_PROMPT_CHOICE_NO
};

// @recomp patched to draw as strips with bilerp compensation instead of tiles.
RECOMP_PATCH s16 KaleidoScope_SetPageVertices(PlayState* play, Vtx* vtx, s16 vtxPage, s16 numQuads) {
    PauseContext* pauseCtx = &play->pauseCtx;
    GameOverContext* gameOverCtx = &play->gameOverCtx;
    s16* quadsX;
    s16* quadsWidth;
    s16* quadsY;
    s16* quadsHeight;
    s32 cur_y;
    u32 row;

    cur_y = (PAGE_BG_HEIGHT + 2) / 2;

    // 2 verts per row plus 2 extra verts at the start and the end.
    for (row = 0; row < RECOMP_PAGE_ROW_COUNT + 2; row++) {
        s32 next_y = MAX(cur_y - RECOMP_PAGE_ROW_HEIGHT, -(PAGE_BG_HEIGHT + 2) / 2);

        vtx[4 * row + 0].v.ob[0] = -PAGE_BG_WIDTH / 2;
        vtx[4 * row + 1].v.ob[0] =  PAGE_BG_WIDTH / 2;
        vtx[4 * row + 2].v.ob[0] = -PAGE_BG_WIDTH / 2;
        vtx[4 * row + 3].v.ob[0] =  PAGE_BG_WIDTH / 2;
        
        vtx[4 * row + 0].v.ob[1] = cur_y + pauseCtx->offsetY;
        vtx[4 * row + 1].v.ob[1] = cur_y + pauseCtx->offsetY;
        vtx[4 * row + 2].v.ob[1] = next_y + pauseCtx->offsetY;
        vtx[4 * row + 3].v.ob[1] = next_y + pauseCtx->offsetY;

        vtx[4 * row + 0].v.ob[2] = vtx[4 * row + 1].v.ob[2] = vtx[4 * row + 2].v.ob[2] = vtx[4 * row + 3].v.ob[2] = 0;

        vtx[4 * row + 0].v.flag = vtx[4 * row + 1].v.flag = vtx[4 * row + 2].v.flag = vtx[4 * row + 3].v.flag = 0;

        #define PIXEL_OFFSET 0

        vtx[4 * row + 0].v.tc[0] = PIXEL_OFFSET;
        vtx[4 * row + 0].v.tc[1] = PIXEL_OFFSET;
        vtx[4 * row + 1].v.tc[0] = (PAGE_BG_WIDTH + 2 - 1) * (1 << 5) + PIXEL_OFFSET;
        vtx[4 * row + 1].v.tc[1] = PIXEL_OFFSET;
        vtx[4 * row + 2].v.tc[0] = PIXEL_OFFSET;
        vtx[4 * row + 2].v.tc[1] = (cur_y - next_y + 1 - 1) * (1 << 5) + PIXEL_OFFSET;
        vtx[4 * row + 3].v.tc[0] = (PAGE_BG_WIDTH + 2 - 1) * (1 << 5) + PIXEL_OFFSET;
        vtx[4 * row + 3].v.tc[1] = (cur_y - next_y + 1 - 1) * (1 << 5) + PIXEL_OFFSET;

        vtx[4 * row + 0].v.cn[0] = vtx[4 * row + 1].v.cn[0] = vtx[4 * row + 2].v.cn[0] = vtx[4 * row + 3].v.cn[0] = 0;
        vtx[4 * row + 0].v.cn[1] = vtx[4 * row + 1].v.cn[1] = vtx[4 * row + 2].v.cn[1] = vtx[4 * row + 3].v.cn[1] = 0;
        vtx[4 * row + 0].v.cn[2] = vtx[4 * row + 1].v.cn[2] = vtx[4 * row + 2].v.cn[2] = vtx[4 * row + 3].v.cn[2] = 0;
        vtx[4 * row + 0].v.cn[3] = vtx[4 * row + 1].v.cn[3] = vtx[4 * row + 2].v.cn[3] = vtx[4 * row + 3].v.cn[3] = pauseCtx->alpha;

        cur_y = next_y;
    }
        
    s16 k = 60;

    if (numQuads != 0) {
        quadsX = sVtxPageQuadsX[vtxPage];
        quadsWidth = sVtxPageQuadsWidth[vtxPage];
        quadsY = sVtxPageQuadsY[vtxPage];
        quadsHeight = sVtxPageQuadsHeight[vtxPage];
        s16 i;

        for (i = 0; i < numQuads; i++, k += 4) {
            vtx[k + 2].v.ob[0] = vtx[k + 0].v.ob[0] = quadsX[i];

            vtx[k + 1].v.ob[0] = vtx[k + 3].v.ob[0] = vtx[k + 0].v.ob[0] + quadsWidth[i];

            if (!IS_PAUSE_STATE_GAMEOVER) {
                vtx[k + 0].v.ob[1] = vtx[k + 1].v.ob[1] = quadsY[i] + pauseCtx->offsetY;
            } else if (gameOverCtx->state == GAMEOVER_INACTIVE) {
                vtx[k + 0].v.ob[1] = vtx[k + 1].v.ob[1] = quadsY[i] + pauseCtx->offsetY;
            } else {
                vtx[k + 0].v.ob[1] = vtx[k + 1].v.ob[1] = sVtxPageGameOverSaveQuadsY[i] + pauseCtx->offsetY;
            }

            vtx[k + 2].v.ob[1] = vtx[k + 3].v.ob[1] = vtx[k + 0].v.ob[1] - quadsHeight[i];

            vtx[k + 0].v.ob[2] = vtx[k + 1].v.ob[2] = vtx[k + 2].v.ob[2] = vtx[k + 3].v.ob[2] = 0;

            vtx[k + 0].v.flag = vtx[k + 1].v.flag = vtx[k + 2].v.flag = vtx[k + 3].v.flag = 0;

            vtx[k + 0].v.tc[0] = vtx[k + 0].v.tc[1] = vtx[k + 1].v.tc[1] = vtx[k + 2].v.tc[0] = 0;
            vtx[k + 1].v.tc[0] = vtx[k + 3].v.tc[0] = quadsWidth[i] << 5;
            vtx[k + 2].v.tc[1] = vtx[k + 3].v.tc[1] = quadsHeight[i] << 5;

            vtx[k + 0].v.cn[0] = vtx[k + 2].v.cn[0] = vtx[k + 0].v.cn[1] = vtx[k + 2].v.cn[1] = vtx[k + 0].v.cn[2] =
                vtx[k + 2].v.cn[2] = 255;

            vtx[k + 1].v.cn[0] = vtx[k + 3].v.cn[0] = vtx[k + 1].v.cn[1] = vtx[k + 3].v.cn[1] = vtx[k + 1].v.cn[2] =
                vtx[k + 3].v.cn[2] = 255;

            vtx[k + 0].v.cn[3] = vtx[k + 2].v.cn[3] = vtx[k + 1].v.cn[3] = vtx[k + 3].v.cn[3] = pauseCtx->alpha;
        }
    }
    return k;
}

// There's one extra row and column of padding on each side, so the size is +2 in each dimension.
typedef u8 bg_image_t[(2 + PAGE_BG_WIDTH) * (2 + PAGE_BG_HEIGHT)];

#define BG_IMAGE_COUNT 4
TexturePtr* bg_pointers[BG_IMAGE_COUNT];
bg_image_t bg_images[BG_IMAGE_COUNT] __attribute__((aligned(8)));

void assemble_image(TexturePtr* textures, bg_image_t* image_out) {
    u8* pixels_out_start = *image_out;
    // Skip a row, it'll be filled in later.
    u8* pixels_out = pixels_out_start + PAGE_BG_WIDTH + 2;
    for (u32 row = 0; row < PAGE_BG_ROWS; row++) {
        u8* texture0 = Lib_SegmentedToVirtual(textures[row + 0]);
        u8* texture1 = Lib_SegmentedToVirtual(textures[row + 5]);
        u8* texture2 = Lib_SegmentedToVirtual(textures[row + 10]);
        for (u32 tile_row = 0; tile_row < PAGE_BG_QUAD_HEIGHT; tile_row++) {
            // Write the first column, setting alpha to 0.
            *pixels_out = (*texture0) & 0xF0;
            pixels_out++;
            // Copy a row from each of the tiles into the output texture.
            Lib_MemCpy(pixels_out, texture0, PAGE_BG_QUAD_WIDTH * sizeof(u8));
            pixels_out += PAGE_BG_QUAD_WIDTH;
            texture0 += PAGE_BG_QUAD_WIDTH;
            Lib_MemCpy(pixels_out, texture1, PAGE_BG_QUAD_WIDTH * sizeof(u8));
            pixels_out += PAGE_BG_QUAD_WIDTH;
            texture1 += PAGE_BG_QUAD_WIDTH;
            Lib_MemCpy(pixels_out, texture2, PAGE_BG_QUAD_WIDTH * sizeof(u8));
            pixels_out += PAGE_BG_QUAD_WIDTH;
            texture2 += PAGE_BG_QUAD_WIDTH;
            // Write the last column, setting alpha to 0.
            *pixels_out = (*(texture2 - 1)) & 0xF0;
            pixels_out++;
        }
    }
    // Fill in the padding rows with duplicates of the first and last row but with zero alpha.
    for (u32 col = 0; col < PAGE_BG_WIDTH + 2; col++) {
        pixels_out_start[col] = pixels_out_start[col + PAGE_BG_WIDTH + 2] & 0xF0;
        pixels_out[col] = pixels_out[col - PAGE_BG_WIDTH - 2] & 0xF0;
    }
}

static bool assembled_kaleido_images = false;

extern TexturePtr sMaskPageBgTextures[];
extern TexturePtr sItemPageBgTextures[];
extern TexturePtr sMapPageBgTextures[];
extern TexturePtr sQuestPageBgTextures[];

extern void (*sKaleidoScopeUpdateFunc)(PlayState* play);
extern void (*sKaleidoScopeDrawFunc)(PlayState* play);

extern void KaleidoScope_Update(PlayState* play);
extern void KaleidoScope_Draw(PlayState* play);

void KaleidoUpdateWrapper(PlayState* play) {
    KaleidoScope_Update(play);
}

void KaleidoDrawWrapper(PlayState* play) {
    // @recomp Update the background image pointers to reflect the overlay's load address.
    bg_pointers[0] = sMaskPageBgTextures;
    bg_pointers[1] = sItemPageBgTextures;
    bg_pointers[2] = sMapPageBgTextures;
    bg_pointers[3] = sQuestPageBgTextures;

    KaleidoScope_Draw(play);

    // @recomp Check if this is the first time kaleido has been drawn. If so, assemble the background textures
    // into the full seamless image.
    if (!assembled_kaleido_images) {
        assembled_kaleido_images = true;
        // Record the old value for segments 0x08 and 0x0D, then update them with the correct values so that segmented addresses
        // can be converted in assemble_image.
        uintptr_t old_segment_08 = gSegments[0x08];
        uintptr_t old_segment_0D = gSegments[0x0D];
        gSegments[0x08] = OS_K0_TO_PHYSICAL(play->pauseCtx.iconItemSegment);
        gSegments[0x0D] = OS_K0_TO_PHYSICAL(play->pauseCtx.iconItemLangSegment);
        assemble_image(sMaskPageBgTextures, &bg_images[0]);
        assemble_image(sItemPageBgTextures, &bg_images[1]);
        assemble_image(sMapPageBgTextures, &bg_images[2]);
        assemble_image(sQuestPageBgTextures, &bg_images[3]);
        gSegments[0x08] = old_segment_08;
        gSegments[0x0D] = old_segment_0D;
    }
}

RECOMP_PATCH void KaleidoScopeCall_Init(PlayState* play) {
    // @recomp Set the update and draw func pointers to the wrappers instead of the actual functions.
    sKaleidoScopeUpdateFunc = KaleidoUpdateWrapper;
    sKaleidoScopeDrawFunc = KaleidoDrawWrapper;
    KaleidoSetup_Init(play);
}

// @recomp patched to fix bilerp seams.
RECOMP_PATCH Gfx* KaleidoScope_DrawPageSections(Gfx* gfx, Vtx* vertices, TexturePtr* textures) {
    s32 i;
    s32 j;

    bg_image_t* cur_image = NULL;

    // Check if this texture set has been assembled into a full image.
    u32 image_index;
    for (image_index = 0; image_index < BG_IMAGE_COUNT; image_index++) {
        if (bg_pointers[image_index] == textures) {
            cur_image = &bg_images[image_index];
            break;
        }
    }

    if (cur_image == NULL) {
        // No image was found.
        return gfx;
    }

    // Draw the rows.
    for (u32 bg_row = 0; bg_row < RECOMP_PAGE_ROW_COUNT; bg_row++) {
        u32 cur_row_height = MIN(RECOMP_PAGE_ROW_HEIGHT, PAGE_BG_HEIGHT + 1 - bg_row * RECOMP_PAGE_ROW_HEIGHT);
        gDPLoadTextureTile(gfx++, *cur_image,
            G_IM_FMT_IA, G_IM_SIZ_8b, // fmt, siz
            PAGE_BG_WIDTH + 2, PAGE_BG_HEIGHT + 2, // width, height
            0, bg_row * RECOMP_PAGE_ROW_HEIGHT, // uls, ult
            PAGE_BG_WIDTH + 2 - 1, bg_row * RECOMP_PAGE_ROW_HEIGHT + cur_row_height + 1 - 1, // lrs, lrt
            0, // pal
            G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP,
            G_TX_NOMASK, G_TX_NOMASK,
            G_TX_NOLOD, G_TX_NOLOD);
        gDPSetTileSize(gfx++, G_TX_RENDERTILE,
                0 << G_TEXTURE_IMAGE_FRAC,
                0 << G_TEXTURE_IMAGE_FRAC,
                (PAGE_BG_WIDTH + 2 - 1) <<G_TEXTURE_IMAGE_FRAC,
                (cur_row_height + 1 - 1) << G_TEXTURE_IMAGE_FRAC);
        gSPVertex(gfx++, vertices + 4 * bg_row, 4, 0);
        gSP2Triangles(gfx++, 0, 3, 1, 0x0, 3, 0, 2, 0x0);
    }

    return gfx;
}

typedef void (*CutsceneHandler)(PlayState* play, CutsceneContext* csCtx);
extern CutsceneHandler sScriptedCutsceneHandlers[];
void Cutscene_SetupScripted(PlayState* play, CutsceneContext* csCtx);

int extra_vis = 0;

// @recomp Patch the giants cutscene to make certain frames take longer to mimic performance on console.
// This prevents the music from desyncing from the cutscene as it was designed around the console's frame times.
RECOMP_PATCH void Cutscene_UpdateScripted(PlayState* play, CutsceneContext* csCtx) {
    if ((gSaveContext.cutsceneTrigger != 0) && (play->transitionTrigger == TRANS_TRIGGER_START)) {
        gSaveContext.cutsceneTrigger = 0;
    }

    if ((gSaveContext.cutsceneTrigger != 0) && (csCtx->state == CS_STATE_IDLE)) {
        gSaveContext.save.cutsceneIndex = 0xFFFD;
        gSaveContext.cutsceneTrigger = 1;
    }

    if (gSaveContext.save.cutsceneIndex >= 0xFFF0) {
        Cutscene_SetupScripted(play, csCtx);
        sScriptedCutsceneHandlers[csCtx->state](play, csCtx);
    }

    // @recomp Insert extra VI interrupt delays on certain frames of the giants cutscene to match console
    // framerates. This prevents the music from desyncing with the cutscene.
    if (play->sceneId == SCENE_00KEIKOKU && gSaveContext.sceneLayer == 1) {
        s32 curFrame = play->csCtx.curFrame;
        s32 scriptIndex = play->csCtx.scriptIndex;
        // These regions of lag were determined by measuring framerate on console, though they pretty clearly
        // correspond to specific camera angles and effects active during the cutscene.
        if (
            (scriptIndex == 0 && curFrame >= 1123 && curFrame <= 1381) ||
            (scriptIndex != 0 && curFrame >= 4 && curFrame <= 85) ||
            (scriptIndex != 0 && curFrame >= 431 && curFrame <= 490)
        ) {
            extra_vis = 1;
        }
    }
}

// @recomp Fix a texture scroll using an incorrect tile size, which resulted in the scroll jumping during the animation.
RECOMP_PATCH s32 DemoEffect_OverrideLimbDrawTimewarp(PlayState* play, SkelCurve* skelCurve, s32 limbIndex, Actor* thisx) {
    s32 pad;
    DemoEffect* this = (DemoEffect*)thisx;
    u32 frames = play->gameplayFrames;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, 170, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, this->envXluColor[0], this->envXluColor[1], this->envXluColor[2], 255);

    // @recomp Fix the tile size to be 64x64 for both tiles to match the actual texture size.
    gSPSegment(POLY_XLU_DISP++, 0x08,
            Gfx_TwoTexScroll(play->state.gfxCtx,
                0, (frames * 6) % 256, 255 - ((frames * 16) % 256), 0x40, 0x40,
                1, (frames * 4) % 256, 255 - ((frames * 12) % 256), 0x40, 0x40));

    CLOSE_DISPS(play->state.gfxCtx);

    if (limbIndex == 0) {
        s16* transform = skelCurve->jointTable[0];

        transform[2] = transform[0] = 1024;
        transform[1] = 1024;
    }

    return true;
}

void DayTelop_Main(GameState* thisx);
void DayTelop_Destroy(GameState* thisx);
void DayTelop_Noop(DayTelopState* this);
void DayTelop_LoadGraphics(DayTelopState* this);

// @recomp Increase the length of the "Dawn of the X Day" screen to account for faster loading.
RECOMP_PATCH void DayTelop_Init(GameState* thisx) {
    DayTelopState* this = (DayTelopState*)thisx;

    GameState_SetFramerateDivisor(&this->state, 1);
    Matrix_Init(&this->state);
    ShrinkWindow_Destroy();
    View_Init(&this->view, this->state.gfxCtx);
    this->state.main = DayTelop_Main;
    this->state.destroy = DayTelop_Destroy;
    // @recomp Add 120 extra frames (2 seconds with a frame divisor of 1) to account for faster loading.
    this->transitionCountdown = 260;
    this->fadeInState = DAYTELOP_HOURSTEXT_OFF;

    if (gSaveContext.save.day < 9) {
        if (gSaveContext.save.day == 0) {
            Sram_ClearFlagsAtDawnOfTheFirstDay();
        }
        Sram_IncrementDay();
    }

    DayTelop_Noop(this);
    DayTelop_LoadGraphics(this);
    Audio_PlaySfx(NA_SE_OC_TELOP_IMPACT);
}

extern PlayerAnimationHeader* D_8085D17C[PLAYER_FORM_MAX];
void Player_TalkWithPlayer(PlayState* play, Actor* actor);
void Player_Action_88(Player* this, PlayState* play);
void Player_SetAction_PreserveItemAction(PlayState* play, Player* this, PlayerActionFunc actionFunc, s32 arg3);
void Player_AnimationPlayOnceReverse(PlayState* play, Player* this, PlayerAnimationHeader* anim);
s32 Player_ActionChange_13(Player* this, PlayState* play);
s32 func_8085B28C(PlayState* play, Player* this, PlayerCsAction csAction);
void func_808525C4(PlayState* play, Player* this);
void func_8085255C(PlayState* play, Player* this);
void func_80836A5C(Player* this, PlayState* play);
s32 func_8082DA90(PlayState* play);

// @recomp Patched to fix the issue where ocarina inputs are discarded for the first 3 frames (150ms).
RECOMP_PATCH void Player_Action_63(Player* this, PlayState* play) {
    if ((this->unk_AA5 != PLAYER_UNKAA5_4) && ((PlayerAnimation_Update(play, &this->skelAnime) &&
                                                (this->skelAnime.animation == D_8085D17C[this->transformation])) ||
                                               ((this->skelAnime.mode == 0) && (this->av2.actionVar2 == 0)))) {
        func_808525C4(play, this);
        // @recomp Fix the bug where ocarina inputs are discarded for 3 frames by only running this on the first frame of this state. 
        if (this->av2.actionVar2 == 1) {
            if (!(this->actor.flags & ACTOR_FLAG_20000000) || (this->unk_A90->id == ACTOR_EN_ZOT)) {
                Message_DisplayOcarinaStaff(play, OCARINA_ACTION_FREE_PLAY);
            }
        }
    } else if (this->av2.actionVar2 != 0) {
        if (play->msgCtx.ocarinaMode == OCARINA_MODE_END) {
            play->interfaceCtx.unk_222 = 0;
            CutsceneManager_Stop(play->playerCsIds[PLAYER_CS_ID_ITEM_OCARINA]);
            this->actor.flags &= ~ACTOR_FLAG_20000000;

            if ((this->talkActor != NULL) && (this->talkActor == this->unk_A90) && (this->unk_A94 >= 0.0f)) {
                Player_TalkWithPlayer(play, this->talkActor);
            } else if (this->tatlTextId < 0) {
                this->talkActor = this->tatlActor;
                this->tatlActor->textId = -this->tatlTextId;
                Player_TalkWithPlayer(play, this->talkActor);
            } else if (!Player_ActionChange_13(this, play)) {
                func_80836A5C(this, play);
                Player_AnimationPlayOnceReverse(play, this, D_8085D17C[this->transformation]);
            }
        } else {
            s32 var_v1 = (play->msgCtx.ocarinaMode >= OCARINA_MODE_WARP_TO_GREAT_BAY_COAST) &&
                         (play->msgCtx.ocarinaMode <= OCARINA_MODE_WARP_TO_ENTRANCE);
            s32 pad[2];

            if (var_v1 || (play->msgCtx.ocarinaMode == OCARINA_MODE_APPLY_SOT) ||
                (play->msgCtx.ocarinaMode == OCARINA_MODE_APPLY_DOUBLE_SOT) ||
                (play->msgCtx.ocarinaMode == OCARINA_MODE_APPLY_INV_SOT_FAST) ||
                (play->msgCtx.ocarinaMode == OCARINA_MODE_APPLY_INV_SOT_SLOW)) {
                if (play->msgCtx.ocarinaMode == OCARINA_MODE_APPLY_SOT) {
                    if (!func_8082DA90(play)) {
                        if (gSaveContext.save.saveInfo.playerData.threeDayResetCount == 1) {
                            play->nextEntrance = ENTRANCE(CUTSCENE, 1);
                        } else {
                            play->nextEntrance = ENTRANCE(CUTSCENE, 0);
                        }

                        gSaveContext.nextCutsceneIndex = 0xFFF7;
                        play->transitionTrigger = TRANS_TRIGGER_START;
                    }
                } else {
                    Actor* actor;

                    play->interfaceCtx.unk_222 = 0;
                    CutsceneManager_Stop(play->playerCsIds[PLAYER_CS_ID_ITEM_OCARINA]);
                    this->actor.flags &= ~ACTOR_FLAG_20000000;

                    actor = Actor_Spawn(&play->actorCtx, play, var_v1 ? ACTOR_EN_TEST7 : ACTOR_EN_TEST6,
                                        this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z, 0, 0,
                                        0, play->msgCtx.ocarinaMode);
                    if (actor != NULL) {
                        this->stateFlags1 &= ~PLAYER_STATE1_20000000;
                        this->csAction = PLAYER_CSACTION_NONE;
                        func_8085B28C(play, NULL, PLAYER_CSACTION_19);
                        this->stateFlags1 |= PLAYER_STATE1_10000000 | PLAYER_STATE1_20000000;
                    } else {
                        func_80836A5C(this, play);
                        Player_AnimationPlayOnceReverse(play, this, D_8085D17C[this->transformation]);
                    }
                }
            } else if ((play->msgCtx.ocarinaMode == OCARINA_MODE_EVENT) &&
                       (play->msgCtx.lastPlayedSong == OCARINA_SONG_ELEGY)) {
                play->interfaceCtx.unk_222 = 0;
                CutsceneManager_Stop(play->playerCsIds[PLAYER_CS_ID_ITEM_OCARINA]);

                this->actor.flags &= ~ACTOR_FLAG_20000000;
                Player_SetAction_PreserveItemAction(play, this, Player_Action_88, 0);
                this->stateFlags1 |= PLAYER_STATE1_10000000 | PLAYER_STATE1_20000000;
            } else if (this->unk_AA5 == PLAYER_UNKAA5_4) {
                f32 temp_fa0 = this->skelAnime.jointTable[PLAYER_LIMB_ROOT - 1].x;
                f32 temp_fa1 = this->skelAnime.jointTable[PLAYER_LIMB_ROOT - 1].z;
                f32 var_fv1;

                var_fv1 = sqrtf(SQ(temp_fa0) + SQ(temp_fa1));
                if (var_fv1 != 0.0f) {
                    var_fv1 = (var_fv1 - 100.0f) / var_fv1;
                    var_fv1 = CLAMP_MIN(var_fv1, 0.0f);
                }

                this->skelAnime.jointTable[PLAYER_LIMB_ROOT - 1].x = temp_fa0 * var_fv1;
                this->skelAnime.jointTable[PLAYER_LIMB_ROOT - 1].z = temp_fa1 * var_fv1;
            } else {
                func_8085255C(play, this);
            }
        }
    }
}
