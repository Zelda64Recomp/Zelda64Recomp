#include "patches.h"
#include "transform_ids.h"
#include "overlays/actors/ovl_En_Hanabi/z_en_hanabi.h"
#include "overlays/actors/ovl_Demo_Kankyo/z_demo_kankyo.h"
#include "overlays/actors/ovl_En_Water_Effect/z_en_water_effect.h"

extern EffectSsInfo sEffectSsInfo;

#define MAX_PARTICLES 256
u8 particle_reset_list[MAX_PARTICLES];

// @recomp Patched to track that the particle has been reset.
RECOMP_PATCH void EffectSS_ResetEntry(EffectSs* particle) {
    u32 i;

    particle->type = EFFECT_SS_MAX;
    particle->accel.x = particle->accel.y = particle->accel.z = 0;
    particle->velocity.x = particle->velocity.y = particle->velocity.z = 0;
    particle->vec.x = particle->vec.y = particle->vec.z = 0;
    particle->pos.x = particle->pos.y = particle->pos.z = 0;
    particle->life = -1;
    particle->flags = 0;
    particle->priority = 128;
    particle->draw = NULL;
    particle->update = NULL;
    particle->gfx = NULL;
    particle->actor = NULL;

    for (i = 0; i < ARRAY_COUNT(particle->regs); i++) {
        particle->regs[i] = 0;
    }

    // @recomp Get this particle's index and mark it as being reset.
    u32 particle_index = particle - &sEffectSsInfo.dataTable[0];
    if (particle_index >= sEffectSsInfo.size) {
        recomp_crash("Invalid particle was reset!\n");
    }
    particle_reset_list[particle_index] = true;
}

// @recomp Check numEntries to be sure enough space has been allocated for tracking particle statuses.
RECOMP_PATCH void EffectSS_Init(PlayState* play, s32 numEntries) {
    u32 i;
    EffectSs* effectsSs;
    EffectSsOverlay* overlay;

    // @recomp Perform the numEntries check.
    if (numEntries > MAX_PARTICLES) {
        recomp_crash("Particle reset list too small!\n");
    }

    sEffectSsInfo.dataTable = (EffectSs*)THA_AllocTailAlign16(&play->state.tha, numEntries * sizeof(EffectSs));
    sEffectSsInfo.searchIndex = 0;
    sEffectSsInfo.size = numEntries;

    for (effectsSs = &sEffectSsInfo.dataTable[0]; effectsSs < &sEffectSsInfo.dataTable[sEffectSsInfo.size];
         effectsSs++) {
        EffectSS_ResetEntry(effectsSs);
    }

    overlay = &gParticleOverlayTable[0];
    for (i = 0; i < EFFECT_SS_MAX; i++) {
        overlay->loadedRamAddr = NULL;
        overlay++;
    }
}

