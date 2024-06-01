#include "recomp_files.h"

constexpr std::u8string_view backup_suffix = u8".bak";
constexpr std::u8string_view temp_suffix = u8".temp";

std::ifstream recomp::open_input_backup_file(const std::filesystem::path& filepath, std::ios_base::openmode mode) {
    std::filesystem::path backup_path{filepath};
    backup_path += backup_suffix;
    return std::ifstream{backup_path, mode};
}

std::ifstream recomp::open_input_file_with_backup(const std::filesystem::path& filepath, std::ios_base::openmode mode) {
    std::ifstream ret{filepath, mode};

    // Check if the file failed to open and open the corresponding backup file instead if so.
    if (!ret.good()) {
        return open_input_backup_file(filepath, mode);
    }

    return ret;
}

std::ofstream recomp::open_output_file_with_backup(const std::filesystem::path& filepath, std::ios_base::openmode mode) {
    std::filesystem::path temp_path{filepath};
    temp_path += temp_suffix;
    std::ofstream temp_file_out{ temp_path, mode };

    return temp_file_out;
}

bool recomp::finalize_output_file_with_backup(const std::filesystem::path& filepath) {
    std::filesystem::path backup_path{filepath};
    backup_path += backup_suffix;

    std::filesystem::path temp_path{filepath};
    temp_path += temp_suffix;
    
    std::error_code ec;
    if (std::filesystem::exists(filepath, ec)) {
        std::filesystem::copy_file(filepath, backup_path, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            return false;
        }
    }
    std::filesystem::copy_file(temp_path, filepath, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        return false;
    }
    std::filesystem::remove(temp_path, ec);
    return true;
}
