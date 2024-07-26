#include "patches.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"
#include "sound.h"

void AudioSeq_ProcessSeqCmd(u32 cmd);
void AudioThread_QueueCmd(u32 opArgs, void** data);

// Direct audio command (skips the queueing system)
#define SEQCMD_SET_SEQPLAYER_VOLUME_NOW(seqPlayerIndex, duration, volume)                          \
    AudioSeq_ProcessSeqCmd((SEQCMD_OP_SET_SEQPLAYER_VOLUME << 28) | ((u8)(seqPlayerIndex) << 24) | \
                           ((u8)(duration) << 16) | ((u8)((volume)*127.0f)));

bool is_bgm_player(u8 player_index) {
    return player_index == SEQ_PLAYER_BGM_MAIN || player_index == SEQ_PLAYER_BGM_SUB;
}

/**
 * Update different commands and requests for active sequences
 */
RECOMP_PATCH void AudioSeq_UpdateActiveSequences(void) {
    u32 tempoCmd;
    u16 tempoPrev;
    u16 seqId;
    u16 channelMask;
    u16 tempoTarget;
    u8 setupOp;
    u8 targetSeqPlayerIndex;
    u8 setupVal2;
    u8 setupVal1;
    u8 tempoOp;
    s32 pad[2];
    u32 retMsg;
    f32 volume;
    u8 tempoTimer;
    u8 seqPlayerIndex;
    u8 j;
    u8 channelIndex;

    for (seqPlayerIndex = 0; seqPlayerIndex < SEQ_PLAYER_MAX; seqPlayerIndex++) {

        // The seqPlayer has finished initializing and is currently playing the active sequences
        if (gActiveSeqs[seqPlayerIndex].isSeqPlayerInit && gAudioCtx.seqPlayers[seqPlayerIndex].enabled) {
            gActiveSeqs[seqPlayerIndex].isSeqPlayerInit = false;
        }

        // The seqPlayer is no longer playing the active sequences
        if ((AudioSeq_GetActiveSeqId(seqPlayerIndex) != NA_BGM_DISABLED) &&
            !gAudioCtx.seqPlayers[seqPlayerIndex].enabled && (!gActiveSeqs[seqPlayerIndex].isSeqPlayerInit)) {
            gActiveSeqs[seqPlayerIndex].seqId = NA_BGM_DISABLED;
        }

        // Check if the requested sequences is waiting for fonts to load
        if (gActiveSeqs[seqPlayerIndex].isWaitingForFonts) {
            switch ((s32)AudioThread_GetExternalLoadQueueMsg(&retMsg)) {
                case SEQ_PLAYER_BGM_MAIN + 1:
                case SEQ_PLAYER_FANFARE + 1:
                case SEQ_PLAYER_SFX + 1:
                case SEQ_PLAYER_BGM_SUB + 1:
                case SEQ_PLAYER_AMBIENCE + 1:
                    // The fonts have been loaded successfully.
                    gActiveSeqs[seqPlayerIndex].isWaitingForFonts = false;
                    // Queue the same command that was stored previously, but without the 0x8000
                    AudioSeq_ProcessSeqCmd(gActiveSeqs[seqPlayerIndex].startAsyncSeqCmd);
                    break;
                case 0xFF:
                    // There was an error in loading the fonts
                    gActiveSeqs[seqPlayerIndex].isWaitingForFonts = false;
                    break;
            }
        }

        // Update global volume
        if (gActiveSeqs[seqPlayerIndex].fadeVolUpdate) {
            volume = 1.0f;
            for (j = 0; j < VOL_SCALE_INDEX_MAX; j++) {
                volume *= (gActiveSeqs[seqPlayerIndex].volScales[j] / 127.0f);
            }

            SEQCMD_SET_SEQPLAYER_VOLUME((u8)(seqPlayerIndex + (SEQCMD_ASYNC_ACTIVE >> 24)),
                                        gActiveSeqs[seqPlayerIndex].volFadeTimer, (u8)(volume * 127.0f));
            gActiveSeqs[seqPlayerIndex].fadeVolUpdate = false;
        }

        if (gActiveSeqs[seqPlayerIndex].volTimer != 0) {
            gActiveSeqs[seqPlayerIndex].volTimer--;

            if (gActiveSeqs[seqPlayerIndex].volTimer != 0) {
                gActiveSeqs[seqPlayerIndex].volCur -= gActiveSeqs[seqPlayerIndex].volStep;
            } else {
                gActiveSeqs[seqPlayerIndex].volCur = gActiveSeqs[seqPlayerIndex].volTarget;
            }

        }
        // @recomp Send a volume scale command regardless of whether volTimer is active and scale it for background music players.
        f32 cur_volume = gActiveSeqs[seqPlayerIndex].volCur;
        if (is_bgm_player(seqPlayerIndex)) {
            cur_volume *= recomp_get_bgm_volume();
        }
        AUDIOCMD_SEQPLAYER_FADE_VOLUME_SCALE(seqPlayerIndex, cur_volume);

        // Process tempo
        if (gActiveSeqs[seqPlayerIndex].tempoCmd != 0) {
            tempoCmd = gActiveSeqs[seqPlayerIndex].tempoCmd;
            tempoTimer = (tempoCmd & 0xFF0000) >> 15;
            tempoTarget = tempoCmd & 0xFFF;
            if (tempoTimer == 0) {
                tempoTimer++;
            }

            // Process tempo commands
            if (gAudioCtx.seqPlayers[seqPlayerIndex].enabled) {
                tempoPrev = gAudioCtx.seqPlayers[seqPlayerIndex].tempo / TATUMS_PER_BEAT;
                tempoOp = (tempoCmd & 0xF000) >> 12;
                switch (tempoOp) {
                    case SEQCMD_SUB_OP_TEMPO_SPEED_UP:
                        // Speed up tempo by `tempoTarget` amount
                        tempoTarget += tempoPrev;
                        break;

                    case SEQCMD_SUB_OP_TEMPO_SLOW_DOWN:
                        // Slow down tempo by `tempoTarget` amount
                        if (tempoTarget < tempoPrev) {
                            tempoTarget = tempoPrev - tempoTarget;
                        }
                        break;

                    case SEQCMD_SUB_OP_TEMPO_SCALE:
                        // Scale tempo by a multiplicative factor
                        tempoTarget = tempoPrev * (tempoTarget / 100.0f);
                        break;

                    case SEQCMD_SUB_OP_TEMPO_RESET:
                        // Reset tempo to original tempo
                        tempoTarget = (gActiveSeqs[seqPlayerIndex].tempoOriginal != 0)
                                          ? gActiveSeqs[seqPlayerIndex].tempoOriginal
                                          : tempoPrev;
                        break;

                    default: // `SEQCMD_SUB_OP_TEMPO_SET`
                        // `tempoTarget` is the new tempo
                        break;
                }

                if (gActiveSeqs[seqPlayerIndex].tempoOriginal == 0) {
                    gActiveSeqs[seqPlayerIndex].tempoOriginal = tempoPrev;
                }

                gActiveSeqs[seqPlayerIndex].tempoTarget = tempoTarget;
                gActiveSeqs[seqPlayerIndex].tempoCur = gAudioCtx.seqPlayers[seqPlayerIndex].tempo / 0x30;
                gActiveSeqs[seqPlayerIndex].tempoStep =
                    (gActiveSeqs[seqPlayerIndex].tempoCur - gActiveSeqs[seqPlayerIndex].tempoTarget) / tempoTimer;
                gActiveSeqs[seqPlayerIndex].tempoTimer = tempoTimer;
                gActiveSeqs[seqPlayerIndex].tempoCmd = 0;
            }
        }

        // Step tempo to target
        if (gActiveSeqs[seqPlayerIndex].tempoTimer != 0) {
            gActiveSeqs[seqPlayerIndex].tempoTimer--;
            if (gActiveSeqs[seqPlayerIndex].tempoTimer != 0) {
                gActiveSeqs[seqPlayerIndex].tempoCur -= gActiveSeqs[seqPlayerIndex].tempoStep;
            } else {
                gActiveSeqs[seqPlayerIndex].tempoCur = gActiveSeqs[seqPlayerIndex].tempoTarget;
            }

            AUDIOCMD_SEQPLAYER_SET_TEMPO(seqPlayerIndex, gActiveSeqs[seqPlayerIndex].tempoCur);
        }

        // Update channel volumes
        if (gActiveSeqs[seqPlayerIndex].volChannelFlags != 0) {
            for (channelIndex = 0; channelIndex < SEQ_NUM_CHANNELS; channelIndex++) {
                if (gActiveSeqs[seqPlayerIndex].channelData[channelIndex].volTimer != 0) {
                    gActiveSeqs[seqPlayerIndex].channelData[channelIndex].volTimer--;
                    if (gActiveSeqs[seqPlayerIndex].channelData[channelIndex].volTimer != 0) {
                        gActiveSeqs[seqPlayerIndex].channelData[channelIndex].volCur -=
                            gActiveSeqs[seqPlayerIndex].channelData[channelIndex].volStep;
                    } else {
                        gActiveSeqs[seqPlayerIndex].channelData[channelIndex].volCur =
                            gActiveSeqs[seqPlayerIndex].channelData[channelIndex].volTarget;
                        gActiveSeqs[seqPlayerIndex].volChannelFlags ^= (1 << channelIndex);
                    }

                    AUDIOCMD_CHANNEL_SET_VOL_SCALE(seqPlayerIndex, channelIndex,
                                                   gActiveSeqs[seqPlayerIndex].channelData[channelIndex].volCur);
                }
            }
        }

        // Update frequencies
        if (gActiveSeqs[seqPlayerIndex].freqScaleChannelFlags != 0) {
            for (channelIndex = 0; channelIndex < SEQ_NUM_CHANNELS; channelIndex++) {
                if (gActiveSeqs[seqPlayerIndex].channelData[channelIndex].freqScaleTimer != 0) {
                    gActiveSeqs[seqPlayerIndex].channelData[channelIndex].freqScaleTimer--;
                    if (gActiveSeqs[seqPlayerIndex].channelData[channelIndex].freqScaleTimer != 0) {
                        gActiveSeqs[seqPlayerIndex].channelData[channelIndex].freqScaleCur -=
                            gActiveSeqs[seqPlayerIndex].channelData[channelIndex].freqScaleStep;
                    } else {
                        gActiveSeqs[seqPlayerIndex].channelData[channelIndex].freqScaleCur =
                            gActiveSeqs[seqPlayerIndex].channelData[channelIndex].freqScaleTarget;
                        gActiveSeqs[seqPlayerIndex].freqScaleChannelFlags ^= (1 << channelIndex);
                    }

                    AUDIOCMD_CHANNEL_SET_FREQ_SCALE(seqPlayerIndex, channelIndex,
                                                    gActiveSeqs[seqPlayerIndex].channelData[channelIndex].freqScaleCur);
                }
            }
        }

        // Process setup commands
        if (gActiveSeqs[seqPlayerIndex].setupCmdNum != 0) {
            // If there is a SeqCmd to reset the audio heap queued, then drop all setup commands
            if (!AudioSeq_IsSeqCmdNotQueued(SEQCMD_OP_RESET_AUDIO_HEAP << 28, SEQCMD_OP_MASK)) {
                gActiveSeqs[seqPlayerIndex].setupCmdNum = 0;
                break;
            }

            // Only process setup commands once the timer reaches zero
            if (gActiveSeqs[seqPlayerIndex].setupCmdTimer != 0) {
                gActiveSeqs[seqPlayerIndex].setupCmdTimer--;
                continue;
            }

            // Only process setup commands if `seqPlayerIndex` if no longer playing
            // i.e. the `seqPlayer` is no longer enabled
            if (gAudioCtx.seqPlayers[seqPlayerIndex].enabled) {
                continue;
            }

            for (j = 0; j < gActiveSeqs[seqPlayerIndex].setupCmdNum; j++) {
                setupOp = (gActiveSeqs[seqPlayerIndex].setupCmd[j] & 0xF00000) >> 20;
                targetSeqPlayerIndex = (gActiveSeqs[seqPlayerIndex].setupCmd[j] & 0xF0000) >> 16;
                setupVal2 = (gActiveSeqs[seqPlayerIndex].setupCmd[j] & 0xFF00) >> 8;
                setupVal1 = gActiveSeqs[seqPlayerIndex].setupCmd[j] & 0xFF;

                switch (setupOp) {
                    case SEQCMD_SUB_OP_SETUP_RESTORE_SEQPLAYER_VOLUME:
                        // Restore `targetSeqPlayerIndex` volume back to normal levels
                        AudioSeq_SetVolumeScale(targetSeqPlayerIndex, VOL_SCALE_INDEX_FANFARE, 0x7F, setupVal1);
                        break;

                    case SEQCMD_SUB_OP_SETUP_RESTORE_SEQPLAYER_VOLUME_IF_QUEUED:
                        // Restore `targetSeqPlayerIndex` volume back to normal levels,
                        // but only if the number of sequence queue requests from `sSeqRequests`
                        // exactly matches the argument to the command
                        if (setupVal1 == sNumSeqRequests[seqPlayerIndex]) {
                            AudioSeq_SetVolumeScale(targetSeqPlayerIndex, VOL_SCALE_INDEX_FANFARE, 0x7F, setupVal2);
                        }
                        break;

                    case SEQCMD_SUB_OP_SETUP_SEQ_UNQUEUE:
                        // Unqueue `seqPlayerIndex` from sSeqRequests
                        //! @bug this command does not work as intended as unqueueing
                        //! the sequence relies on `gActiveSeqs[seqPlayerIndex].seqId`
                        //! However, `gActiveSeqs[seqPlayerIndex].seqId` is reset before the sequence on
                        //! `seqPlayerIndex` is requested to stop, i.e. before the sequence is disabled and setup
                        //! commands (including this command) can run. A simple fix would have been to unqueue based on
                        //! `gActiveSeqs[seqPlayerIndex].prevSeqId` instead
                        SEQCMD_UNQUEUE_SEQUENCE((u8)(seqPlayerIndex + (SEQCMD_ASYNC_ACTIVE >> 24)), 0,
                                                gActiveSeqs[seqPlayerIndex].seqId);
                        break;

                    case SEQCMD_SUB_OP_SETUP_RESTART_SEQ:
                        // Restart the currently active sequence on `targetSeqPlayerIndex` with full volume.
                        // Sequence on `targetSeqPlayerIndex` must still be active to play (can be muted)
                        SEQCMD_PLAY_SEQUENCE((u8)(targetSeqPlayerIndex + (SEQCMD_ASYNC_ACTIVE >> 24)), 1,
                                             gActiveSeqs[targetSeqPlayerIndex].seqId);
                        gActiveSeqs[targetSeqPlayerIndex].fadeVolUpdate = true;
                        gActiveSeqs[targetSeqPlayerIndex].volScales[1] = 0x7F;
                        break;

                    case SEQCMD_SUB_OP_SETUP_TEMPO_SCALE:
                        // Scale tempo by a multiplicative factor
                        SEQCMD_SCALE_TEMPO((u8)(targetSeqPlayerIndex + (SEQCMD_ASYNC_ACTIVE >> 24)), setupVal2,
                                           setupVal1);
                        break;

                    case SEQCMD_SUB_OP_SETUP_TEMPO_RESET:
                        // Reset tempo to previous tempo
                        SEQCMD_RESET_TEMPO((u8)(targetSeqPlayerIndex + (SEQCMD_ASYNC_ACTIVE >> 24)), setupVal1);
                        break;

                    case SEQCMD_SUB_OP_SETUP_PLAY_SEQ:
                        // Play the requested sequence
                        // Uses the fade timer set by `SEQCMD_SUB_OP_SETUP_SET_FADE_TIMER`
                        seqId = gActiveSeqs[seqPlayerIndex].setupCmd[j] & 0xFFFF;
                        SEQCMD_PLAY_SEQUENCE((u8)(targetSeqPlayerIndex + (SEQCMD_ASYNC_ACTIVE >> 24)),
                                             gActiveSeqs[targetSeqPlayerIndex].setupFadeTimer, seqId);
                        AudioSeq_SetVolumeScale(targetSeqPlayerIndex, VOL_SCALE_INDEX_FANFARE, 0x7F, 0);
                        gActiveSeqs[targetSeqPlayerIndex].setupFadeTimer = 0;
                        break;

                    case SEQCMD_SUB_OP_SETUP_SET_FADE_TIMER:
                        // A command specifically to support `SEQCMD_SUB_OP_SETUP_PLAY_SEQ`
                        // Sets the fade timer for the sequence requested in `SEQCMD_SUB_OP_SETUP_PLAY_SEQ`
                        gActiveSeqs[seqPlayerIndex].setupFadeTimer = setupVal2;
                        break;

                    case SEQCMD_SUB_OP_SETUP_RESTORE_SEQPLAYER_VOLUME_WITH_SCALE_INDEX:
                        // Restore the volume back to default levels
                        // Allows a `scaleIndex` to be specified.
                        AudioSeq_SetVolumeScale(targetSeqPlayerIndex, setupVal2, 0x7F, setupVal1);
                        break;

                    case SEQCMD_SUB_OP_SETUP_POP_PERSISTENT_CACHE:
                        // Discard audio data by popping one more audio caches from the audio heap
                        if (setupVal1 & (1 << SEQUENCE_TABLE)) {
                            AUDIOCMD_GLOBAL_POP_PERSISTENT_CACHE(SEQUENCE_TABLE);
                        }
                        if (setupVal1 & (1 << FONT_TABLE)) {
                            AUDIOCMD_GLOBAL_POP_PERSISTENT_CACHE(FONT_TABLE);
                        }
                        if (setupVal1 & (1 << SAMPLE_TABLE)) {
                            AUDIOCMD_GLOBAL_POP_PERSISTENT_CACHE(SAMPLE_TABLE);
                        }
                        break;

                    case SEQCMD_SUB_OP_SETUP_SET_CHANNEL_DISABLE_MASK:
                        // Disable (or reenable) specific channels of `targetSeqPlayerIndex`
                        channelMask = gActiveSeqs[seqPlayerIndex].setupCmd[j] & 0xFFFF;
                        SEQCMD_SET_CHANNEL_DISABLE_MASK((u8)(targetSeqPlayerIndex + (SEQCMD_ASYNC_ACTIVE >> 24)),
                                                        channelMask);
                        break;

                    case SEQCMD_SUB_OP_SETUP_SET_SEQPLAYER_FREQ:
                        // Scale all channels of `targetSeqPlayerIndex`
                        SEQCMD_SET_SEQPLAYER_FREQ((u8)(targetSeqPlayerIndex + (SEQCMD_ASYNC_ACTIVE >> 24)), setupVal2,
                                                  setupVal1 * 10);
                        break;

                    default:
                        break;
                }
            }

            gActiveSeqs[seqPlayerIndex].setupCmdNum = 0;
        }
    }
}

// @recomp Patched to add the ability to turn off low health beeps.
RECOMP_PATCH void LifeMeter_UpdateSizeAndBeep(PlayState* play) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;

    if (interfaceCtx->lifeSizeChangeDirection != 0) {
        interfaceCtx->lifeSizeChange--;
        if (interfaceCtx->lifeSizeChange <= 0) {
            interfaceCtx->lifeSizeChange = 0;
            interfaceCtx->lifeSizeChangeDirection = 0;
            // @recomp Additional check for whether low health beeps are enabled.
            if (recomp_get_low_health_beeps_enabled() && !Player_InCsMode(play) && (play->pauseCtx.state == PAUSE_STATE_OFF) &&
                (play->pauseCtx.debugEditor == DEBUG_EDITOR_NONE) && LifeMeter_IsCritical() && !Play_InCsMode(play)) {
                Audio_PlaySfx(NA_SE_SY_HITPOINT_ALARM);
            }
        }
    } else {
        interfaceCtx->lifeSizeChange++;
        if ((s32)interfaceCtx->lifeSizeChange >= 10) {
            interfaceCtx->lifeSizeChange = 10;
            interfaceCtx->lifeSizeChangeDirection = 1;
        }
    }
}

