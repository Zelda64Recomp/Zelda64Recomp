#include "ui_mod_installer.h"

#include "librecomp/mods.hpp"

namespace recompui {
    static const std::string ManifestFilename = "mod.json";
    static const char *TextureDatabaseFilename = "rt64.json";
    static const std::u8string OldExtension = u8".old";
    static const std::u8string NewExtension = u8".new";

    static bool is_dynamic_lib(const std::filesystem::path &file_path) {
#if defined(_WIN32)
        return file_path.extension() == ".dll";
#elif defined(__linux__)
        return file_path.extension() == ".so" || file_path.filename().string().find(".so.") != std::string::npos;
#elif defined(__APPLE__)
        return file_path.extension() == ".dylib";
#else
        static_assert(false, "Unimplemented for this platform.");
#endif
    }

    size_t zip_write_func(void *opaque, mz_uint64 offset, const void *bytes, size_t count) {
        std::ofstream &stream = *(std::ofstream *)(opaque);
        stream.seekp(offset, std::ios::beg);
        stream.write((const char *)(bytes), count);
        return stream.bad() ? 0 : count;
    }

    void start_single_mod_installation(const std::filesystem::path &file_path, recomp::mods::ZipModFileHandle &file_handle, std::function<void(std::filesystem::path, size_t, size_t)> progress_callback, ModInstaller::Result &result) {
        // Check for the existence of the manifest file.
        std::filesystem::path mods_directory = recomp::mods::get_mods_directory();
        std::filesystem::path target_path = mods_directory / file_path.filename();
        std::filesystem::path target_write_path = target_path.u8string() + NewExtension;
        ModInstaller::Installation installation;
        bool exists = false;
        std::vector<char> manifest_bytes = file_handle.read_file(ManifestFilename, exists);
        if (exists) {
            // Parse the manifest file to check for its validity.
            std::string error;
            recomp::mods::ModManifest manifest;
            recomp::mods::ModOpenError open_error = parse_manifest(manifest, manifest_bytes, error);
            exists = (open_error == recomp::mods::ModOpenError::Good);

            if (exists) {
                installation.mod_id = manifest.mod_id;
                installation.display_name = manifest.display_name;
                installation.mod_version = manifest.version;
                installation.mod_file = target_path;
            }
        }
        else if (file_path.extension() == ".rtz") {
            // When it's an rtz file, check if the texture database file exists.
            exists = mz_zip_reader_locate_file(file_handle.archive.get(), TextureDatabaseFilename, nullptr, 0) >= 0;

            if (exists) {
                installation.mod_id = std::string((const char *)(target_path.stem().u8string().c_str()));
                installation.display_name = installation.mod_id;
                installation.mod_version = recomp::Version{0, 0, 0, ""};
                installation.mod_file = target_path;
            }
        }

        std::error_code ec;
        if (exists) {
            std::filesystem::copy(file_path, target_write_path, ec);
            if (ec) {
                result.error_messages.emplace_back("Unable to install " + file_path.filename().string() + " to mod directory.");
                return;
            }
        }
        else {
            result.error_messages.emplace_back(file_path.string() + " is not a mod.");
            std::filesystem::remove(target_write_path, ec);
            return;
        }

        if (std::filesystem::exists(installation.mod_file, ec)) {
            installation.needs_overwrite_confirmation = true;
        }
        if (!installation.needs_overwrite_confirmation) {
            // This check isn't really needed as additional_files will be empty for a single mod installation,
            // but it's good to have in case this logic ever changes.
            for (const std::filesystem::path &path : installation.additional_files) {
                if (std::filesystem::exists(path, ec)) {
                    installation.needs_overwrite_confirmation = true;
                    break;
                }
            }
        }

        result.pending_installations.emplace_back(installation);
    }

