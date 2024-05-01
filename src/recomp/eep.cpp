#include "recomp.h"
#include "../ultramodern/ultra64.h"

void save_write(RDRAM_ARG PTR(void) rdram_address, uint32_t offset, uint32_t count);
void save_read(RDRAM_ARG PTR(void) rdram_address, uint32_t offset, uint32_t count);

constexpr int eeprom_block_size = 8;
constexpr int eep4_size = 4096;
constexpr int eep4_block_count = eep4_size / eeprom_block_size;
constexpr int eep16_size = 16384;
constexpr int eep16_block_count = eep16_size / eeprom_block_size;

extern "C" void osEepromProbe_recomp(uint8_t* rdram, recomp_context* ctx) {
    ctx->r2 = 0x02; // EEP16K
}

extern "C" void osEepromWrite_recomp(uint8_t* rdram, recomp_context* ctx) {
    assert(false);// ctx->r2 = 8; // CONT_NO_RESPONSE_ERROR
}

extern "C" void osEepromLongWrite_recomp(uint8_t* rdram, recomp_context* ctx) {
    uint8_t eep_address = ctx->r5;
    gpr buffer = ctx->r6;
    int32_t nbytes = ctx->r7;

    assert(!(nbytes & 7));
    assert(eep_address * eeprom_block_size + nbytes <= eep16_size);

    save_write(rdram, buffer, eep_address * eeprom_block_size, nbytes);

    ctx->r2 = 0;
}

extern "C" void osEepromRead_recomp(uint8_t* rdram, recomp_context* ctx) {
    assert(false);// ctx->r2 = 8; // CONT_NO_RESPONSE_ERROR
}

extern "C" void osEepromLongRead_recomp(uint8_t* rdram, recomp_context* ctx) {
    uint8_t eep_address = ctx->r5;
    gpr buffer = ctx->r6;
    int32_t nbytes = ctx->r7;

    assert(!(nbytes & 7));
    assert(eep_address * eeprom_block_size + nbytes <= eep16_size);

    save_read(rdram, buffer, eep_address * eeprom_block_size, nbytes);

    ctx->r2 = 0;
}
