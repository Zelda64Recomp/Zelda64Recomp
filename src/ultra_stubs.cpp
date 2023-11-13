#include "../ultramodern/ultra64.h"
#include "../ultramodern/ultramodern.hpp"
#include "recomp.h"

// None of these functions need to be reimplemented, so stub them out
extern "C" void osUnmapTLBAll_recomp(uint8_t * rdram, recomp_context * ctx) {
    // TODO this will need to be implemented in the future for any games that actually use the TLB
}

extern "C" void osVoiceInit_recomp(uint8_t * rdram, recomp_context * ctx) {
    ctx->r2 = 11; // CONT_ERR_DEVICE
}

extern "C" void osVoiceSetWord_recomp(uint8_t * rdram, recomp_context * ctx) {
    assert(false);
}

extern "C" void osVoiceCheckWord_recomp(uint8_t * rdram, recomp_context * ctx) {
    assert(false);
}

extern "C" void osVoiceStopReadData_recomp(uint8_t * rdram, recomp_context * ctx) {
    assert(false);
}

extern "C" void osVoiceMaskDictionary_recomp(uint8_t * rdram, recomp_context * ctx) {
    assert(false);
}

extern "C" void osVoiceStartReadData_recomp(uint8_t * rdram, recomp_context * ctx) {
    assert(false);
}

extern "C" void osVoiceControlGain_recomp(uint8_t * rdram, recomp_context * ctx) {
    assert(false);
}

extern "C" void osVoiceGetReadData_recomp(uint8_t * rdram, recomp_context * ctx) {
    assert(false);
}

extern "C" void osVoiceClearDictionary_recomp(uint8_t * rdram, recomp_context * ctx) {
    assert(false);
}

