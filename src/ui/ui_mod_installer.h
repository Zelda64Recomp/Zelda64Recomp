#ifndef RECOMPUI_MOD_INSTALLER_H
#define RECOMPUI_MOD_INSTALLER_H

#include <librecomp/game.hpp>

#include <unordered_set>

namespace recompui {
    struct ModInstaller {
        struct Installation {
            std::string mod_id;
            std::string display_name;
            recomp::Version mod_version;
            std::list<std::filesystem::path> mod_files;
            bool needs_overwrite_confirmation = false;
        };

        struct Result {
            std::list<std::string> error_messages;
            std::list<Installation> pending_installations;
        };

        static void start_mod_installation(const std::list<std::filesystem::path> &file_paths, std::function<void(std::filesystem::path, size_t, size_t)> progress_callback, Result &result);
        static void finish_mod_installation(const std::unordered_set<std::string> &confirmed_overwrites, Result &result);
    };
};

#endif