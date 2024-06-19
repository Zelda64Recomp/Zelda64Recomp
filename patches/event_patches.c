#include "patches.h"

u8 func_800FE5D0(struct PlayState* play);

void Environment_UpdateTimeBasedSequence(PlayState* play) {
    s32 pad;

    //! FAKE:
    if (gSaveContext.sceneLayer) {}

    if ((play->csCtx.state == CS_STATE_IDLE) && !(play->actorCtx.flags & ACTORCTX_FLAG_TELESCOPE_ON)) {
        switch (play->envCtx.timeSeqState) {
            case TIMESEQ_DAY_BGM:
                break;

            case TIMESEQ_FADE_DAY_BGM:
                if (CURRENT_TIME > CLOCK_TIME(17, 10)) {
                    SEQCMD_STOP_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 240);
                    play->envCtx.timeSeqState++;
                }
                break;

            case TIMESEQ_NIGHT_BEGIN_SFX:
                if (CURRENT_TIME >= CLOCK_TIME(18, 0)) {
                    play->envCtx.timeSeqState++;
                }
                break;

            case TIMESEQ_EARLY_NIGHT_CRITTERS:
                if (play->envCtx.precipitation[PRECIP_RAIN_CUR] < 9) {
                    Audio_PlayAmbience(play->sequenceCtx.ambienceId);
                    Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_CRITTER_0, 1, 1);
                }
                play->envCtx.timeSeqState++;
                break;

            case TIMESEQ_NIGHT_DELAY:
                if (CURRENT_TIME >= CLOCK_TIME(19, 0)) {
                    play->envCtx.timeSeqState++;
                }
                break;

            case TIMESEQ_NIGHT_CRITTERS:
                Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_CRITTER_0, 1, 0);
                Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_CRITTER_1 << 4 | AMBIENCE_CHANNEL_CRITTER_3, 1, 1);
                play->envCtx.timeSeqState++;
                break;

            case TIMESEQ_DAY_BEGIN_SFX:
                if ((CURRENT_TIME < CLOCK_TIME(19, 0)) && (CURRENT_TIME >= CLOCK_TIME(5, 0))) {
                    play->envCtx.timeSeqState++;
                }
                break;

            case TIMESEQ_MORNING_CRITTERS:
                Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_CRITTER_1 << 4 | AMBIENCE_CHANNEL_CRITTER_3, 1, 0);
                Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_CRITTER_4 << 4 | AMBIENCE_CHANNEL_CRITTER_5, 1, 1);
                play->envCtx.timeSeqState++;
                break;

            case TIMESEQ_DAY_DELAY:
                break;

            default:
                break;
        }
    }

    // @recomp Don't play final hours until it's actually past midnight
    if ((play->envCtx.timeSeqState != TIMESEQ_REQUEST) && (((void)0, gSaveContext.save.day) == 3) &&
        (CURRENT_TIME < CLOCK_TIME(6, 0)) && !func_800FE5D0(play) && (play->transitionTrigger == TRANS_TRIGGER_OFF) &&
        (play->transitionMode == TRANS_MODE_OFF) && (play->csCtx.state == CS_STATE_IDLE) &&
        ((play->sceneId != SCENE_00KEIKOKU) || (((void)0, gSaveContext.sceneLayer) != 1)) &&
        (CutsceneManager_GetCurrentCsId() == CS_ID_NONE) &&
        (AudioSeq_GetActiveSeqId(SEQ_PLAYER_BGM_MAIN) != NA_BGM_FINAL_HOURS) &&
        (AudioSeq_GetActiveSeqId(SEQ_PLAYER_BGM_MAIN) != NA_BGM_SONG_OF_SOARING) &&
        (CURRENT_TIME > CLOCK_TIME(0, 0))) {
        SEQCMD_STOP_SEQUENCE(SEQ_PLAYER_AMBIENCE, 0);
        Audio_PlaySceneSequence(NA_BGM_FINAL_HOURS, 3 - 1);
    }
}

#include "overlays/actors/ovl_En_Test6/z_en_test6.h"
#include "z64quake.h"