// @recomp Add transform tags to particles
RECOMP_PATCH void EffectSS_DrawParticle(PlayState* play, s32 index) {
    EffectSs* entry = &sEffectSsInfo.dataTable[index];

    OPEN_DISPS(play->state.gfxCtx);

    // @recomp If this particle was just reset then skip interpolation.
    if (particle_reset_list[index]) {
        gEXMatrixGroupDecomposedSkipAll(POLY_OPA_DISP++, PARTICLE_TRANSFORM_ID_START + index, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
        gEXMatrixGroupDecomposedSkipAll(POLY_XLU_DISP++, PARTICLE_TRANSFORM_ID_START + index, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
    }
    else {
        gEXMatrixGroupDecomposedNormal(POLY_OPA_DISP++, PARTICLE_TRANSFORM_ID_START + index, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
        gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, PARTICLE_TRANSFORM_ID_START + index, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
    }

    // @recomp Clear this particle's reset state.
    particle_reset_list[index] = false;
    
    if (entry->draw != NULL) {
        entry->draw(play, index, entry);
    }
    
    gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);
    gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);

    CLOSE_DISPS(play->state.gfxCtx);
}

extern u64 gSun1Tex[];
extern Gfx gSunSparkleMaterialDL[];
extern Gfx gSunSparkleModelDL[];

extern u8 D_80B23C40[];
extern u8 D_80B23C2C[];

// @recomp Modified to tag firework transforms.
void func_80B22FA8_patched(Actor* thisx, EnHanabiStruct* arg0, PlayState* play2) {
    PlayState* play = play2;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    s32 i;
    u8 sp53;

    OPEN_DISPS(gfxCtx);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    POLY_XLU_DISP = Gfx_SetupDL(POLY_XLU_DISP, SETUPDL_20);

    gSPSegment(POLY_XLU_DISP++, 0x08, Lib_SegmentedToVirtual(gSun1Tex));
    gSPDisplayList(POLY_XLU_DISP++, gSunSparkleMaterialDL);

    sp53 = 0xFF;

    for (i = 0; i < 400; i++, arg0++) {
        if (arg0->unk_00 != 1) {
            continue;
        }

        Matrix_Translate(arg0->unk_08.x, arg0->unk_08.y, arg0->unk_08.z, MTXMODE_NEW);
        Matrix_ReplaceRotation(&play->billboardMtxF);
        if (arg0->unk_01 < 40) {
            Matrix_Scale(arg0->unk_04 * 0.025f * arg0->unk_01, arg0->unk_04 * 0.025f * arg0->unk_01, 1.0f,
                         MTXMODE_APPLY);
        } else {
            Matrix_Scale(arg0->unk_04, arg0->unk_04, 1.0f, MTXMODE_APPLY);
        }
        Matrix_RotateZS(play->gameplayFrames * 4864, MTXMODE_APPLY);

        // @recomp Tag the matrix.
        gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, actor_transform_id(thisx) + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);

        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

        if (sp53 != arg0->unk_02) {
            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetEnvColor(POLY_XLU_DISP++, D_80B23C40[arg0->unk_02], D_80B23C40[arg0->unk_02 + 1],
                           D_80B23C40[arg0->unk_02 + 2], 255);

            sp53 = arg0->unk_02;
        }

        if (arg0->unk_01 < 6) {
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, D_80B23C2C[arg0->unk_02], D_80B23C2C[arg0->unk_02 + 1],
                            D_80B23C2C[arg0->unk_02 + 2], arg0->unk_01 * 50);
        } else {
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, D_80B23C2C[arg0->unk_02], D_80B23C2C[arg0->unk_02 + 1],
                            D_80B23C2C[arg0->unk_02 + 2], 255);
        }

        gSPDisplayList(POLY_XLU_DISP++, gSunSparkleModelDL);
        // @recomp Pop the matrix group.
        gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
    }

    CLOSE_DISPS(gfxCtx);
}

// @recomp Patched to call a custom version of a vanilla function.
RECOMP_PATCH void EnHanabi_Draw(Actor* thisx, PlayState* play) {
    EnHanabi* this = (EnHanabi*)thisx;

    Matrix_Push();
    // @recomp Call a modified version of the function that takes the actor for tagging purposes.
    func_80B22FA8_patched(thisx, this->unk_148, play);
    Matrix_Pop();
}

Vec3f kankyo_prev_pos_base[DEMOKANKYO_EFFECT_COUNT] = {0};

