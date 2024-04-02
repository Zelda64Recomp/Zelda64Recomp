#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include "common/rt64_user_configuration.h"

namespace ultramodern {
	enum class Resolution {
		Original,
		Original2x,
		Auto,
		OptionCount
	};
	enum class WindowMode {
		Windowed,
		Fullscreen,
		OptionCount
	};
	enum class HUDRatioMode {
		Original,
		Clamp16x9,
		Full,
		OptionCount
	};

	struct GraphicsConfig {
		Resolution res_option;
		WindowMode wm_option;
		HUDRatioMode hr_option;
		RT64::UserConfiguration::AspectRatio ar_option;
		RT64::UserConfiguration::Antialiasing msaa_option;
		RT64::UserConfiguration::RefreshRate rr_option;
		int rr_manual_value;
		int ds_option;
		bool developer_mode;

		auto operator<=>(const GraphicsConfig& rhs) const = default;
	};

	void set_graphics_config(const GraphicsConfig& config);
	GraphicsConfig get_graphics_config();

	NLOHMANN_JSON_SERIALIZE_ENUM(ultramodern::Resolution, {
		{ultramodern::Resolution::Original, "Original"},
		{ultramodern::Resolution::Original2x, "Original2x"},
		{ultramodern::Resolution::Auto, "Auto"},
	});

	NLOHMANN_JSON_SERIALIZE_ENUM(ultramodern::WindowMode, {
		{ultramodern::WindowMode::Windowed, "Windowed"},
		{ultramodern::WindowMode::Fullscreen, "Fullscreen"}
	});

	NLOHMANN_JSON_SERIALIZE_ENUM(ultramodern::HUDRatioMode, {
		{ultramodern::HUDRatioMode::Original, "Original"},
		{ultramodern::HUDRatioMode::Clamp16x9, "Clamp16x9"},
		{ultramodern::HUDRatioMode::Full, "Full"},
	});
};

#endif
