#include "play_patches.h"
#include "z64debug_display.h"
#include "input.h"
#include "prevent_bss_reordering.h"
#include "z64.h"
#include "regs.h"
#include "functions.h"
#include "z64vismono.h"
#include "z64visfbuf.h"
#include "buffers.h"

#include "variables.h"
#include "macros.h"
#include "buffers.h"
#include "idle.h"
#include "sys_cfb.h"
#include "z64bombers_notebook.h"
#include "z64quake.h"
#include "z64rumble.h"
#include "z64shrink_window.h"
#include "z64view.h"

#include "overlays/gamestates/ovl_daytelop/z_daytelop.h"
#include "overlays/gamestates/ovl_opening/z_opening.h"
#include "overlays/gamestates/ovl_file_choose/z_file_select.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"
#include "debug.h"

extern Input D_801F6C18;

RECOMP_DECLARE_EVENT(recomp_on_play_main(PlayState* play));
RECOMP_DECLARE_EVENT(recomp_on_play_update(PlayState* play));
RECOMP_DECLARE_EVENT(recomp_after_play_update(PlayState* play));

void controls_play_update(PlayState* play) {
    gSaveContext.options.zTargetSetting = recomp_get_targeting_mode();
}

// @recomp Patched to add hooks for various added functionality.
RECOMP_PATCH void Play_Main(GameState* thisx) {
    static Input* prevInput = NULL;
    PlayState* this = (PlayState*)thisx;

    // @recomp_event recomp_on_play_main(PlayState* play): Allow mods to execute code every frame.
    recomp_on_play_main(this);

    // @recomp
    debug_play_update(this);
    controls_play_update(this);
    analog_cam_pre_play_update(this);
    matrix_play_update(this);
    
    // @recomp avoid unused variable warning
    (void)prevInput;

    prevInput = CONTROLLER1(&this->state);
    DebugDisplay_Init();

    {
        GraphicsContext* gfxCtx = this->state.gfxCtx;

        if (1) {
            this->state.gfxCtx = NULL;
        }
        camera_pre_play_update(this);

        // @recomp_event recomp_on_play_update(PlayState* play): Play_Update is about to be called.
        recomp_on_play_update(this);

        Play_Update(this);

        // @recomp_event recomp_after_play_update(PlayState* play): Play_Update was called.
        recomp_after_play_update(this);

        camera_post_play_update(this);
        analog_cam_post_play_update(this);
        autosave_post_play_update(this);
        this->state.gfxCtx = gfxCtx;
    }

    {
        Input input = *CONTROLLER1(&this->state);

        if (1) {
            *CONTROLLER1(&this->state) = D_801F6C18;
        }
        Play_Draw(this);
        *CONTROLLER1(&this->state) = input;
    }

    CutsceneManager_Update();
    CutsceneManager_ClearWaiting();
}

// @recomp Patched to add load a hook for loading rooms.
RECOMP_PATCH s32 Room_HandleLoadCallbacks(PlayState* play, RoomContext* roomCtx) {
    if (roomCtx->status == 1) {
        if (osRecvMesg(&roomCtx->loadQueue, NULL, OS_MESG_NOBLOCK) == 0) {
            roomCtx->status = 0;
            roomCtx->curRoom.segment = roomCtx->activeRoomVram;
            gSegments[3] = OS_K0_TO_PHYSICAL(roomCtx->activeRoomVram);

            // @recomp Call the room load hook.
            room_load_hook(play, &roomCtx->curRoom);

            Scene_ExecuteCommands(play, roomCtx->curRoom.segment);
            func_80123140(play, GET_PLAYER(play));
            Actor_SpawnTransitionActors(play, &play->actorCtx);

            if (((play->sceneId != SCENE_IKANA) || (roomCtx->curRoom.num != 1)) && (play->sceneId != SCENE_IKNINSIDE)) {
                play->envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
                play->envCtx.lightBlendOverride = LIGHT_BLEND_OVERRIDE_NONE;
            }
            func_800FEAB0();
            if (Environment_GetStormState(play) == STORM_STATE_OFF) {
                Environment_StopStormNatureAmbience(play);
            }
        } else {
            return 0;
        }
    }

    return 1;
}

