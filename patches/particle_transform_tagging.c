#include "patches.h"
#include "transform_ids.h"

extern EffectSsInfo sEffectSsInfo;

// @recomp Add transform tags to particles
void EffectSS_DrawParticle(PlayState* play, s32 index) {
    EffectSs* entry = &sEffectSsInfo.dataTable[index];

    OPEN_DISPS(play->state.gfxCtx);

    gEXMatrixGroupDecomposed(POLY_OPA_DISP++, PARTICLE_TRANSFORM_ID_START + index, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);
    gEXMatrixGroupDecomposed(POLY_XLU_DISP++, PARTICLE_TRANSFORM_ID_START + index, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR);
    
    if (entry->draw != NULL) {
        entry->draw(play, index, entry);
    }
    
    gEXPopMatrixGroup(POLY_OPA_DISP++);
    gEXPopMatrixGroup(POLY_XLU_DISP++);

    CLOSE_DISPS(play->state.gfxCtx);
}
