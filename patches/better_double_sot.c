#include "patches.h"

void dsot_load_day_number_texture(PlayState* play, s32 day);
void dsot_actor_fixes(PlayState* play);

bool dsot_on;

void dsot_determine_enabled(void) {
    dsot_on = recomp_dsot_enabled();
}

bool dsot_enabled(void) {
    return dsot_on;
}

u8 choiceHour;

void dsot_init_hour_selection(PlayState* play) {
    choiceHour = (u16)(TIME_TO_HOURS_F(CURRENT_TIME - 1)) + 1;
    if (choiceHour >= 24) {
        choiceHour = 0;
    }
    if (choiceHour == 6) {
        if (gSaveContext.save.day == 3) {
            choiceHour = 5;
        } else {
            dsot_load_day_number_texture(play, gSaveContext.save.day + 1);
        }
    }
}

#define TIMER_INIT 10
#define TIMER_STEP_RESET 0

void dsot_handle_hour_selection(PlayState* play) {
    static s8 sAnalogStickTimer = TIMER_INIT;
    static s8 sPrevStickX = 0;
    static bool sAnalogStickHeld = false;
    Input* input = CONTROLLER1(&play->state);

    if ((input->rel.stick_x >= 30) && !sAnalogStickHeld) {
        u8 prevHour = choiceHour;
        sAnalogStickHeld = true;

        choiceHour++;
        if (choiceHour >= 24) {
            choiceHour = 0;
        }

        if ((CURRENT_DAY == 3) && (prevHour <= 5) && (choiceHour > 5)) {
            choiceHour = 5;
        } else if ((prevHour <= 6) && (choiceHour > 6)) {
            choiceHour = 6;
        } else {
            Audio_PlaySfx(NA_SE_SY_CURSOR);
        }

        if ((choiceHour == 6) && (choiceHour > prevHour)) {
            dsot_load_day_number_texture(play, gSaveContext.save.day + 1);
        }
    } else if ((input->rel.stick_x <= -30) && !sAnalogStickHeld) {
        u8 prevHour = choiceHour;
        sAnalogStickHeld = true;

        choiceHour--;
        if (choiceHour > 24) {
            choiceHour = 23;
        }

        if (((CLOCK_TIME(choiceHour, 0) <= CURRENT_TIME) && (CLOCK_TIME(prevHour, 0) > CURRENT_TIME)
        || ((choiceHour > prevHour) && (CURRENT_TIME >= CLOCK_TIME(23, 0)))
        || ((choiceHour > prevHour) && (CURRENT_TIME < CLOCK_TIME(1, 0))))) {
            choiceHour = prevHour;
        }
        else {
            Audio_PlaySfx(NA_SE_SY_CURSOR);
        }

        if ((prevHour == 6) && (choiceHour < prevHour)) {
            dsot_load_day_number_texture(play, gSaveContext.save.day);
        }
    } else if (ABS_ALT(input->rel.stick_x) < 30) {
        sAnalogStickHeld = false;
        sAnalogStickTimer = TIMER_INIT;
    }

    if (ABS_ALT(input->rel.stick_x) >= 30) {
        if (!DECR(sAnalogStickTimer)) {
            sAnalogStickHeld = false;
            sAnalogStickTimer = TIMER_STEP_RESET;
        }
    }

    if (sPrevStickX * input->rel.stick_x < 0) {
        sAnalogStickHeld = false;
        sAnalogStickTimer = TIMER_INIT;
    }

    sPrevStickX = input->rel.stick_x;
}

void dsot_cancel_hour_selection(PlayState* play) {
    if (choiceHour == 6) {
        dsot_load_day_number_texture(play, gSaveContext.save.day);
    }
}

