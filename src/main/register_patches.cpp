#include "ovl_patches.hpp"
#include "../../RecompiledPatches/patches_bin.h"
#include "../../RecompiledPatches/recomp_overlays.inl"

#include "librecomp/overlays.hpp"
#include "librecomp/game.hpp"

void zelda64::register_patches() {
    // TODO: This is a bit awful, maybe provide only one functions that does both in librecomp?
    recomp::register_patch(mm_patches_bin, sizeof(mm_patches_bin));
    recomp::register_patch_section(section_table);
}