    void start_package_mod_installation(const std::filesystem::path &path, recomp::mods::ZipModFileHandle &file_handle, std::function<void(std::filesystem::path, size_t, size_t)> progress_callback, ModInstaller::Result &result) {
        std::error_code ec;
        char filename[1024];
        std::filesystem::path mods_directory = recomp::mods::get_mods_directory();
        mz_zip_archive *zip_archive = file_handle.archive.get();
        mz_uint num_files = mz_zip_reader_get_num_files(file_handle.archive.get());
        std::list<std::filesystem::path> dynamic_lib_files;
        std::list<ModInstaller::Installation>::iterator first_nrm_iterator = result.pending_installations.end();
        bool found_mod = false;
        for (mz_uint i = 0; i < num_files; i++) {
            mz_uint filename_length = mz_zip_reader_get_filename(zip_archive, i, filename, sizeof(filename));
            if (filename_length == 0) {
                continue;
            }

            std::filesystem::path target_path = mods_directory / std::u8string_view((const char8_t *)(filename));
            if ((target_path.extension() == ".rtz") || (target_path.extension() == ".nrm")) {
                found_mod = true;
                ModInstaller::Installation installation;
                std::filesystem::path target_write_path = target_path.u8string() + NewExtension;
                std::ofstream output_stream(target_write_path, std::ios::binary);
                if (!output_stream.is_open()) {
                    result.error_messages.emplace_back("Unable to write to mod directory.");
                    continue;
                }

                if (!mz_zip_reader_extract_to_callback(zip_archive, i, &zip_write_func, &output_stream, 0)) {
                    output_stream.close();
                    std::filesystem::remove(target_write_path, ec);
                    result.error_messages.emplace_back("Failed to install " + path.filename().string() + " to mod directory.");
                    continue;
                }

                output_stream.close();
                if (output_stream.bad()) {
                    std::filesystem::remove(target_write_path, ec);
                    result.error_messages.emplace_back("Failed to install " + path.filename().string() + " to mod directory.");
                    continue;
                }

                // Try to load the extracted file as a mod file handle.
                recomp::mods::ModOpenError open_error;
                std::unique_ptr<recomp::mods::ZipModFileHandle> extracted_file_handle = std::make_unique<recomp::mods::ZipModFileHandle>(target_write_path, open_error);
                if (open_error != recomp::mods::ModOpenError::Good) {
                    result.error_messages.emplace_back("Invalid mod (" + target_path.filename().string() + ") in " + path.filename().string() + ".");
                    extracted_file_handle.reset();
                    std::filesystem::remove(target_write_path, ec);
                    continue;
                }

                // Check for the existence of the manifest file.
                bool exists = false;
                std::vector<char> manifest_bytes = extracted_file_handle->read_file(ManifestFilename, exists);
                if (exists) {
                    // Parse the manifest file to check for its validity.
                    std::string error;
                    recomp::mods::ModManifest manifest;
                    open_error = parse_manifest(manifest, manifest_bytes, error);
                    exists = (open_error == recomp::mods::ModOpenError::Good);

                    if (exists) {
                        installation.mod_id = manifest.mod_id;
                        installation.display_name = manifest.display_name;
                        installation.mod_version = manifest.version;
                        installation.mod_file = target_path;
                    }
                }
                else if (target_path.extension() == ".rtz") {
                    // When it's an rtz file, check if the texture database file exists.
                    exists = mz_zip_reader_locate_file(extracted_file_handle->archive.get(), TextureDatabaseFilename, nullptr, 0) >= 0;

                    if (exists) {
                        installation.mod_id = std::string((const char *)(target_path.stem().u8string().c_str()));
                        installation.display_name = installation.mod_id;
                        installation.mod_version = recomp::Version();
                        installation.mod_file = target_path;
                    }
                }

                if (!exists) {
                    result.error_messages.emplace_back("Invalid mod (" + target_path.filename().string() + ") in " + path.filename().string() + ".");
                    extracted_file_handle.reset();
                    std::filesystem::remove(target_write_path, ec);
                    continue;
                }

                if (std::filesystem::exists(installation.mod_file, ec)) {
                    installation.needs_overwrite_confirmation = true;
                }
                if (!installation.needs_overwrite_confirmation) {
                    // This check isn't really needed as additional_files will be empty at this point,
                    // but it's good to have in case this logic ever changes.
                    for (const std::filesystem::path &path : installation.additional_files) {
                        if (std::filesystem::exists(path, ec)) {
                            installation.needs_overwrite_confirmation = true;
                            break;
                        }
                    }
                }

                result.pending_installations.emplace_back(installation);

                // Store the first nrm found for any dynamic libraries that might be found.
                if ((first_nrm_iterator == result.pending_installations.end()) && (target_path.extension() == ".nrm")) {
                    first_nrm_iterator = std::prev(result.pending_installations.end());
                }
            }
            
            if (is_dynamic_lib(target_path)) {
                std::filesystem::path target_write_path = target_path.u8string() + NewExtension;
                std::ofstream output_stream(target_write_path, std::ios::binary);
                if (!output_stream.is_open()) {
                    result.error_messages.emplace_back("Failed to install " + path.filename().string() + " to mod directory.");
                    continue;
                }

                if (!mz_zip_reader_extract_to_callback(zip_archive, i, &zip_write_func, &output_stream, 0)) {
                    output_stream.close();
                    std::filesystem::remove(target_write_path, ec);
                    result.error_messages.emplace_back("Failed to install " + path.filename().string() + " to mod directory.");
                    continue;
                }

                output_stream.close();
                if (output_stream.bad()) {
                    std::filesystem::remove(target_write_path, ec);
                    result.error_messages.emplace_back("Failed to install " + path.filename().string() + " to mod directory.");
                    continue;
                }

                dynamic_lib_files.emplace_back(target_path);
            }
        }

        if (!dynamic_lib_files.empty()) {
            if (first_nrm_iterator != result.pending_installations.end()) {
                // Associate all these files to the first mod that is found.
                for (const std::filesystem::path &path : dynamic_lib_files) {
                    first_nrm_iterator->additional_files.emplace_back(path);

                    // Run verification against for overwrite confirmations.
                    if (std::filesystem::exists(path, ec)) {
                        first_nrm_iterator->needs_overwrite_confirmation = true;
                    }
                }
            }
            else {
                // These library files were not required by any mod, just delete them.
                for (const std::filesystem::path &path : dynamic_lib_files) {
                    std::filesystem::path new_path(path.u8string() + NewExtension);
                    std::filesystem::remove(new_path, ec);
                }
            }
        }

        if (!found_mod) {
            result.error_messages.emplace_back("No mods found in " + path.filename().string() + ".");
        }
    }