void dsot_advance_hour(PlayState* play) {
    gSaveContext.save.time = CLOCK_TIME(choiceHour, 0);
    gSaveContext.skyboxTime = CURRENT_TIME;

    // Instantly enable/disable rain on day 2.
    if ((CURRENT_DAY == 2) && (Environment_GetStormState(play) != STORM_STATE_OFF)) {
        if ((CURRENT_TIME >= CLOCK_TIME(8, 0)) && (CURRENT_TIME < CLOCK_TIME(18, 00))) {
            gWeatherMode = WEATHER_MODE_RAIN;
            play->envCtx.lightningState = LIGHTNING_ON;
            play->envCtx.precipitation[PRECIP_RAIN_MAX] = 60;
            play->envCtx.precipitation[PRECIP_RAIN_CUR] = 60;
            Environment_PlayStormNatureAmbience(play);
        } else {
            gWeatherMode = WEATHER_MODE_CLEAR;
            play->envCtx.lightningState = LIGHTNING_OFF;
            play->envCtx.precipitation[PRECIP_RAIN_MAX] = 0;
            play->envCtx.precipitation[PRECIP_RAIN_CUR] = 0;
            Environment_StopStormNatureAmbience(play);
        }
    }

    // Play music/ambience.
    play->envCtx.timeSeqState = TIMESEQ_FADE_DAY_BGM;

    if ((CURRENT_TIME >= CLOCK_TIME(18, 0)) || (CURRENT_TIME <= CLOCK_TIME(6,0))) {
        SEQCMD_STOP_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 0);
        play->envCtx.timeSeqState = TIMESEQ_NIGHT_BEGIN_SFX;
    } else if (CURRENT_TIME > CLOCK_TIME(17, 10)) {
        SEQCMD_STOP_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 240);
        play->envCtx.timeSeqState = TIMESEQ_NIGHT_BEGIN_SFX;
    }

    if ((CURRENT_TIME >= CLOCK_TIME(18, 0)) || (CURRENT_TIME <= CLOCK_TIME(6, 0))) {
        Audio_PlayAmbience(play->sequenceCtx.ambienceId);
        Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_CRITTER_0, 1, 1);
        play->envCtx.timeSeqState = TIMESEQ_NIGHT_DELAY;
    }

    if ((CURRENT_TIME >= CLOCK_TIME(19, 0)) || (CURRENT_TIME <= CLOCK_TIME(6, 0))) {
        Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_CRITTER_0, 1, 0);
        Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_CRITTER_1 << 4 | AMBIENCE_CHANNEL_CRITTER_3, 1, 1);
        play->envCtx.timeSeqState = TIMESEQ_DAY_BEGIN_SFX;
    }

    if ((CURRENT_TIME >= CLOCK_TIME(5, 0)) && (CURRENT_TIME <= CLOCK_TIME(6, 0))) {
        Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_CRITTER_1 << 4 | AMBIENCE_CHANNEL_CRITTER_3, 1, 0);
        Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_CRITTER_4 << 4 | AMBIENCE_CHANNEL_CRITTER_5, 1, 1);
        play->envCtx.timeSeqState = TIMESEQ_DAY_DELAY;
    }

    dsot_actor_fixes(play);
}

// Loads day number texture from week_static for the three-day clock.
void dsot_load_day_number_texture(PlayState* play, s32 day) {
    s16 i = day - 1;

    if ((i < 0) || (i >= 3)) {
        i = 0;
    }

    DmaMgr_SendRequest0((void*)(play->interfaceCtx.doActionSegment + 0x780),
                        SEGMENT_ROM_START_OFFSET(week_static, i * 0x510), 0x510);
}

#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"

void Interface_DrawClock(PlayState* play);

// Wrapper for Interface_DrawClock to display selected hour.
void dsot_draw_clock(PlayState* play) {
    MessageContext *msgCtx = &play->msgCtx;

    u8 prev_msgMode = msgCtx->msgMode;
    msgCtx->msgMode = MSGMODE_NONE;

    PlayState* state = (PlayState*)&play->state;
    s32 prev_frameAdvanceCtx = state->frameAdvCtx.enabled;
    state->frameAdvCtx.enabled = 0;

    u16 prev_pauseCtx_state = play->pauseCtx.state;
    play->pauseCtx.state = PAUSE_STATE_OFF;

    u16 prev_pauseCtx_debugEditor = play->pauseCtx.state;
    play->pauseCtx.debugEditor = DEBUG_EDITOR_NONE;

    bool flag_modified = false;
    if (!(play->actorCtx.flags & ACTORCTX_FLAG_TELESCOPE_ON)) {
        play->actorCtx.flags ^= ACTORCTX_FLAG_TELESCOPE_ON;
        flag_modified = true;
    }

    u16 prev_time = gSaveContext.save.time;
    gSaveContext.save.time = CLOCK_TIME(choiceHour, 0);

    Interface_DrawClock(play);

    gSaveContext.save.time = prev_time;

    if (flag_modified) {
        play->actorCtx.flags ^= ACTORCTX_FLAG_TELESCOPE_ON;
    }

    play->pauseCtx.debugEditor == prev_pauseCtx_debugEditor;
    play->pauseCtx.state = prev_pauseCtx_state;
    state->frameAdvCtx.enabled = prev_frameAdvanceCtx;
    msgCtx->msgMode = prev_msgMode;
}


#include "overlays/actors/ovl_En_Test4/z_en_test4.h"
#include "overlays/actors/ovl_Obj_Tokei_Step/z_obj_tokei_step.h"
#include "overlays/actors/ovl_Obj_Tokeidai/z_obj_tokeidai.h"