void func_800B7298(PlayState* play, Actor* csActor, u8 csAction); // Player_SetCsActionWithHaltedActors

void EnTest6_SharedSoTCutscene(EnTest6* this, PlayState* play);

extern CutsceneData sDoubleSoTCsCamData[];
static Vec3f sSubCamUp;

struct SoTCsAmmoDrops;

typedef void (*SoTCsAmmoDropDrawFunc)(EnTest6*, PlayState*, struct SoTCsAmmoDrops*);

typedef enum SoTCsAmmoDropType {
    /* 0 */ SOTCS_AMMO_DROP_NONE,
    /* 1 */ SOTCS_AMMO_DROP_ARROWS,
    /* 2 */ SOTCS_AMMO_DROP_BOMB,
    /* 3 */ SOTCS_AMMO_DROP_DEKU_NUT,
    /* 4 */ SOTCS_AMMO_DROP_DEKU_STICK,
    /* 5 */ SOTCS_AMMO_DROP_RUPEE_GREEN,
    /* 6 */ SOTCS_AMMO_DROP_RUPEE_BLUE
} SoTCsAmmoDropType;

typedef struct SoTCsAmmoDrops {
    /* 0x00 */ SoTCsAmmoDropType type;
    /* 0x04 */ f32 scale;
    /* 0x08 */ Vec3f pos;
    /* 0x14 */ SoTCsAmmoDropDrawFunc draw;
} SoTCsAmmoDrops; // size = 0x18

extern SoTCsAmmoDrops sSoTCsAmmoDrops[12];

typedef enum SoTCsDrawType {
    /*  0 */ SOTCS_DRAW_DOUBLE_SOT,
    /*  1 */ SOTCS_DRAW_RESET_CYCLE_SOT,
    /*  2 */ SOTCS_DRAW_INVERTED_SOT,
    /* 99 */ SOTCS_DRAW_TYPE_NONE = 99
} SoTCsDrawType;

void EnTest6_EnableMotionBlur(s16 alpha);
void EnTest6_DisableMotionBlur(void);

void EnTest6_EnableWhiteFillScreen(PlayState* play, f32 alphaRatio);
void EnTest6_DisableWhiteFillScreen(PlayState* play);
void EnTest6_StopDoubleSoTCutscene(EnTest6* this, PlayState* play);

extern Color_RGB8 sDoubleSoTCsFogColor;
extern Color_RGB8 sDoubleSoTCsAmbientColor;
extern Color_RGB8 sDoubleSoTCsDiffuseColor;
extern s16 sDoubleSoTCsFogNear;
extern s16 sDoubleSoTCsZFar;

