#include <cassert>
#include <cstring>
#include <fstream>

#include "zelda_game.h"

void naive_copy(std::span<uint8_t> dst, std::span<const uint8_t> src) {
    for (size_t i = 0; i < src.size(); i++) {
        dst[i] = src[i];
    }
}

void yaz0_decompress(std::span<const uint8_t> input, std::span<uint8_t> output) {
    int32_t layoutBitIndex;
    uint8_t layoutBits;

    size_t input_pos = 0;
    size_t output_pos = 0;

    size_t input_size = input.size();
    size_t output_size = output.size();

    while (input_pos < input_size) {
        int32_t layoutBitIndex = 0;
        uint8_t layoutBits = input[input_pos++];

        while (layoutBitIndex < 8 && input_pos < input_size && output_pos < output_size) {
            if (layoutBits & 0x80) {
                output[output_pos++] = input[input_pos++];
            } else {
                int32_t firstByte = input[input_pos++];
                int32_t secondByte = input[input_pos++];
                uint32_t bytes = firstByte << 8 | secondByte;
                uint32_t offset = (bytes & 0x0FFF) + 1;
                uint32_t length;

                // Check how the group length is encoded
                if ((firstByte & 0xF0) == 0) {
                    // 3 byte encoding, 0RRRNN
                    int32_t thirdByte = input[input_pos++];
                    length = thirdByte + 0x12;
                } else {
                    // 2 byte encoding, NRRR
                    length = ((bytes & 0xF000) >> 12) + 2;
                }

                naive_copy(output.subspan(output_pos, length), output.subspan(output_pos - offset, length));
                output_pos += length;
            }

            layoutBitIndex++;
            layoutBits <<= 1;
        }
    }
}

#ifdef _MSC_VER
inline uint32_t byteswap(uint32_t val) {
    return _byteswap_ulong(val);
}
#else
constexpr uint32_t byteswap(uint32_t val) {
    return __builtin_bswap32(val);
}
#endif

// Produces a decompressed MM rom. This is only needed because the game has compressed code.
// For other recomps using this repo as an example, you can omit the decompression routine and
// set the corresponding fields in the GameEntry if the game doesn't have compressed code,
// even if it does have compressed data.
std::vector<uint8_t> zelda64::decompress_mm(std::span<const uint8_t> compressed_rom) {
    // Sanity check the rom size and header. These should already be correct from the runtime's check,
    // but it should prevent this file from accidentally being copied to another recomp.
    if (compressed_rom.size() != 0x2000000) {
        assert(false);
        return {};
    }

    if (compressed_rom[0x3B] != 'N' || compressed_rom[0x3C] != 'Z' || compressed_rom[0x3D] != 'S' || compressed_rom[0x3E] != 'E') {
        assert(false);
        return {};
    }

    struct DmaDataEntry {
        uint32_t vrom_start;
        uint32_t vrom_end;
        uint32_t rom_start;
        uint32_t rom_end;

        void bswap() {
            vrom_start = byteswap(vrom_start);
            vrom_end = byteswap(vrom_end);
            rom_start = byteswap(rom_start);
            rom_end = byteswap(rom_end);
        }
    };

    DmaDataEntry cur_entry{};
    size_t cur_entry_index = 0;

    constexpr size_t dma_data_rom_addr = 0x1A500;

    std::vector<uint8_t> ret{};
    ret.resize(0x2F00000);

    size_t content_end = 0;

    do {
        // Read the entry from the compressed rom.
        size_t cur_entry_rom_address = dma_data_rom_addr + (cur_entry_index++) * sizeof(DmaDataEntry);
        memcpy(&cur_entry, compressed_rom.data() + cur_entry_rom_address, sizeof(DmaDataEntry));
        // Swap the entry to native endianness after reading from the big endian data.
        cur_entry.bswap();

        // Rom end being 0 means the data is already uncompressed, so copy it as-is to vrom start.
        size_t entry_decompressed_size = cur_entry.vrom_end - cur_entry.vrom_start;
        if (cur_entry.rom_end == 0) {
            memcpy(ret.data() + cur_entry.vrom_start, compressed_rom.data() + cur_entry.rom_start, entry_decompressed_size);

            // Edit the entry to account for it being in a new location now.
            cur_entry.rom_start = cur_entry.vrom_start;
        }
        // Otherwise, decompress the input data into the output data.
        else {
            if (cur_entry.rom_end != cur_entry.rom_start) {
                // Validate the presence of the yaz0 header.
                if (compressed_rom[cur_entry.rom_start + 0] != 'Y' ||
                    compressed_rom[cur_entry.rom_start + 1] != 'a' ||
                    compressed_rom[cur_entry.rom_start + 2] != 'z' ||
                    compressed_rom[cur_entry.rom_start + 3] != '0')
                {
                    assert(false);
                    return {};
                }
                // Skip the yaz0 header.
                size_t compressed_data_rom_start = cur_entry.rom_start + 0x10;
                size_t entry_compressed_size = cur_entry.rom_end - compressed_data_rom_start;

                std::span input_span = std::span{ compressed_rom }.subspan(compressed_data_rom_start, entry_compressed_size);
                std::span output_span = std::span{ ret }.subspan(cur_entry.vrom_start, entry_decompressed_size);
                yaz0_decompress(input_span, output_span);

                // Edit the entry to account for it being decompressed now.
                cur_entry.rom_start = cur_entry.vrom_start;
                cur_entry.rom_end = 0;
            }
        }

        if (entry_decompressed_size != 0) {
            if (cur_entry.vrom_end > content_end) {
                content_end = cur_entry.vrom_end;
            }
        }

        // Swap the entry back to big endian for writing.
        cur_entry.bswap();
        // Write the modified entry to the decompressed rom.
        memcpy(ret.data() + cur_entry_rom_address, &cur_entry, sizeof(DmaDataEntry));
    } while (cur_entry.vrom_end != 0);

    // Align the start of padding to the closest 0x1000 (matches decomp rom decompression behavior).
    content_end = (content_end + 0x1000 - 1) & -0x1000;

    // Write 0xFF as the padding.
    std::fill(ret.begin() + content_end, ret.end(), 0xFF);

    return ret;
}