void dsot_ObjEnTest4_fix(EnTest4* this, PlayState* play);
void dsot_ObjTokeiStep_fix(ObjTokeiStep* this, PlayState* play);
void dsot_ObjTokeidai_fix(ObjTokeidai* this, PlayState* play);

// Calls actor specific fixes.
void dsot_actor_fixes(PlayState* play) {
    s32 category;
    Actor* actor;
    Player* player = GET_PLAYER(play);
    u32* categoryFreezeMaskP;
    ActorListEntry* entry;

    ActorContext* actorCtx = &play->actorCtx;

    for (category = 0, entry = actorCtx->actorLists; category < ACTORCAT_MAX;
         entry++, categoryFreezeMaskP++, category++) {
        actor = entry->first;

        for (actor = entry->first; actor != NULL; actor = actor->next) {
            switch(actor->id) {
                case ACTOR_EN_TEST4:
                    dsot_ObjEnTest4_fix(actor, play);
                    break;
                case ACTOR_OBJ_TOKEI_STEP:
                    dsot_ObjTokeiStep_fix(actor, play);
                    break;
                case ACTOR_OBJ_TOKEIDAI:
                    dsot_ObjTokeidai_fix(actor, play);
                    break;
            }
        }
    }
}

#define PAST_MIDNIGHT ((CURRENT_DAY == 3) && (CURRENT_TIME < CLOCK_TIME(6, 0)) && (CURRENT_TIME > CLOCK_TIME(0, 0)))

// z_obj_en_test_4
void func_80A42198(EnTest4* this); // EnTest4_GetBellTimeOnDay3

void dsot_ObjEnTest4_fix(EnTest4* this, PlayState* play) {
    this->prevTime = CURRENT_TIME;
    this->prevBellTime = CURRENT_TIME;

    // Change daytime to night manually if necessary.
    if ((this->daytimeIndex = THREEDAY_DAYTIME_DAY) && (CURRENT_TIME > CLOCK_TIME(18, 0)) || (CURRENT_TIME <= CLOCK_TIME(6, 0))) {
        this->daytimeIndex = THREEDAY_DAYTIME_NIGHT;
        // Re-spawn the setup actors.
        play->numSetupActors = -play->numSetupActors;
    }

    // Setup next bell time.
    if (CURRENT_DAY == 3) {
        gSaveContext.save.time--;
        func_80A42198(this);
        gSaveContext.save.time++;
    }
}

// z_obj_tokei_step

void ObjTokeiStep_InitStepsOpen(ObjTokeiStep* this);
void ObjTokeiStep_SetupDoNothingOpen(ObjTokeiStep* this);
void ObjTokeiStep_DrawOpen(Actor* thisx, PlayState* play);

void dsot_ObjTokeiStep_fix(ObjTokeiStep* this, PlayState* play) {
    if (PAST_MIDNIGHT) {
        // @recomp Manual relocation, TODO remove when the recompiler handles this automatically.
        this->dyna.actor.draw = actor_relocate(this, ObjTokeiStep_DrawOpen);
        ObjTokeiStep_InitStepsOpen(this);
        ObjTokeiStep_SetupDoNothingOpen(this);
        DynaPoly_DisableCollision(play, &play->colCtx.dyna, this->dyna.bgId);
    }
}

// z_obj_tokeidai

#define GET_CLOCK_FACE_ROTATION(currentClockHour) (TRUNCF_BINANG(currentClockHour * (0x10000 / 24.0f)))
#define GET_MINUTE_RING_OR_EXTERIOR_GEAR_ROTATION(currentClockMinute) \
    (TRUNCF_BINANG(currentClockMinute * (0x10000 * 12.0f / 360)))

s32 ObjTokeidai_GetTargetSunMoonPanelRotation(void);
void ObjTokeidai_Init(Actor* thisx, PlayState* play);
void ObjTokeidai_Draw(Actor* thisx, PlayState* play);
void ObjTokeidai_ExteriorGear_Draw(Actor* thisx, PlayState* play);
void ObjTokeidai_Clock_Draw(Actor* thisx, PlayState* play);

void ObjTokeidai_Counterweight_Idle(ObjTokeidai* this, PlayState* play);
void ObjTokeidai_ExteriorGear_Idle(ObjTokeidai* this, PlayState* play);
void ObjTokeidai_TowerClock_Idle(ObjTokeidai* this, PlayState* play);

s32 dsot_ObjTokeidai_get_target_sun_moon_panel_rotation(void) {
    if (CURRENT_TIME >= CLOCK_TIME(18, 0) || CURRENT_TIME < CLOCK_TIME(6, 0)) {
        return 0x8000;
    }
    return 0;
}

