#include "ConfigRegistry.h"
#include "ConfigOption.h"
#include "librecomp/config_store.hpp"

using json = nlohmann::json;

namespace recompui {

ConfigRegistry config_registry = {{}, {}};

#define TODO_PARSE_ERROR(m, t) printf("Value at key: \"%s\" was invalid. Expected: \"%s\"\n", m, t)

std::string get_string_in_json(const json& j, const std::string& key) {
    std::string ret;

    auto find_it = j.find(key);
    if (find_it != j.end()) {
        auto at_val = j.at(key);
        if (!at_val.is_string()) {
            TODO_PARSE_ERROR(key, "string");
        }

        find_it->get_to(ret);
    }

    return ret;
}

std::string get_string_in_json_with_default(const json& j, const std::string& key, const std::string& default_val) {
    std::string ret;

    auto find_it = j.find(key);
    if (find_it != j.end()) {
        auto at_val = j.at(key);
        if (!at_val.is_string()) {
            return default_val;
        }

        find_it->get_to(ret);
    } else {
        return default_val;
    }

    return ret;
}

static bool validate_json_value_is_array(const json &j, const std::string& json_path, const std::string& arr_key) {
    const auto &options = j.find(arr_key);
    if (options == j.end()) {
        TODO_PARSE_ERROR(json_path + "/" + arr_key, "array");
        return false;
    }
    const auto &opt_array = j[arr_key];

    if (!opt_array.is_array()) {
        TODO_PARSE_ERROR(json_path + "/" + arr_key, "array");
        return false;
    }

    return true;
}

// Option
void register_config_option(
    const json& j,
    const std::string& previous_key, // previous path before current key
    const std::string& config_group,
    const std::string& json_path // path to this json object from the root
) {
    const std::string key = get_string_in_json(j, ConfigOption::schema::key);
    const std::string this_key = previous_key + "/" + key;

    ConfigOptionType type = get_value_in_json<ConfigOptionType>(j, ConfigOption::schema::type);

    config_registry.key_ref_map[this_key] = { config_group, json_path };

    switch (type) {
        case ConfigOptionType::Checkbox: {
            bool default_val = false;
            if (j.find("default") != j.end()) {
                default_val = get_value_in_json<bool>(j, "default");
            }
            recomp::set_config_store_value_and_default(this_key, default_val, default_val);
            break;
        }
        case ConfigOptionType::RadioTabs: {
            if (!validate_json_value_is_array(j, json_path, "values")) {
                return;
            }

            int default_val = 0;
            if (j.find("default") != j.end()) {
                const auto &opt_array = j["values"];
                const std::string default_val_string = get_string_in_json(j, "default");
                // Based on default value's string, find which option index corresponds
                for (int i = 0; i < opt_array.size(); i++) {
                    const auto &j_opt = opt_array[i];
                    if (!j_opt.is_string()) {
                        TODO_PARSE_ERROR(key + "/values/" + std::to_string(i) , "string");
                        return;
                    }
                    const std::string opt_val = j_opt.get<std::string>();
                    if (opt_val == default_val_string) {
                        default_val = i;
                        break;
                    }
                }
            }
            recomp::set_config_store_value_and_default(this_key, default_val, default_val);
            break;
        }
        case ConfigOptionType::Range: {
            int default_val = 0;
            int max = 0;
            int min = 0;
            int step = 1;

            if (j.find("default") != j.end()) {
                default_val = get_value_in_json<int>(j, "default");
            }

            // Max is required
            if (j.find("max") != j.end()) {
                max = get_value_in_json<int>(j, "max");
                if (default_val > max) default_val = max;
            } else {
                TODO_PARSE_ERROR(key + "/max", "int");
                return;
            }

            if (j.find("min") != j.end()) {
                min = get_value_in_json<int>(j, "min");
                if (default_val < min) default_val = min;
            }

            if (j.find("step") != j.end()) {
                step = get_value_in_json<int>(j, "step");
            }

            assert(max > min);
            assert(step < max - min);
            recomp::set_config_store_value_and_default(this_key, default_val, default_val);
            break;
        }
    }
    if (j.find("label") != j.end()) {
        const std::string label = get_string_in_json(j, "label");
        recomp::set_config_store_value(this_key, label);
    }

    if ((type == ConfigOptionType::Group || type == ConfigOptionType::CheckboxGroup) && j.find("options") != j.end()) {
        if (!validate_json_value_is_array(j, json_path, "options")) {
            return;
        }

        const auto &opt_array = j["options"];

        for (int i = 0; i < opt_array.size(); i++) {
            const auto &el = opt_array[i];
            register_config_option(
                el,
                this_key,
                config_group,
                json_path + "/options/" + std::to_string(i)
            );
        }
    }
}

void register_config(const std::string &json_str, const std::string &config_group) {
    config_registry.group_json_map[config_group] = json::parse(json_str);
    const auto &j = config_registry.group_json_map[config_group];

    if (!j.is_array()) {
        TODO_PARSE_ERROR("/", "array");
        return;
    }

    for (int i = 0; i < j.size(); i++) {
        const auto &el = j[i];
        register_config_option(
            el,
            config_group,
            config_group,
            "/" + std::to_string(i) // json_path at top level
        );
    }
}

void register_translation(const std::string &json_str, const std::string &config_group) {
    const auto &j = json::parse(json_str);

    if (!j.is_object()) {
        TODO_PARSE_ERROR("/", "object");
        return;
    }

    for (auto& el : j.items())
    {
        std::string translation_key = "translations/" + config_group + "/" + el.key();
        const std::string value = el.value();
        recomp::set_config_store_value(translation_key, value);
    }
}

json get_json_from_key(const std::string &config_key) {
    if (config_registry.key_ref_map.find(config_key) == config_registry.key_ref_map.end()) {
        // TODO: handle not finding config_key
        printf("FAILURE: AddOptionTypeElement failed to find config_key '%s' in config_registry.key_ref_map\n", config_key);
    }
    const JSONRef& json_ref = config_registry.key_ref_map[config_key];
    const json& group_json = config_registry.group_json_map[json_ref.config_group];
    json::json_pointer pointer(json_ref.json_path);
    return group_json[pointer];
}

bool config_key_is_base_group(const std::string &config_group_key) {
    // determine if the key references a base group by checking the group_json_map
    if (config_registry.group_json_map.find(config_group_key) == config_registry.group_json_map.end()) {
        return false;
    }
    return true;
}

json& get_group_json(const std::string &config_group_key) {
    return config_registry.group_json_map[config_group_key];
}


} // namespace recompui