void ZeldaArena_Init(void* start, size_t size);

void Play_SpawnScene(PlayState* this, s32 sceneId, s32 spawn);
void Play_InitMotionBlur(void);

extern s16 sTransitionFillTimer;
extern Input D_801F6C18;
extern TransitionTile sTransitionTile;
extern s32 gTransitionTileState;
extern VisMono sPlayVisMono;
extern Color_RGBA8_u32 gVisMonoColor;
extern VisFbuf sPlayVisFbuf;
extern VisFbuf* sPlayVisFbufInstance;
extern BombersNotebook sBombersNotebook;
extern u8 sBombersNotebookOpen;
extern u8 sMotionBlurStatus;

extern s32 gDbgCamEnabled;
extern u8 D_801D0D54;

extern u8 gPictoPhotoI8[];
extern u8 D_80784600[];

// Non-relocatable references to the original addresses of these game state functions.
void DayTelop_Init_NORELOCATE(GameState*);
void TitleSetup_Init_NORELOCATE(GameState*);

RECOMP_DECLARE_EVENT(recomp_on_play_init(PlayState* this));
RECOMP_DECLARE_EVENT(recomp_after_play_init(PlayState* this));

bool allow_no_ocarina_tf = false;

// @recomp_export void recomp_set_allow_no_ocarina_tf(bool new_val): Set whether to force Termina Field to load normally even if Link has no ocarina.
RECOMP_EXPORT void recomp_set_allow_no_ocarina_tf(bool new_val) {
    allow_no_ocarina_tf = new_val;
}