// @recomp Patched to draw the lost woods particles outside the 4:3 region and tag their transforms.
RECOMP_PATCH void DemoKakyo_DrawLostWoodsSparkle(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    DemoKankyo* this = (DemoKankyo*)thisx;
    s16 i;
    f32 scaleAlpha;
    Vec3f worldPos;
    Vec3f screenPos;

    // if not underwater
    if (!(play->cameraPtrs[CAM_ID_MAIN]->stateFlags & CAM_STATE_UNDERWATER)) {
        OPEN_DISPS(play->state.gfxCtx);

        POLY_XLU_DISP = Gfx_SetupDL(POLY_XLU_DISP, SETUPDL_20);

        gSPSegment(POLY_XLU_DISP++, 0x08, Lib_SegmentedToVirtual(gSun1Tex));
        gSPDisplayList(POLY_XLU_DISP++, gSunSparkleMaterialDL);

        for (i = 0; i < play->envCtx.precipitation[PRECIP_SNOW_MAX]; i++) {
            worldPos.x = this->effects[i].posBase.x + this->effects[i].posOffset.x;
            worldPos.y = this->effects[i].posBase.y + this->effects[i].posOffset.y;
            worldPos.z = this->effects[i].posBase.z + this->effects[i].posOffset.z;

            // @recomp Render the particle regardless of its position on screen.
            // Play_GetScreenPos(play, &worldPos, &screenPos);

            // // checking if particle is on screen
            // if ((screenPos.x >= 0.0f) && (screenPos.x < SCREEN_WIDTH) && (screenPos.y >= 0.0f) &&
            //     (screenPos.y < SCREEN_HEIGHT)) {
            if (true) {
                Matrix_Translate(worldPos.x, worldPos.y, worldPos.z, MTXMODE_NEW);
                scaleAlpha = this->effects[i].alpha / 50.0f;
                if (scaleAlpha > 1.0f) {
                    scaleAlpha = 1.0f;
                }

                Matrix_Scale(this->effects[i].scale * scaleAlpha, this->effects[i].scale * scaleAlpha,
                             this->effects[i].scale * scaleAlpha, MTXMODE_APPLY);

                // adjust transparency of this particle
                if (i < 32) {
                    // Skyfish particles
                    if (this->effects[i].state != DEMO_KANKYO_STATE_SKYFISH) {
                        // still initializing
                        if (this->effects[i].alpha > 0) { // NOT DECR
                            this->effects[i].alpha--;
                        }
                    } else if (this->effects[i].alpha < 100) {
                        this->effects[i].alpha++;
                    }
                } else if (this->effects[i].state != DEMO_KANKYO_STATE_SKYFISH) {
                    if ((this->effects[i].alphaClock & 31) < 16) {
                        if (this->effects[i].alpha < 235) {
                            this->effects[i].alpha += 20;
                        }
                    } else if (this->effects[i].alpha > 20) {
                        this->effects[i].alpha -= 20;
                    }
                } else if ((this->effects[i].alphaClock & 15) < 8) {
                    if (this->effects[i].alpha < 255) {
                        this->effects[i].alpha += 100;
                    }
                } else if (this->effects[i].alpha > 10) {
                    this->effects[i].alpha -= 10;
                }

                gDPPipeSync(POLY_XLU_DISP++);

                switch (i & 1) {
                    case 0: // gold particles
                        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 155, this->effects[i].alpha);
                        gDPSetEnvColor(POLY_XLU_DISP++, 250, 180, 0, this->effects[i].alpha);
                        break;

                    case 1: // silver particles
                        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, this->effects[i].alpha);
                        gDPSetEnvColor(POLY_XLU_DISP++, 0, 100, 255, this->effects[i].alpha);
                        break;
                }

                Matrix_Mult(&play->billboardMtxF, MTXMODE_APPLY);
                Matrix_RotateZF(DEG_TO_RAD(play->state.frames * 20.0f), MTXMODE_APPLY);

                // @recomp Tag the particle's matrix. Skip if the particle's base position changed.
                if (this->effects[i].state == DEMO_KANKYO_STATE_SKYFISH || (
                        kankyo_prev_pos_base[i].x == this->effects[i].posBase.x && 
                        kankyo_prev_pos_base[i].y == this->effects[i].posBase.y && 
                        kankyo_prev_pos_base[i].z == this->effects[i].posBase.z)) {
                    gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, actor_transform_id(thisx) + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
                }
                else {
                    gEXMatrixGroupDecomposedSkipAll(POLY_XLU_DISP++, actor_transform_id(thisx) + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
                }
                gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx),
                          G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                gSPDisplayList(POLY_XLU_DISP++, gSunSparkleModelDL);
                // @recomp Pop the matrix tag and record the particle's current base position.
                gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
                kankyo_prev_pos_base[i] = this->effects[i].posBase;
            }
        }

        CLOSE_DISPS(play->state.gfxCtx);
    }
}

extern Gfx gLightOrbMaterial1DL[];
extern Gfx gBubbleDL[];
extern Gfx gLightOrbModelDL[];