void dsot_ObjTokeidai_update_clock(ObjTokeidai* this, u16 currentHour, u16 currentMinute) {
    // @recomp Manual relocation, TODO remove when the recompiler handles this automatically.
    this->clockTime = CURRENT_TIME;

    // Instantly switch to desired hour.
    u16 clockHour = currentHour;
    if (currentMinute == 0) {
        clockHour--;
        if (clockHour > 23) {
            clockHour = 23;
        }
    }

    this->clockFaceRotation = GET_CLOCK_FACE_ROTATION(clockHour);
    this->clockHour = clockHour;
    this->clockFaceAngularVelocity = 0;
    this->clockFaceRotationTimer = 0;

    // Instantly switch to desired minute.
    u16 clockMinute = currentMinute - 1;
    if (clockMinute > 59) {
        clockMinute = 59;
    }

    this->minuteRingOrExteriorGearRotation = GET_MINUTE_RING_OR_EXTERIOR_GEAR_ROTATION(clockMinute);
    this->clockMinute = clockMinute;
    this->minuteRingOrExteriorGearAngularVelocity = 0x5A;
    this->minuteRingOrExteriorGearRotationTimer = 0;

    // Switch the medalion rotation immediately.
    if (((currentHour != 6) && (currentHour != 18)) || (currentMinute != 0)) {
        this->sunMoonPanelRotation = dsot_ObjTokeidai_get_target_sun_moon_panel_rotation();
        this->sunMoonPanelAngularVelocity = 0;
    }
}

void dsot_ObjTokeidai_fix(ObjTokeidai* this, PlayState* play) {
    s32 type = OBJ_TOKEIDAI_TYPE(&this->actor);
    u16 currentHour = TIME_TO_HOURS_F(CURRENT_TIME);
    u16 currentMinute = TIME_TO_MINUTES_F(CURRENT_TIME);

    switch(type) {
        case OBJ_TOKEIDAI_TYPE_COUNTERWEIGHT_TERMINA_FIELD:
        case OBJ_TOKEIDAI_TYPE_COUNTERWEIGHT_CLOCK_TOWN:
            if (PAST_MIDNIGHT && (this->actionFunc == actor_relocate(this, ObjTokeidai_Counterweight_Idle))) {
                this->actor.shape.rot.y = 0;
                ObjTokeidai_Init(this, play);
            }
            break;

        case OBJ_TOKEIDAI_TYPE_EXTERIOR_GEAR_TERMINA_FIELD:
        case OBJ_TOKEIDAI_TYPE_EXTERIOR_GEAR_CLOCK_TOWN:
            if (PAST_MIDNIGHT && (this->actionFunc == actor_relocate(this, ObjTokeidai_ExteriorGear_Idle))) {
                dsot_ObjTokeidai_update_clock(this, 0, 0);
                // @recomp Manual relocation, TODO remove when the recompiler handles this automatically.
                this->actor.draw = actor_relocate(this, ObjTokeidai_ExteriorGear_Draw);
                ObjTokeidai_Init(this, play);
            } else {
                dsot_ObjTokeidai_update_clock(this, currentHour, currentMinute);
            }
            break;

        case OBJ_TOKEIDAI_TYPE_TOWER_CLOCK_TERMINA_FIELD:
        case OBJ_TOKEIDAI_TYPE_TOWER_CLOCK_CLOCK_TOWN:
            if (PAST_MIDNIGHT && (this->actionFunc == actor_relocate(this, ObjTokeidai_TowerClock_Idle))) {
                dsot_ObjTokeidai_update_clock(this, 0, 0);
                // @recomp Manual relocation, TODO remove when the recompiler handles this automatically.
                this->actor.draw = actor_relocate(this, ObjTokeidai_Clock_Draw);
                ObjTokeidai_Init(this, play);
            } else {
                dsot_ObjTokeidai_update_clock(this, currentHour, currentMinute);
            }
            break;
        case OBJ_TOKEIDAI_TYPE_STAIRCASE_TO_ROOFTOP:
            if (PAST_MIDNIGHT) {
                // @recomp Manual relocation, TODO remove when the recompiler handles this automatically.
                this->actor.draw = actor_relocate(this, ObjTokeidai_Draw);
            }
            break;
        case OBJ_TOKEIDAI_TYPE_WALL_CLOCK:
        case OBJ_TOKEIDAI_TYPE_SMALL_WALL_CLOCK:
            dsot_ObjTokeidai_update_clock(this, currentHour, currentMinute);
            break;
    }
}