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
	enum class GraphicsApi {
		Auto,
		D3D12,
		Vulkan,
		OptionCount
	};
	enum class HighPrecisionFramebuffer {
		Auto,
		On,
		Off,
		OptionCount
	};

	struct GraphicsConfig {
		Resolution res_option;
		WindowMode wm_option;
		HUDRatioMode hr_option;
		GraphicsApi api_option;
		RT64::UserConfiguration::AspectRatio ar_option;
		RT64::UserConfiguration::Antialiasing msaa_option;
		RT64::UserConfiguration::RefreshRate rr_option;
		HighPrecisionFramebuffer hpfb_option;
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

	NLOHMANN_JSON_SERIALIZE_ENUM(ultramodern::GraphicsApi, {
		{ultramodern::GraphicsApi::Auto, "Auto"},
		{ultramodern::GraphicsApi::D3D12, "D3D12"},
		{ultramodern::GraphicsApi::Vulkan, "Vulkan"},
	});

	NLOHMANN_JSON_SERIALIZE_ENUM(ultramodern::HighPrecisionFramebuffer, {
		{ultramodern::HighPrecisionFramebuffer::Auto, "Auto"},
		{ultramodern::HighPrecisionFramebuffer::On, "On"},
		{ultramodern::HighPrecisionFramebuffer::Off, "Off"},
	});
};

#endif
