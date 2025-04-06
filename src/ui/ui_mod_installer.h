#ifndef RECOMPUI_MOD_INSTALLER_H
#define RECOMPUI_MOD_INSTALLER_H

#include <librecomp/game.hpp>

#include <unordered_set>
#include <vector>
#include <string>
#include <list>

namespace recompui {
    struct ModInstaller {
        struct Installation {
            std::string mod_id;
            std::string display_name;
            recomp::Version mod_version;
            std::filesystem::path mod_file;
            std::list<std::filesystem::path> additional_files;
            bool needs_overwrite_confirmation = false;
        };

        struct Confirmation {
            std::string old_display_name;
            std::string new_display_name;
            std::string old_mod_id;
            std::string new_mod_id;
            recomp::Version old_version;
            recomp::Version new_version;
        };

        struct Result {
            std::list<std::string> error_messages;
            std::list<Installation> pending_installations;
        };

        static void start_mod_installation(const std::list<std::filesystem::path> &file_paths, std::function<void(std::filesystem::path, size_t, size_t)> progress_callback, Result &result);
        static void cancel_mod_installation(const Result& result, std::vector<std::string>& errors);
        static void finish_mod_installation(const Result &result, std::vector<std::string>& errors);
    };
};

#endif