
#include "ConfigOption.h"
#include "librecomp/config_store.hpp"
#include <string>
#include <unordered_map>

using json = nlohmann::json;
namespace recompui {

bool ConfigOption::validate(json& opt_json) {
    auto type_struct = get_type_structure();
    for (const auto& [key, expected_type] : type_struct) {
        if (!opt_json.contains(key) || opt_json[key].type() != expected_type) {
            return false;
        }
    }
    return true;
}

ConfigOption::ConfigOption(json& opt_json)
{
    type = opt_json[ConfigOption::schema::type];
    key = opt_json[ConfigOption::schema::key];
}

ConfigOption::~ConfigOption()
{

}

} // namespace Rml
