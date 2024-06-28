#ifndef RECOMPUI_CONFIG_OPTION_H
#define RECOMPUI_CONFIG_OPTION_H

#include <string>
#include "json/json.hpp"

namespace recompui {

enum class ConfigOptionType {
    Label,
    // Base types
    Checkbox,
    RadioTabs,
    Dropdown,
    Range,
    Trigger,
    // Group types
    CheckboxGroup,
    Group,
    OptionCount
};

NLOHMANN_JSON_SERIALIZE_ENUM(ConfigOptionType, {
    {ConfigOptionType::Label, "Label"},
    {ConfigOptionType::Checkbox, "Checkbox"},
    {ConfigOptionType::RadioTabs, "RadioTabs"},
    {ConfigOptionType::Dropdown, "Dropdown"},
    {ConfigOptionType::Range, "Range"},
    {ConfigOptionType::Trigger, "Trigger"},
    {ConfigOptionType::CheckboxGroup, "CheckboxGroup"},
    {ConfigOptionType::Group, "Group"}
});

typedef std::unordered_map<std::string, nlohmann::detail::value_t> config_type_structure;

inline bool config_option_is_group(ConfigOptionType option_type) {
    return option_type == ConfigOptionType::Group || option_type == ConfigOptionType::CheckboxGroup;
}

/**
    Base Config Option class. Defines what type of option or group it is, and
    sets the key.
 */

class ConfigOption {
public:
	ConfigOption(nlohmann::json& opt_json);
	virtual ~ConfigOption();

    bool validate(nlohmann::json& opt_json);

    ConfigOptionType type;
    std::string key;

    struct schema {
        static constexpr const char* type = "type";
        static constexpr const char* key  = "key";
    };

protected:
    virtual const config_type_structure& get_type_structure() const = 0;
};

}
#endif
