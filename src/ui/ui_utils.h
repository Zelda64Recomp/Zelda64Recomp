#ifndef __UI_UTILS_H__
#define __UI_UTILS_H__

#include "elements/ui_types.h"

namespace recompui {
    Color lerp_color(const Color& a, const Color& b, float factor);
    Color get_pulse_color(uint32_t millisecond_period);
}

#endif