void EnTest6_DoubleSoTCutscene(EnTest6* this, PlayState* play) {
    Input* input = CONTROLLER1(&play->state);
    Player* player = GET_PLAYER(play);
    Camera* subCam;
    s32 pad;
    s16 subCamId;
    s16 pad2;

    if (this->timer > 115) {
        this->doubleSoTEnvLerp += 0.2f;
        EnTest6_EnableWhiteFillScreen(play, this->doubleSoTEnvLerp);
    } else if (this->timer > 90) {
        this->doubleSoTEnvLerp -= 0.05f;
        EnTest6_EnableWhiteFillScreen(play, this->doubleSoTEnvLerp);
    } else if (this->timer == 90) {
        this->doubleSoTEnvLerp = 0.0f;
        EnTest6_DisableWhiteFillScreen(play);
    }

    if (this->timer == 1) {
        this->doubleSoTEnvLerp = 0.0f;
        EnTest6_DisableWhiteFillScreen(play);
    } else if (this->timer < 17) {
        this->doubleSoTEnvLerp -= 0.06666666f;
        EnTest6_EnableWhiteFillScreen(play, this->doubleSoTEnvLerp);
    } else if (this->timer < 22) {
        this->doubleSoTEnvLerp += 0.2f;
        EnTest6_EnableWhiteFillScreen(play, this->doubleSoTEnvLerp);
    }

    // @recomp Manual relocation, TODO remove when the recompiler handles this automatically.
    s16* sDoubleSoTCsFogNear_ptr = actor_relocate(&this->actor, &sDoubleSoTCsFogNear);
    s16* sDoubleSoTCsZFar_ptr = actor_relocate(&this->actor, &sDoubleSoTCsZFar);

    Color_RGB8* sDoubleSoTCsFogColor_ptr = actor_relocate(&this->actor, &sDoubleSoTCsFogColor);
    Color_RGB8* sDoubleSoTCsAmbientColor_ptr = actor_relocate(&this->actor, &sDoubleSoTCsAmbientColor);
    Color_RGB8* sDoubleSoTCsDiffuseColor_ptr = actor_relocate(&this->actor, &sDoubleSoTCsDiffuseColor);

    if (this->timer == 115) {
        Environment_LerpAmbientColor(play, sDoubleSoTCsAmbientColor_ptr, 1.0f);
        Environment_LerpDiffuseColor(play, sDoubleSoTCsDiffuseColor_ptr, 1.0f);
        Environment_LerpFogColor(play, sDoubleSoTCsFogColor_ptr, 1.0f);
        Environment_LerpFog(play, *sDoubleSoTCsFogNear_ptr, *sDoubleSoTCsZFar_ptr, 1.0f);
        play->unk_18844 = true;
    }

    if (this->timer == 15) {
        Environment_LerpAmbientColor(play, sDoubleSoTCsAmbientColor_ptr, 0.0f);
        Environment_LerpDiffuseColor(play, sDoubleSoTCsDiffuseColor_ptr, 0.0f);
        Environment_LerpFogColor(play, sDoubleSoTCsFogColor_ptr, 0.0f);
        Environment_LerpFog(play, *sDoubleSoTCsFogNear_ptr, *sDoubleSoTCsZFar_ptr, 0.0f);
        play->unk_18844 = false;
    }

    if (this->screenFillAlpha >= 20) {
        Environment_LerpAmbientColor(play, sDoubleSoTCsAmbientColor_ptr, this->doubleSoTEnvLerp);
        Environment_LerpDiffuseColor(play, sDoubleSoTCsDiffuseColor_ptr, this->doubleSoTEnvLerp);
        Environment_LerpFogColor(play, sDoubleSoTCsFogColor_ptr, this->doubleSoTEnvLerp);
        Environment_LerpFog(play, *sDoubleSoTCsFogNear_ptr, *sDoubleSoTCsZFar_ptr, this->doubleSoTEnvLerp);
        play->unk_18844 = false;
    }

    Actor_PlaySfx_FlaggedCentered1(&player->actor, NA_SE_PL_FLYING_AIR - SFX_FLAG);

    switch (this->timer) {
        case 119:
            EnTest6_EnableMotionBlur(50);
            break;

        case 115:
            EnTest6_EnableMotionBlur(20);
            Distortion_Request(DISTORTION_TYPE_SONG_OF_TIME);
            Distortion_SetDuration(90);
            this->cueId = SOTCS_CUEID_DOUBLE_INIT;
            break;

        case 110:
            Audio_PlayFanfare(NA_BGM_SONG_OF_DOUBLE_TIME);
            break;

        case 38:
        case 114:
            this->cueId = SOTCS_CUEID_DOUBLE_WAIT;
            break;

        case 76:
            this->cueId = SOTCS_CUEID_DOUBLE_CLOCKS_INWARD;
            break;

        case 61:
            EnTest6_EnableMotionBlur(150);
            this->cueId = SOTCS_CUEID_DOUBLE_CLOCKS_SPIN;
            break;

        case 51:
            EnTest6_EnableMotionBlur(180);
            this->cueId = SOTCS_CUEID_DOUBLE_CLOCKS_OUTWARD;
            break;

        case 14:
        case 15:
            EnTest6_EnableMotionBlur(50);
            Distortion_RemoveRequest(DISTORTION_TYPE_SONG_OF_TIME);
            this->cueId = SOTCS_CUEID_NONE;
            break;

        case 1:
            EnTest6_DisableMotionBlur();
            if (CHECK_EVENTINF(EVENTINF_HAS_DAYTIME_TRANSITION_CS)) {
                this->cueId = SOTCS_CUEID_DOUBLE_END;
            }
            break;

        default:
            break;
    }

    EnTest6_SharedSoTCutscene(this, play);

    if (this->timer == 115) {
        subCamId = CutsceneManager_GetCurrentSubCamId(play->playerCsIds[PLAYER_CS_ID_SONG_WARP]);
        subCam = Play_GetCamera(play, subCamId);

        this->subCamAt = subCam->at;
        this->subCamEye = subCam->eye;
        this->subCamFov = subCam->fov;
        CutsceneCamera_Init(subCam, &this->csCamInfo);
    }

    if ((this->timer <= 115) && (this->timer >= 16)) {
        // @recomp Manual relocation, TODO remove when the recompiler handles this automatically.
        CutsceneData* sDoubleSoTCsCamData_reloc = actor_relocate(&this->actor, sDoubleSoTCsCamData);
        CutsceneCamera_UpdateSplines((u8*)sDoubleSoTCsCamData_reloc, &this->csCamInfo);
    } else if (this->timer < 16) {
        subCamId = CutsceneManager_GetCurrentSubCamId(play->playerCsIds[PLAYER_CS_ID_SONG_WARP]);

        // @recomp Manual relocation, TODO remove when the recompiler handles this automatically.
        Vec3f* sSubCamUp_ptr = KaleidoManager_GetRamAddr(&sSubCamUp);
        Play_SetCameraAtEyeUp(play, subCamId, &this->subCamAt, &this->subCamEye, sSubCamUp_ptr);
        Play_SetCameraFov(play, subCamId, this->subCamFov);
        Play_SetCameraRoll(play, subCamId, 0);
    }

    switch (this->timer) {
        case 116:
            player->actor.freezeTimer = 2;
            player->actor.shape.rot.x = 0;
            player->actor.shape.rot.y = 0;
            player->actor.world.pos.x = 0.0f;
            player->actor.world.pos.y = 0.0f;
            player->actor.world.pos.z = 0.0f;
            player->actor.home.pos.x = 0.0f;
            player->actor.home.pos.y = 0.0f;
            player->actor.home.pos.z = 0.0f;
            break;

        case 98:
            func_800B7298(play, NULL, PLAYER_CSACTION_64);
            break;

        case 68:
            func_800B7298(play, NULL, PLAYER_CSACTION_65);
            break;

        case 52:
            func_800B7298(play, NULL, PLAYER_CSACTION_88);
            break;

        case 43:
            func_800B7298(play, NULL, PLAYER_CSACTION_114);
            break;

        case 38:
            func_800B7298(play, NULL, PLAYER_CSACTION_WAIT);
            break;

        case 14:
            player->actor.freezeTimer = 5;
            player->actor.world.pos = player->actor.home.pos = this->actor.home.pos;
            player->actor.shape.rot = this->actor.home.rot;
            player->actor.focus.rot.y = player->actor.shape.rot.y;
            player->currentYaw = player->actor.shape.rot.y;
            player->unk_ABC = 0.0f;
            player->unk_AC0 = 0.0f;
            player->actor.shape.yOffset = 0.0f;
            break;

        default:
            break;
    }

    if ((this->screenFillAlpha > 0) && (this->screenFillAlpha < 20)) {
        EnTest6_EnableWhiteFillScreen(play, this->screenFillAlpha * 0.05f);
        this->screenFillAlpha++;
        if (this->screenFillAlpha >= 20) {
            this->timer = 15;
            this->doubleSoTEnvLerp = 0.9333333f;
        }
    } else if ((this->timer < 96) && (this->timer > 50) &&
               (CHECK_BTN_ALL(input->press.button, BTN_A) || CHECK_BTN_ALL(input->press.button, BTN_B))) {
        this->screenFillAlpha = 1;
        this->timer = 39;
        SEQCMD_STOP_SEQUENCE(SEQ_PLAYER_FANFARE, 20);
    }

    // @recomp Replace DSoT functionality if the option for it is enabled.
    if (dsot_enabled() && this->timer == 15) {
        dsot_advance_hour(play);
    }

    if (DECR(this->timer) == 0) {
        EnTest6_StopDoubleSoTCutscene(this, play);
    }
}

