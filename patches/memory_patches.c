#include "patches.h"

// @recomp Leave the entire KSEG0 range unmodified when translating to a virtual address. This will allow
// using the entirety of the extended RAM address space for custom assets. 
RECOMP_PATCH void* Lib_SegmentedToVirtual(void* ptr) {
    if (IS_KSEG0(ptr)) {
        return ptr;
    }
    else {
        return SEGMENTED_TO_K0(ptr);
    }
}
