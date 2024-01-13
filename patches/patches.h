#ifndef __PATCHES_H__
#define __PATCHES_H__

// TODO fix renaming symbols in patch recompilation
#define osRecvMesg osRecvMesg_recomp
#define osSendMesg osSendMesg_recomp
#include "global.h"
#include "rt64_extended_gbi.h"

int recomp_printf(const char* fmt, ...);

#endif
