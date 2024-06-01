#ifndef __RECOMP_FILES_H__
#define __RECOMP_FILES_H__

#include <filesystem>
#include <fstream>

namespace recomp {
    std::ifstream open_input_file_with_backup(const std::filesystem::path& filepath, std::ios_base::openmode mode = std::ios_base::in);
    std::ifstream open_input_backup_file(const std::filesystem::path& filepath, std::ios_base::openmode mode = std::ios_base::in);
    std::ofstream open_output_file_with_backup(const std::filesystem::path& filepath, std::ios_base::openmode mode = std::ios_base::out);
    bool finalize_output_file_with_backup(const std::filesystem::path& filepath);
};

#endif