// @recomp Patched to draw the lost woods particles outside the 4:3 region and tag their transforms.
RECOMP_PATCH void DemoKankyo_DrawMoonAndGiant(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    DemoKankyo* this = (DemoKankyo*)thisx;
    s16 i;
    f32 alphaScale;

    if (this->isSafeToDrawGiants != false) {
        Vec3f worldPos;
        Vec3f screenPos;
        s32 pad;
        GraphicsContext* gfxCtx = play->state.gfxCtx;

        OPEN_DISPS(gfxCtx);

        Gfx_SetupDL25_Xlu(gfxCtx);

        for (i = 0; i < play->envCtx.precipitation[PRECIP_SNOW_MAX]; i++) {
            worldPos.x = this->effects[i].posBase.x + this->effects[i].posOffset.x;
            worldPos.y = this->effects[i].posBase.y + this->effects[i].posOffset.y;
            worldPos.z = this->effects[i].posBase.z + this->effects[i].posOffset.z;

            // @recomp Render the particle regardless of its position on screen.
            // Play_GetScreenPos(play, &worldPos, &screenPos);

            // checking if effect is on screen
            // if ((screenPos.x >= 0.0f) && (screenPos.x < SCREEN_WIDTH) && (screenPos.y >= 0.0f) &&
            //     (screenPos.y < SCREEN_HEIGHT)) {
            if (true) {
                Matrix_Translate(worldPos.x, worldPos.y, worldPos.z, MTXMODE_NEW);
                alphaScale = this->effects[i].alpha / 50.0f;
                if (alphaScale > 1.0f) {
                    alphaScale = 1.0f;
                }
                Matrix_Scale(this->effects[i].scale * alphaScale, this->effects[i].scale * alphaScale,
                             this->effects[i].scale * alphaScale, MTXMODE_APPLY);
                alphaScale = Math_Vec3f_DistXYZ(&worldPos, &play->view.eye) / 300.0f;
                alphaScale = CLAMP(1.0f - alphaScale, 0.0f, 1.0f);

                if (this->actor.params == DEMO_KANKYO_TYPE_GIANTS) {
                    this->effects[i].alpha = 255.0f * alphaScale;
                } else {
                    this->effects[i].alpha = 160.0f * alphaScale;
                }

                gDPPipeSync(POLY_XLU_DISP++);

                switch (i & 1) { // half/half slightly different shades of yellow/tan
                    case 0:
                        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 230, 230, 220, this->effects[i].alpha);
                        gDPSetEnvColor(POLY_XLU_DISP++, 230, 230, 30, this->effects[i].alpha);
                        break;

                    case 1:
                        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 200, 200, 190, this->effects[i].alpha);
                        gDPSetEnvColor(POLY_XLU_DISP++, 200, 200, 30, this->effects[i].alpha);
                        break;
                }

                gSPDisplayList(POLY_XLU_DISP++, gLightOrbMaterial1DL);

                Matrix_Mult(&play->billboardMtxF, MTXMODE_APPLY);

                Matrix_RotateZF(DEG_TO_RAD(play->state.frames * 20.0f), MTXMODE_APPLY);

                gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx),
                          G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

                // @recomp Tag the particle's matrix. Skip if the particle's base position changed.
                // Allow the Y base position to change if this is a moon particle.
                if (kankyo_prev_pos_base[i].x == this->effects[i].posBase.x && 
                    (kankyo_prev_pos_base[i].y == this->effects[i].posBase.y || this->actor.params == DEMO_KANKYO_TYPE_MOON) && 
                    kankyo_prev_pos_base[i].z == this->effects[i].posBase.z) {
                    gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, actor_transform_id(thisx) + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
                }
                else {
                    gEXMatrixGroupDecomposedSkipAll(POLY_XLU_DISP++, actor_transform_id(thisx) + i, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
                }

                if (this->actor.params == DEMO_KANKYO_TYPE_GIANTS) {
                    gSPDisplayList(POLY_XLU_DISP++, gBubbleDL);
                } else {
                    gSPDisplayList(POLY_XLU_DISP++, gLightOrbModelDL);
                }

                // @recomp Pop the matrix tag and record the particle's current base position.
                gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
                kankyo_prev_pos_base[i] = this->effects[i].posBase;
            }
        }

        CLOSE_DISPS(gfxCtx);
    }
}

static Vec3f D_80A5AFB0 = { 0.0f, 0.0f, 0.0f };

// The byte after unk_01 in EnWaterEffectStruct is unused, so we'll use it as a respawn flag.
#define WATER_EFFECT_RESPAWNED(ptr) (&(ptr)->unk_01)[1]

// @recomp Mark respawned water effect particles so they can be skipped for the first frame.
RECOMP_PATCH void func_80A599E8(EnWaterEffect* this, Vec3f* arg1, u8 arg2) {
    s16 i;
    EnWaterEffectStruct* ptr = &this->unk_144[0];

    for (i = 0; i < ARRAY_COUNT(this->unk_144); i++, ptr++) {
        if (ptr->unk_00 == 0) {
            ptr->unk_00 = 4;
            // @recomp Use an unused byte to mark that this particle was respawned.
            WATER_EFFECT_RESPAWNED(ptr) = true;
            ptr->unk_04 = *arg1;

            ptr->unk_1C = D_80A5AFB0;
            ptr->unk_10 = D_80A5AFB0;

            if ((arg2 == 0) || (arg2 == 2)) {
                ptr->unk_1C.y = -1.0f;
            }

            if (arg2 >= 2) {
                if (arg2 == 2) {
                    ptr->unk_10.x = Rand_CenteredFloat(10.0f);
                    ptr->unk_10.y = Rand_ZeroFloat(3.0f) + 5.0f;
                    ptr->unk_10.z = Rand_CenteredFloat(10.0f);
                }
                ptr->unk_2C.z = 0.0017f;
                ptr->unk_2C.x = 0.003f;
                ptr->unk_2C.y = 0.0018f;
            } else {
                ptr->unk_2C.z = 0.003f;
                ptr->unk_2C.x = 0.003f;
                ptr->unk_2C.y = 0.003f;
            }
            ptr->unk_38 = 255.0f;
            ptr->unk_28 = Rand_ZeroFloat(0x10000);
            ptr->unk_3C = 255;
            ptr->unk_01 = Rand_ZeroFloat(200.0f);
            ptr->unk_2A = arg2;
            break;
        }
    }
}

