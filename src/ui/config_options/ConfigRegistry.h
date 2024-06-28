#ifndef RECOMPUI_CONFIG_REGISTRY_H
#define RECOMPUI_CONFIG_REGISTRY_H

#include <string>
#include <vector>
#include <string>
#include <unordered_map>
#include <variant>
#include <map>
#include "json/json.hpp"

namespace recompui {
    struct JSONRef {
        std::string config_group;
        std::string json_path; // used as a json pointer
    };
    // config key -> JSONRef
    typedef std::unordered_map<std::string, JSONRef> config_registry_key_reference_map;
    // config group -> json
    typedef std::unordered_map<std::string, nlohmann::json> config_registry_group_json_map;

    struct ConfigRegistry {
        config_registry_key_reference_map key_ref_map;
        config_registry_group_json_map    group_json_map;
    };

    extern ConfigRegistry config_registry;

    void register_config(const std::string& json_str, const std::string& config_group);
    void register_translation(const std::string &json_str, const std::string &config_group);

    nlohmann::json get_json_from_key(const std::string &config_key);
    std::string get_string_in_json(const nlohmann::json& j, const std::string& key);
    std::string get_string_in_json_with_default(const nlohmann::json& j, const std::string& key, const std::string& default_val);
    bool config_key_is_base_group(const std::string &config_group_key);
    nlohmann::json& get_group_json(const std::string &config_group_key);

    #define TODO_PARSE_ERROR(m, t) printf("Value at key: \"%s\" was invalid. Expected: \"%s\"\n", m, t)

    template <typename T>
    T get_value_in_json(const nlohmann::json& j, const std::string& key) {
        T ret;

        auto find_it = j.find(key);
        if (find_it != j.end()) {
            auto at_val = j.at(key);
            find_it->get_to(ret);
        }

        return ret;
    }

    template <typename T>
    T get_value_in_json_with_default(const nlohmann::json& j, const std::string& key, T default_val) {
        T ret = default_val;

        auto find_it = j.find(key);
        if (find_it != j.end()) {
            auto at_val = j.at(key);
            find_it->get_to(ret);
        }

        return ret;
    }
}
#endif