void Play_InitScene(PlayState* this, s32 spawn);

/**
 * Processes two different cutscenes:
 * return to "Dawn of the First Day" Cs, and Song of Double Time Cs
 */
void EnTest6_SharedSoTCutscene(EnTest6* this, PlayState* play) {
    s32 pad[2];
    Player* player = GET_PLAYER(play);
    f32 yDiff;
    s32 i;
    s32 cueChannel;

    if (Cutscene_IsCueInChannel(play, CS_CMD_ACTOR_CUE_SOTCS)) {
        cueChannel = Cutscene_GetCueChannel(play, CS_CMD_ACTOR_CUE_SOTCS);
        this->cueId = play->csCtx.actorCues[cueChannel]->id;

        switch (this->cueId) {
            case SOTCS_CUEID_DOUBLE_WAIT:
                break;

            case SOTCS_CUEID_DOUBLE_INIT:
                this->drawType = SOTCS_DRAW_DOUBLE_SOT;
                this->counter = 0;
                this->clockAngle = 0;
                player->actor.shape.shadowDraw = NULL;

                if (play->csCtx.actorCues[cueChannel]->startPos.x != 0) {
                    this->clockSpeed = (u32)play->csCtx.actorCues[cueChannel]->startPos.x;
                } else {
                    this->clockSpeed = 150.0f;
                }

                if (play->csCtx.actorCues[cueChannel]->startPos.y != 0) {
                    this->clockColorGray = play->csCtx.actorCues[cueChannel]->startPos.y;
                } else {
                    this->clockColorGray = 38;
                }

                if (play->csCtx.actorCues[cueChannel]->startPos.z != 0) {
                    this->clockDist = (u32)play->csCtx.actorCues[cueChannel]->startPos.z;
                } else {
                    this->clockDist = 480.0f;
                }
                break;

            case SOTCS_CUEID_DOUBLE_CLOCKS_INWARD:
                if (play->csCtx.actorCues[cueChannel]->startPos.x != 0) {
                    this->clockSpeed += (u32)play->csCtx.actorCues[cueChannel]->startPos.x;
                }

                if (play->csCtx.actorCues[cueChannel]->startPos.y != 0) {
                    this->clockColorGray += (s16)play->csCtx.actorCues[cueChannel]->startPos.y;

                } else {
                    this->clockColorGray += 6;
                }

                if (play->csCtx.actorCues[cueChannel]->startPos.z != 0) {
                    this->clockRadialSpeed = (u32)play->csCtx.actorCues[cueChannel]->startPos.z;
                } else {
                    this->clockRadialSpeed = -32.0f;
                }
                this->clockDist += this->clockRadialSpeed;
                break;

            case SOTCS_CUEID_DOUBLE_CLOCKS_SPIN:
                if (play->csCtx.actorCues[cueChannel]->startPos.x != 0) {
                    this->clockSpeed += (u32)play->csCtx.actorCues[cueChannel]->startPos.x;
                }

                if (play->csCtx.actorCues[cueChannel]->startPos.y != 0) {
                    this->clockColorGray += (s16)play->csCtx.actorCues[cueChannel]->startPos.y;
                } else {
                    this->clockColorGray -= 4;
                }
                break;

            case SOTCS_CUEID_DOUBLE_CLOCKS_OUTWARD:
                if (play->csCtx.actorCues[cueChannel]->startPos.x != 0) {
                    this->clockSpeed += (u32)play->csCtx.actorCues[cueChannel]->startPos.x;
                }

                if (play->csCtx.actorCues[cueChannel]->startPos.y != 0) {
                    this->clockColorGray += (s16)play->csCtx.actorCues[cueChannel]->startPos.y;
                } else {
                    this->clockColorGray -= 8;
                }

                if (play->csCtx.actorCues[cueChannel]->startPos.z != 0) {
                    this->clockRadialSpeed += (u32)play->csCtx.actorCues[cueChannel]->startPos.z;
                } else {
                    this->clockRadialSpeed += 20.0f;
                }

                this->clockDist += this->clockRadialSpeed;
                if (this->clockDist > 3500.0f) {
                    this->clockDist = 3500.0f;
                    this->cueId = SOTCS_CUEID_NONE;
                }
                break;

            case SOTCS_CUEID_RESET_INIT:
                this->drawType = SOTCS_DRAW_RESET_CYCLE_SOT;
                this->counter = 0;
                this->clockAngle = 0;
                player->actor.shape.shadowDraw = NULL;

                if (play->csCtx.actorCues[cueChannel]->startPos.x != 0) {
                    this->clockSpeed = (u32)play->csCtx.actorCues[cueChannel]->startPos.x;
                } else {
                    this->clockSpeed = 100.0f;
                }

                if (play->csCtx.actorCues[cueChannel]->startPos.y != 0) {
                    this->speed = (u32)play->csCtx.actorCues[cueChannel]->startPos.y;
                } else {
                    this->speed = 20.0f;
                }

                if (play->csCtx.actorCues[cueChannel]->startPos.z != 0) {
                    this->clockDist = (u32)play->csCtx.actorCues[cueChannel]->startPos.z;
                } else {
                    this->clockDist = 300.0f;
                }
                this->clockAccel = 0.0f;
                break;

            case SOTCS_CUEID_RESET_CLOCKS_SLOW_DOWN:
                if (play->csCtx.actorCues[cueChannel]->startPos.x != 0) {
                    this->clockAccel = (u32)play->csCtx.actorCues[cueChannel]->startPos.x;
                } else {
                    this->clockAccel = -5.0f;
                }
                this->clockSpeed += this->clockAccel;
                break;

            case SOTCS_CUEID_RESET_CLOCKS_SPEED_UP:
                if (play->csCtx.actorCues[cueChannel]->startPos.x != 0) {
                    this->clockAccel += (u32)play->csCtx.actorCues[cueChannel]->startPos.x;
                } else {
                    this->clockAccel += 2.0f;
                }

                this->clockSpeed += this->clockAccel;
                if (this->clockSpeed > 10000.0f) {
                    this->clockSpeed = 10000.0f;
                    this->cueId = SOTCS_CUEID_NONE;
                }
                break;

            case SOTCS_CUEID_NONE:
            default:
                this->drawType = SOTCS_DRAW_TYPE_NONE;
                return;

            case SOTCS_CUEID_DOUBLE_END:
                Play_SetRespawnData(&play->state, RESPAWN_MODE_RETURN, ((void)0, gSaveContext.save.entrance),
                                    player->unk_3CE, PLAYER_PARAMS(0xFF, PLAYER_INITMODE_B), &player->unk_3C0,
                                    player->unk_3CC);
                this->drawType = SOTCS_DRAW_TYPE_NONE;
                play->transitionTrigger = TRANS_TRIGGER_START;
                play->nextEntrance = gSaveContext.respawn[RESPAWN_MODE_RETURN].entrance;
                play->transitionType = TRANS_TYPE_FADE_BLACK;
                if ((CURRENT_TIME > CLOCK_TIME(18, 0)) || (CURRENT_TIME < CLOCK_TIME(6, 0))) {
                    gSaveContext.respawnFlag = -0x63;
                    SET_EVENTINF(EVENTINF_TRIGGER_DAYTELOP);
                } else {
                    gSaveContext.respawnFlag = 2;
                }
                play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                return;
        }
    } else {
        switch (this->cueId) {
            case SOTCS_CUEID_DOUBLE_INIT:
                this->drawType = SOTCS_DRAW_DOUBLE_SOT;
                this->counter = 0;
                this->clockAngle = 0;
                player->actor.shape.shadowDraw = NULL;
                this->clockColorGray = 38;
                this->clockSpeed = 150.0f;
                this->clockDist = 480.0f;

            case SOTCS_CUEID_DOUBLE_WAIT:
                break;

            case SOTCS_CUEID_DOUBLE_CLOCKS_INWARD:
                this->clockRadialSpeed = -32.0f;
                this->clockColorGray += 6;
                this->clockDist += -32.0f;
                break;

            case SOTCS_CUEID_DOUBLE_CLOCKS_SPIN:
                this->clockColorGray -= 4;
                break;

            case SOTCS_CUEID_DOUBLE_CLOCKS_OUTWARD:
                this->clockColorGray -= 8;
                this->clockRadialSpeed += 20.0f;
                this->clockDist += this->clockRadialSpeed;
                if (this->clockDist > 3500.0f) {
                    this->clockDist = 3500.0f;
                    this->cueId = SOTCS_CUEID_NONE;
                }
                break;

            case SOTCS_CUEID_RESET_INIT:
                this->drawType = SOTCS_DRAW_RESET_CYCLE_SOT;
                this->counter = 0;
                this->clockAngle = 0;
                player->actor.shape.shadowDraw = NULL;
                this->clockSpeed = 100.0f;
                this->speed = 20.0f;
                this->clockDist = 300.0f;
                this->clockAccel = 0.0f;
                break;

            case SOTCS_CUEID_RESET_CLOCKS_SLOW_DOWN:
                this->clockAccel = -5.0f;
                this->clockSpeed += -5.0f;
                break;

            case SOTCS_CUEID_RESET_CLOCKS_SPEED_UP:
                this->clockAccel += 2.0f;
                this->clockSpeed += this->clockAccel;
                if (this->clockSpeed > 10000.0f) {
                    this->clockSpeed = 10000.0f;
                    this->cueId = SOTCS_CUEID_NONE;
                }
                break;

            case SOTCS_CUEID_NONE:
            default:
                this->drawType = SOTCS_DRAW_TYPE_NONE;
                return;

            case SOTCS_CUEID_DOUBLE_END:
                // @recomp Replace DSoT functionality if the option for it is enabled.
                if (!dsot_enabled() && (CURRENT_TIME > CLOCK_TIME(12, 0))) {
                    Play_SetRespawnData(&play->state, RESPAWN_MODE_RETURN, ((void)0, gSaveContext.save.entrance),
                                        player->unk_3CE, PLAYER_PARAMS(0xFF, PLAYER_INITMODE_B), &player->unk_3C0,
                                        player->unk_3CC);
                    this->drawType = SOTCS_DRAW_TYPE_NONE;
                    play->transitionTrigger = TRANS_TRIGGER_START;
                    play->nextEntrance = gSaveContext.respawn[RESPAWN_MODE_RETURN].entrance;
                    play->transitionType = TRANS_TYPE_FADE_BLACK;
                    gSaveContext.respawnFlag = 2;
                    play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                }
                return;
        }
    }

    SoTCsAmmoDrops* sSoTCsAmmoDrops_reloc = KaleidoManager_GetRamAddr(sSoTCsAmmoDrops);
    if (this->drawType == SOTCS_DRAW_RESET_CYCLE_SOT) {
        for (i = 0; i < ARRAY_COUNT(sSoTCsAmmoDrops_reloc); i++) {
            sSoTCsAmmoDrops_reloc[i].pos.x += 2.0f * ((2.0f * Rand_ZeroOne()) - 1.0f);
            sSoTCsAmmoDrops_reloc[i].pos.z += 2.0f * ((2.0f * Rand_ZeroOne()) - 1.0f);
            sSoTCsAmmoDrops_reloc[i].pos.y += 3.0f;

            if (player->actor.world.pos.y < sSoTCsAmmoDrops_reloc[i].pos.y) {
                yDiff = sSoTCsAmmoDrops_reloc[i].pos.y - player->actor.world.pos.y;
                if (yDiff > 550.0f) {
                    yDiff = 1.0f;
                } else {
                    yDiff /= 550.0f;
                }
                sSoTCsAmmoDrops_reloc[i].scale = SQ(yDiff);
            } else {
                sSoTCsAmmoDrops_reloc[i].scale = -10.0f;
            }
        }
    }
    this->counter++;
}