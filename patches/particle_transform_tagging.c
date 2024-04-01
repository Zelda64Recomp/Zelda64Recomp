#include "patches.h"
#include "transform_ids.h"
#include "overlays/actors/ovl_En_Hanabi/z_en_hanabi.h"

extern EffectSsInfo sEffectSsInfo;

#define MAX_PARTICLES 256
u8 particle_reset_list[MAX_PARTICLES];

// @recomp Patched to track that the particle has been reset.
void EffectSS_ResetEntry(EffectSs* particle) {
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
void EffectSS_Init(PlayState* play, s32 numEntries) {
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
void EffectSS_DrawParticle(PlayState* play, s32 index) {
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

// @recomp Modified to take the actor as an argument for relocation and to tag firework transforms.
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

    // @recomp Manually relocate, TODO remove when automated by recompiler.
    u8* D_80B23C40_relocated = (u8*)actor_relocate(thisx, D_80B23C40);
    u8* D_80B23C2C_relocated = (u8*)actor_relocate(thisx, D_80B23C2C);

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
            gDPSetEnvColor(POLY_XLU_DISP++, D_80B23C40_relocated[arg0->unk_02], D_80B23C40_relocated[arg0->unk_02 + 1],
                           D_80B23C40_relocated[arg0->unk_02 + 2], 255);

            sp53 = arg0->unk_02;
        }

        if (arg0->unk_01 < 6) {
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, D_80B23C2C_relocated[arg0->unk_02], D_80B23C2C_relocated[arg0->unk_02 + 1],
                            D_80B23C2C_relocated[arg0->unk_02 + 2], arg0->unk_01 * 50);
        } else {
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, D_80B23C2C_relocated[arg0->unk_02], D_80B23C2C_relocated[arg0->unk_02 + 1],
                            D_80B23C2C_relocated[arg0->unk_02 + 2], 255);
        }

        gSPDisplayList(POLY_XLU_DISP++, gSunSparkleModelDL);
        // @recomp Pop the matrix group.
        gEXPopMatrixGroup(POLY_XLU_DISP++, G_MTX_MODELVIEW);
    }

    CLOSE_DISPS(gfxCtx);
}

// @recomp Patched to call a custom version of a vanilla function.
void EnHanabi_Draw(Actor* thisx, PlayState* play) {
    EnHanabi* this = (EnHanabi*)thisx;

    Matrix_Push();
    // @recomp Call a modified version of the function that takes the actor for relocation purposes.
    func_80B22FA8_patched(thisx, this->unk_148, play);
    Matrix_Pop();
}