RECOMP_PATCH void Play_Init(GameState* thisx) {
    PlayState* this = (PlayState*)thisx;
    GraphicsContext* gfxCtx = this->state.gfxCtx;
    s32 pad;
    uintptr_t zAlloc;
    s32 zAllocSize;
    Player* player;
    s32 i;
    s32 spawn;
    u8 sceneLayer;
    s32 scene;

    // @recomp_event recomp_on_play_init(PlayState* this): A new PlayState is being initialized.
    recomp_on_play_init(this);

    if ((gSaveContext.respawnFlag == -4) || (gSaveContext.respawnFlag == -0x63)) {
        if (CHECK_EVENTINF(EVENTINF_TRIGGER_DAYTELOP)) {
            CLEAR_EVENTINF(EVENTINF_TRIGGER_DAYTELOP);
            STOP_GAMESTATE(&this->state);
            // @recomp Use non-relocatable reference to DayTelop_Init instead.
            SET_NEXT_GAMESTATE(&this->state, DayTelop_Init_NORELOCATE, sizeof(DayTelopState));
            return;
        }

        gSaveContext.unk_3CA7 = 1;
        if (gSaveContext.respawnFlag == -0x63) {
            gSaveContext.respawnFlag = 2;
        }
    } else {
        gSaveContext.unk_3CA7 = 0;
    }

    if (gSaveContext.save.entrance == -1) {
        gSaveContext.save.entrance = 0;
        STOP_GAMESTATE(&this->state);
        // @recomp Use non-relocatable reference to TitleSetup_Init instead.
        SET_NEXT_GAMESTATE(&this->state, TitleSetup_Init_NORELOCATE, sizeof(TitleSetupState));
        return;
    }

    if ((gSaveContext.nextCutsceneIndex == 0xFFEF) || (gSaveContext.nextCutsceneIndex == 0xFFF0)) {
        scene = ((void)0, gSaveContext.save.entrance) >> 9;
        spawn = (((void)0, gSaveContext.save.entrance) >> 4) & 0x1F;

        if (CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE)) {
            if (scene == ENTR_SCENE_MOUNTAIN_VILLAGE_WINTER) {
                scene = ENTR_SCENE_MOUNTAIN_VILLAGE_SPRING;
            } else if (scene == ENTR_SCENE_GORON_VILLAGE_WINTER) {
                scene = ENTR_SCENE_GORON_VILLAGE_SPRING;
            } else if (scene == ENTR_SCENE_PATH_TO_GORON_VILLAGE_WINTER) {
                scene = ENTR_SCENE_PATH_TO_GORON_VILLAGE_SPRING;
            } else if ((scene == ENTR_SCENE_SNOWHEAD) || (scene == ENTR_SCENE_PATH_TO_SNOWHEAD) ||
                       (scene == ENTR_SCENE_PATH_TO_MOUNTAIN_VILLAGE) || (scene == ENTR_SCENE_GORON_SHRINE) ||
                       (scene == ENTR_SCENE_GORON_RACETRACK)) {
                gSaveContext.nextCutsceneIndex = 0xFFF0;
            }
        }

        if (CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE)) {
            if (scene == ENTR_SCENE_SOUTHERN_SWAMP_POISONED) {
                scene = ENTR_SCENE_SOUTHERN_SWAMP_CLEARED;
            } else if (scene == ENTR_SCENE_WOODFALL) {
                gSaveContext.nextCutsceneIndex = 0xFFF1;
            }
        }

        if (CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE) && (scene == ENTR_SCENE_IKANA_CANYON)) {
            gSaveContext.nextCutsceneIndex = 0xFFF2;
        }

        if (CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE) &&
            ((scene == ENTR_SCENE_GREAT_BAY_COAST) || (scene == ENTR_SCENE_ZORA_CAPE))) {
            gSaveContext.nextCutsceneIndex = 0xFFF0;
        }

        // "First cycle" Termina Field
        // @recomp_use_export_var allow_no_ocarina_tf: Skip loading into "First cycle" Termina Field if mods enable it.
        if (!allow_no_ocarina_tf && INV_CONTENT(ITEM_OCARINA_OF_TIME) != ITEM_OCARINA_OF_TIME) {
            if ((scene == ENTR_SCENE_TERMINA_FIELD) &&
                (((void)0, gSaveContext.save.entrance) != ENTRANCE(TERMINA_FIELD, 10))) {
                gSaveContext.nextCutsceneIndex = 0xFFF4;
            }
        }
        //! FAKE:
        gSaveContext.save.entrance =
            Entrance_Create(((void)0, scene), spawn, ((void)0, gSaveContext.save.entrance) & 0xF);
    }

    GameState_Realloc(&this->state, 0);
    KaleidoManager_Init(this);
    ShrinkWindow_Init();
    View_Init(&this->view, gfxCtx);
    Audio_SetExtraFilter(0);
    Quake_Init();
    Distortion_Init(this);

    for (i = 0; i < ARRAY_COUNT(this->cameraPtrs); i++) {
        this->cameraPtrs[i] = NULL;
    }

    Camera_Init(&this->mainCamera, &this->view, &this->colCtx, this);
    Camera_ChangeStatus(&this->mainCamera, CAM_STATUS_ACTIVE);

    for (i = 0; i < ARRAY_COUNT(this->subCameras); i++) {
        Camera_Init(&this->subCameras[i], &this->view, &this->colCtx, this);
        Camera_ChangeStatus(&this->subCameras[i], CAM_STATUS_INACTIVE);
    }

    this->cameraPtrs[CAM_ID_MAIN] = &this->mainCamera;
    this->cameraPtrs[CAM_ID_MAIN]->uid = CAM_ID_MAIN;
    this->activeCamId = CAM_ID_MAIN;

    Camera_OverwriteStateFlags(&this->mainCamera, CAM_STATE_0 | CAM_STATE_CHECK_WATER | CAM_STATE_2 | CAM_STATE_3 |
                                                      CAM_STATE_4 | CAM_STATE_DISABLE_MODE_CHANGE | CAM_STATE_6);
    Sram_Alloc(&this->state, &this->sramCtx);
    Regs_InitData(this);
    Message_Init(this);
    GameOver_Init(this);
    SoundSource_InitAll(this);
    EffFootmark_Init(this);
    Effect_Init(this);
    EffectSS_Init(this, 100);
    CollisionCheck_InitContext(this, &this->colChkCtx);
    AnimationContext_Reset(&this->animationCtx);
    Cutscene_InitContext(this, &this->csCtx);

    if (gSaveContext.nextCutsceneIndex != 0xFFEF) {
        gSaveContext.save.cutsceneIndex = gSaveContext.nextCutsceneIndex;
        gSaveContext.nextCutsceneIndex = 0xFFEF;
    }

    if (gSaveContext.save.cutsceneIndex == 0xFFFD) {
        gSaveContext.save.cutsceneIndex = 0;
    }

    if (gSaveContext.nextDayTime != NEXT_TIME_NONE) {
        gSaveContext.save.time = gSaveContext.nextDayTime;
        gSaveContext.skyboxTime = gSaveContext.nextDayTime;
    }

    if ((CURRENT_TIME >= CLOCK_TIME(18, 0)) || (CURRENT_TIME < CLOCK_TIME(6, 30))) {
        gSaveContext.save.isNight = true;
    } else {
        gSaveContext.save.isNight = false;
    }

    func_800EDDB0(this);

    if (((gSaveContext.gameMode != GAMEMODE_NORMAL) && (gSaveContext.gameMode != GAMEMODE_TITLE_SCREEN)) ||
        (gSaveContext.save.cutsceneIndex >= 0xFFF0)) {
        gSaveContext.nayrusLoveTimer = 0;
        Magic_Reset(this);
        gSaveContext.sceneLayer = (gSaveContext.save.cutsceneIndex & 0xF) + 1;

        // Set saved cutscene to 0 so it doesn't immediately play, but instead let the `CutsceneManager` handle it.
        gSaveContext.save.cutsceneIndex = 0;
    } else {
        gSaveContext.sceneLayer = 0;
    }

    sceneLayer = gSaveContext.sceneLayer;

    Play_SpawnScene(
        this, Entrance_GetSceneIdAbsolute(((void)0, gSaveContext.save.entrance) + ((void)0, gSaveContext.sceneLayer)),
        Entrance_GetSpawnNum(((void)0, gSaveContext.save.entrance) + ((void)0, gSaveContext.sceneLayer)));
    KaleidoScopeCall_Init(this);
    Interface_Init(this);

    if (gSaveContext.nextDayTime != NEXT_TIME_NONE) {
        if (gSaveContext.nextDayTime == NEXT_TIME_DAY) {
            gSaveContext.save.day++;
            gSaveContext.save.eventDayCount++;
            gSaveContext.dogIsLost = true;
            gSaveContext.nextDayTime = NEXT_TIME_DAY_SET;
        } else {
            gSaveContext.nextDayTime = NEXT_TIME_NIGHT_SET;
        }
    }

    Play_InitMotionBlur();

    R_PAUSE_BG_PRERENDER_STATE = PAUSE_BG_PRERENDER_OFF;
    R_PICTO_PHOTO_STATE = PICTO_PHOTO_STATE_OFF;

    PreRender_Init(&this->pauseBgPreRender);
    PreRender_SetValuesSave(&this->pauseBgPreRender, gCfbWidth, gCfbHeight, NULL, NULL, NULL);
    PreRender_SetValues(&this->pauseBgPreRender, gCfbWidth, gCfbHeight, NULL, NULL);

    this->unk_18E64 = gWorkBuffer;
    this->pictoPhotoI8 = gPictoPhotoI8;
    this->unk_18E68 = D_80784600;
    this->unk_18E58 = D_80784600;
    this->unk_18E60 = D_80784600;
    gTransitionTileState = TRANS_TILE_OFF;
    this->transitionMode = TRANS_MODE_OFF;
    D_801D0D54 = false;

    FrameAdvance_Init(&this->frameAdvCtx);
    Rand_Seed(osGetTime());
    Matrix_Init(&this->state);

    this->state.main = Play_Main;
    this->state.destroy = Play_Destroy;

    this->transitionTrigger = TRANS_TRIGGER_END;
    this->worldCoverAlpha = 0;
    this->bgCoverAlpha = 0;
    this->haltAllActors = false;
    this->unk_18844 = false;

    if (gSaveContext.gameMode != GAMEMODE_TITLE_SCREEN) {
        if (gSaveContext.nextTransitionType == TRANS_NEXT_TYPE_DEFAULT) {
            this->transitionType =
                (Entrance_GetTransitionFlags(((void)0, gSaveContext.save.entrance) + sceneLayer) >> 7) & 0x7F;
        } else {
            this->transitionType = gSaveContext.nextTransitionType;
            gSaveContext.nextTransitionType = TRANS_NEXT_TYPE_DEFAULT;
        }
    } else {
        this->transitionType = TRANS_TYPE_FADE_BLACK;
    }

    TransitionFade_Init(&this->unk_18E48);
    TransitionFade_SetType(&this->unk_18E48, 3);
    TransitionFade_SetColor(&this->unk_18E48, RGBA8(160, 160, 160, 255));
    TransitionFade_Start(&this->unk_18E48);
    VisMono_Init(&sPlayVisMono);

    gVisMonoColor.a = 0;
    sPlayVisFbufInstance = &sPlayVisFbuf;
    VisFbuf_Init(sPlayVisFbufInstance);
    sPlayVisFbufInstance->lodProportion = 0.0f;
    sPlayVisFbufInstance->mode = VIS_FBUF_MODE_GENERAL;
    sPlayVisFbufInstance->primColor.r = 0;
    sPlayVisFbufInstance->primColor.g = 0;
    sPlayVisFbufInstance->primColor.b = 0;
    sPlayVisFbufInstance->primColor.a = 0;
    sPlayVisFbufInstance->envColor.r = 0;
    sPlayVisFbufInstance->envColor.g = 0;
    sPlayVisFbufInstance->envColor.b = 0;
    sPlayVisFbufInstance->envColor.a = 0;
    CutsceneFlags_UnsetAll(this);
    THA_GetRemaining(&this->state.tha);
    zAllocSize = THA_GetRemaining(&this->state.tha);
    zAlloc = (uintptr_t)THA_AllocTailAlign16(&this->state.tha, zAllocSize);

    //! @bug: Incorrect ALIGN16s
    ZeldaArena_Init((void*)((zAlloc + 8) & ~0xF), (zAllocSize - ((zAlloc + 8) & ~0xF)) + zAlloc);

    Actor_InitContext(this, &this->actorCtx, this->linkActorEntry);

    while (!Room_HandleLoadCallbacks(this, &this->roomCtx)) {}

    if ((CURRENT_DAY != 0) && ((this->roomCtx.curRoom.behaviorType1 == ROOM_BEHAVIOR_TYPE1_1) ||
                               (this->roomCtx.curRoom.behaviorType1 == ROOM_BEHAVIOR_TYPE1_5))) {
        Actor_Spawn(&this->actorCtx, this, ACTOR_EN_TEST4, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0);
    }

    player = GET_PLAYER(this);

    Camera_InitFocalActorSettings(&this->mainCamera, &player->actor);
    gDbgCamEnabled = false;

    if (PLAYER_GET_BG_CAM_INDEX(&player->actor) != 0xFF) {
        Camera_ChangeActorCsCamIndex(&this->mainCamera, PLAYER_GET_BG_CAM_INDEX(&player->actor));
    }

    CutsceneManager_StoreCamera(&this->mainCamera);
    Interface_SetSceneRestrictions(this);
    Environment_PlaySceneSequence(this);
    gSaveContext.seqId = this->sequenceCtx.seqId;
    gSaveContext.ambienceId = this->sequenceCtx.ambienceId;
    AnimationContext_Update(this, &this->animationCtx);
    Cutscene_HandleEntranceTriggers(this);
    gSaveContext.respawnFlag = 0;
    sBombersNotebookOpen = false;
    BombersNotebook_Init(&sBombersNotebook);

    // @recomp_event recomp_after_play_init(PlayState* this): The new PlayState has finished initializing.
    recomp_after_play_init(this);
}
