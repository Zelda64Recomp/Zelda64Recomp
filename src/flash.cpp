#include <array>
#include <cassert>
#include "../portultra/ultra64.h"
#include "../portultra/multilibultra.hpp"
#include "recomp.h"

constexpr uint32_t flash_size = 1024 * 1024 / 8; // 1Mbit
constexpr uint32_t page_size = 128;
constexpr uint32_t pages_per_sector = 128;
constexpr uint32_t sector_size = page_size * pages_per_sector;
constexpr uint32_t sector_count = flash_size / sector_size;

void save_write_ptr(const void* in, uint32_t offset, uint32_t count);
void save_write(uint8_t* rdram, gpr rdram_address, uint32_t offset, uint32_t count);
void save_read(uint8_t* rdram, gpr rdram_address, uint32_t offset, uint32_t count);
void save_clear(uint32_t start, uint32_t size);

std::array<char, page_size> write_buffer;

extern "C" void osFlashInit_recomp(uint8_t * rdram, recomp_context * ctx) {
	ctx->r2 = Multilibultra::flash_handle;
}

extern "C" void osFlashReadStatus_recomp(uint8_t * rdram, recomp_context * ctx) {
	PTR(u8) flash_status = ctx->r4;

	MEM_B(0, flash_status) = 0;
}

extern "C" void osFlashReadId_recomp(uint8_t * rdram, recomp_context * ctx) {
	PTR(u32) flash_type = ctx->r4;
	PTR(u32) flash_maker = ctx->r5;

	MEM_B(0, flash_type) = 0;
	MEM_B(0, flash_maker) = 0;
}

extern "C" void osFlashClearStatus_recomp(uint8_t * rdram, recomp_context * ctx) {

}

extern "C" void osFlashAllErase_recomp(uint8_t * rdram, recomp_context * ctx) {
	save_clear(0, Multilibultra::save_size);

	ctx->r2 = 0;
}

extern "C" void osFlashAllEraseThrough_recomp(uint8_t * rdram, recomp_context * ctx) {
	save_clear(0, Multilibultra::save_size);

	ctx->r2 = 0;
}

extern "C" void osFlashSectorErase_recomp(uint8_t * rdram, recomp_context * ctx) {
	uint32_t page_num = (uint32_t)ctx->r4;
	uint32_t sector_num = page_num / sector_size;

	// Prevent out of bounds erase
	if (sector_num >= sector_size) {
		ctx->r2 = -1;
		return;
	}

	save_clear(sector_num * sector_size, sector_size);

	ctx->r2 = 0;
}

extern "C" void osFlashSectorEraseThrough_recomp(uint8_t * rdram, recomp_context * ctx) {
	uint32_t page_num = (uint32_t)ctx->r4;
	uint32_t sector_num = page_num / sector_size;

	// Prevent out of bounds erase
	if (sector_num >= sector_size) {
		ctx->r2 = -1;
		return;
	}

	save_clear(sector_num * sector_size, sector_size);

	ctx->r2 = 0;
}

extern "C" void osFlashCheckEraseEnd_recomp(uint8_t * rdram, recomp_context * ctx) {
	// All erases are blocking in this implementation, so this should always return OK.
	ctx->r2 = 0; // FLASH_STATUS_ERASE_OK
}

extern "C" void osFlashWriteBuffer_recomp(uint8_t * rdram, recomp_context * ctx) {
	OSIoMesg* mb = TO_PTR(OSIoMesg, ctx->r4);
	int32_t pri = ctx->r5;
	PTR(void) dramAddr = ctx->r6;
	PTR(OSMesgQueue) mq = ctx->r7;
	
	// Copy the input data into the write buffer
	for (size_t i = 0; i < page_size; i++) {
		write_buffer[i] = MEM_B(i, dramAddr);
	}

	// Send the message indicating write completion
	osSendMesg(PASS_RDRAM mq, 0, OS_MESG_NOBLOCK);

	ctx->r2 = 0;
}

extern "C" void osFlashWriteArray_recomp(uint8_t * rdram, recomp_context * ctx) {
	uint32_t page_num = ctx->r4;

	// Copy the write buffer into the save file
	save_write_ptr(write_buffer.data(), page_num * page_size, page_size);

	ctx->r2 = 0;
}

extern "C" void osFlashReadArray_recomp(uint8_t * rdram, recomp_context * ctx) {
	OSIoMesg* mb = TO_PTR(OSIoMesg, ctx->r4);
	int32_t pri = ctx->r5;
	uint32_t page_num = ctx->r6;
	PTR(void) dramAddr = ctx->r7;
	uint32_t n_pages = MEM_W(0x10, ctx->r29);
	PTR(OSMesgQueue) mq = MEM_W(0x14, ctx->r29);

	uint32_t offset = page_num * page_size;
	uint32_t count = n_pages * page_size;

	// Read from the save file into the provided buffer
	save_read(PASS_RDRAM dramAddr, offset, count);

	// Send the message indicating read completion
	osSendMesg(PASS_RDRAM mq, 0, OS_MESG_NOBLOCK);

	ctx->r2 = 0;
}

extern "C" void osFlashChange_recomp(uint8_t * rdram, recomp_context * ctx) {
	assert(false);
}
