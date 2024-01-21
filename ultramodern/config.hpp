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

	struct GraphicsConfig {
		Resolution res_option;
		WindowMode wm_option;
		RT64::UserConfiguration::AspectRatio ar_option;
		RT64::UserConfiguration::Antialiasing msaa_option;
		RT64::UserConfiguration::RefreshRate rr_option;
		int rr_manual_value;

		auto operator<=>(const GraphicsConfig& rhs) const = default;
	};

	void set_graphics_config(const GraphicsConfig& config);
	const GraphicsConfig& get_graphics_config();
};

#endif