extern Gfx object_water_effect_DL_004340[];
extern Gfx object_water_effect_DL_0043E8[];
extern Gfx gameplay_keep_DL_06AB30[];

// @recomp Tag the transforms for falling fire rocks.
RECOMP_PATCH void func_80A5A184(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    EnWaterEffect* this = (EnWaterEffect*)thisx;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    EnWaterEffectStruct* ptr = &this->unk_144[0];
    u8 flag = false;
    s16 i;

    OPEN_DISPS(gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    for (i = 0; i < ARRAY_COUNT(this->unk_144); i++, ptr++) {
        if (ptr->unk_00 == 4) {
            if (!flag) {
                gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_004340);
                gDPSetEnvColor(POLY_XLU_DISP++, 255, 10, 0, 0);
                POLY_OPA_DISP = Gfx_SetFog(POLY_OPA_DISP, 255, 0, 0, 255, 500, 3600);
                flag++;
            }

            gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, (u8)ptr->unk_38, 0, 0, (u8)ptr->unk_3C);
            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScroll(play->state.gfxCtx, 0, 0, 0, 0x20, 0x40, 1, 0, (ptr->unk_01 * -20) & 0x1FF,
                                        0x20, 0x80));

            Matrix_Translate(ptr->unk_04.x, ptr->unk_04.y, ptr->unk_04.z, MTXMODE_NEW);

            if (ptr->unk_2A >= 2) {
                Matrix_ReplaceRotation(&play->billboardMtxF);
            } else {
                Matrix_RotateYS(Camera_GetInputDirYaw(GET_ACTIVE_CAM(play)), MTXMODE_APPLY);
            }

            Matrix_Scale(ptr->unk_2C.x, ptr->unk_2C.y, 1.0f, MTXMODE_APPLY);

            if ((i & 1) != 0) {
                Matrix_RotateYF(M_PI, MTXMODE_APPLY);
            }

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

            // @recomp Tag the matrix group. Skip all interpolation if this particle just respawned. Otherwise,
            // skip rotation interpolation if there was a camera cut since this is a billboard.
            if (WATER_EFFECT_RESPAWNED(ptr)) {
                gEXMatrixGroupDecomposedSkipAll(POLY_XLU_DISP++, actor_transform_id(thisx) + (i * 2) + 0, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
            }
            else if (camera_was_skipped()) {
                gEXMatrixGroupDecomposedSkipRot(POLY_XLU_DISP++, actor_transform_id(thisx) + (i * 2) + 0, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
            }
            else {
                gEXMatrixGroupDecomposedNormal(POLY_XLU_DISP++, actor_transform_id(thisx) + (i * 2) + 0, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
            }

            gSPDisplayList(POLY_XLU_DISP++, object_water_effect_DL_0043E8);

            // @recomp Pop the matrix group.
            gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);

            if ((ptr->unk_2A & 1) == 0) {
                Matrix_Translate(ptr->unk_04.x, ptr->unk_04.y + 5.0f, ptr->unk_04.z, MTXMODE_NEW);
                Matrix_RotateXS(ptr->unk_28, MTXMODE_APPLY);
                Matrix_Scale(ptr->unk_2C.z, ptr->unk_2C.z, ptr->unk_2C.z, MTXMODE_APPLY);

                gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx),
                          G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

                // @recomp Tag the matrix group. Skip all interpolation if the base particle just respawned.
                if (WATER_EFFECT_RESPAWNED(ptr)) {
                    gEXMatrixGroupDecomposedSkipAll(POLY_OPA_DISP++, actor_transform_id(thisx) + (i * 2) + 1, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
                }
                else {
                    gEXMatrixGroupDecomposedNormal(POLY_OPA_DISP++, actor_transform_id(thisx) + (i * 2) + 1, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_ALLOW);
                }

                gSPDisplayList(POLY_OPA_DISP++, gameplay_keep_DL_06AB30);

                // @recomp Pop the matrix group.
                gEXPopMatrixGroup(POLY_OPA_DISP++, G_MTX_MODELVIEW);
            }
            
            // @recomp Clear the respawned state of this particle.
            WATER_EFFECT_RESPAWNED(ptr) = false;
        }
    }

    POLY_OPA_DISP = Play_SetFog(play, POLY_OPA_DISP);

    CLOSE_DISPS(gfxCtx);
}
