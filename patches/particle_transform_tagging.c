#include "patches.h"
#include "transform_ids.h"

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
        gEXMatrixGroupDecomposedSkip(POLY_OPA_DISP++, PARTICLE_TRANSFORM_ID_START + index, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
        gEXMatrixGroupDecomposedSkip(POLY_XLU_DISP++, PARTICLE_TRANSFORM_ID_START + index, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
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