    void remove_and_rename(std::vector<std::string>& error_messages, const std::filesystem::path& path) {
        std::error_code ec;
        std::filesystem::path old_path(path.u8string() + OldExtension);
        std::filesystem::path new_path(path.u8string() + NewExtension);

        // Rename the current path to a temporary old path, but only if the current path already exists.
        if (std::filesystem::exists(path, ec)) {
            std::filesystem::remove(old_path, ec);
            std::filesystem::rename(path, old_path, ec);
            if (ec) {
                // If it fails, remove the new path.
                std::filesystem::remove(new_path, ec);
                error_messages.emplace_back("Unable to rename " + path.filename().string() + ".");
                return;
            }
        }

        // Rename the new path to the current path.
        std::filesystem::rename(new_path, path, ec);
        if (ec) {
            // If it fails, remove the new path and also restore the temporary old path to the current path.
            std::filesystem::remove(new_path, ec);
            std::filesystem::rename(old_path, path, ec);
            error_messages.emplace_back("Unable to rename " + path.filename().string() + ".");
            return;
        }

        // If nothing failed, just remove the temporary old path.
        std::filesystem::remove(old_path, ec);
    };

    void ModInstaller::start_mod_installation(const std::list<std::filesystem::path> &file_paths, std::function<void(std::filesystem::path, size_t, size_t)> progress_callback, Result &result) {
        result = Result();

        for (const std::filesystem::path &path : file_paths) {
            if (is_dynamic_lib(path)) {
                result.error_messages.emplace_back("The provided mod(s) must be installed without extracting the ZIP file(s). Please install the mod ZIP file(s) directly.");
                return;
            }
        }

        for (const std::filesystem::path &path : file_paths) {
            recomp::mods::ModOpenError open_error;
            recomp::mods::ZipModFileHandle file_handle(path, open_error);
            if (open_error != recomp::mods::ModOpenError::Good) {
                result.error_messages.emplace_back(path.filename().string() + " is not a valid zip or mod.");
                continue;
            }

            // First we verify if the container itself isn't a mod already.
            // TODO hook into the runtime's container registration to check the extension instead of using hardcoded values.
            if ((path.extension() == ".rtz") || (path.extension() == ".nrm")) {
                start_single_mod_installation(path, file_handle, progress_callback, result);
            }
            else {
                // Scan the container for compatible mods instead. This is the case for packages made by users or how they're tipically uploaded to Thunderstore.
                start_package_mod_installation(path, file_handle, progress_callback, result);
            }
        }
    }

    void ModInstaller::cancel_mod_installation(const Result &result, std::vector<std::string>& error_messages) {
        error_messages.clear();

        std::error_code ec;
        // Delete all the files that were extracted for all mods.
        for (const Installation &installation : result.pending_installations) {
            std::filesystem::path new_path(installation.mod_file.u8string() + NewExtension);
            std::filesystem::remove(new_path, ec);
            for (const std::filesystem::path &path : installation.additional_files) {
                std::filesystem::path new_path(path.u8string() + NewExtension);
                std::filesystem::remove(new_path, ec);
            }
        }
    }

    void ModInstaller::finish_mod_installation(const Result &result, std::vector<std::string>& error_messages) {
        error_messages.clear();

        std::error_code ec;
        for (const Installation &installation : result.pending_installations) {
            // Overwrite the mod files.
            remove_and_rename(error_messages, installation.mod_file);
            for (const std::filesystem::path &path : installation.additional_files) {
                remove_and_rename(error_messages, path);
            }
        }
    }
};