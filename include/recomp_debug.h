#ifndef __RECOMP_DEBUG_H__
#define __RECOMP_DEBUG_H__

#include <vector>
#include <string>

namespace recomp {
    struct SceneWarps {
        int index;
        std::string name;
        std::vector<std::string> entrances;
    };

    struct AreaWarps {
        std::string name;
        std::vector<SceneWarps> scenes;
    };

    extern std::vector<AreaWarps> game_warps;

    void do_warp(int area, int scene, int entrance);
    void set_time(uint8_t day, uint8_t hour, uint8_t minute);
}

#endif
