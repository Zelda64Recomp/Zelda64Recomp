#include <unordered_map>
#include <algorithm>
#include <vector>
#include "recomp.h"
#include "../../RecompiledPatches/recomp_overlays.inl"

void load_special_overlay(const SectionTableEntry& section, int32_t ram);

void load_patch_functions() {
	load_special_overlay(section_table[0], section_table[0].ram_addr);
}
