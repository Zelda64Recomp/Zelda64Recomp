#ifndef __ZELDA_LAUNCHER_H__
#define __ZELDA_LAUNCHER_H__

#include "recompui/recompui.h"

namespace zelda64 {
    void on_launcher_init(recompui::LauncherMenu *menu);

    void launcher_animation_setup(recompui::LauncherMenu *menu);
    void launcher_animation_update(recompui::LauncherMenu *menu);
}

#endif
