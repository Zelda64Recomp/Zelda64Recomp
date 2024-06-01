#include "ovl_patches.hpp"
#include "../../RecompiledFuncs/recomp_overlays.inl"

#include "recomp_overlays.h"

void zelda64::register_overlays() {
    const static recomp::overlay_section_table_data_t sections {
        .code_sections = section_table,
        .num_code_sections = ARRLEN(section_table),
        .total_num_sections = num_sections,
    };

    const static recomp::overlays_by_index_t overlays {
        .table = overlay_sections_by_index,
        .len = ARRLEN(overlay_sections_by_index),
    };

    recomp::register_overlays(sections, overlays);
}
