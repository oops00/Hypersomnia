#pragma once
#include <string>
#include "augs/window_framework/window_settings.h"
#include "application/http_client/self_update_settings.h"
#include "augs/image/image.h"

enum class self_update_result_type {
	// GEN INTROSPECTOR enum class self_update_result_type
	NONE,

	EXIT_APPLICATION,
	FAILED,
	COULDNT_DOWNLOAD_BINARY,
	COULDNT_DOWNLOAD_VERSION_FILE,
	COULDNT_SAVE_BINARY,
	FAILED_TO_OPEN_SSH_KEYGEN,
	FAILED_TO_VERIFY_BINARY,
	DOWNLOADED_BINARY_WAS_OLDER,
	CANCELLED,
	UPGRADED,
	UP_TO_DATE,

	FIRST_LAUNCH_AFTER_UPGRADE,

	UPDATE_AVAILABLE
	// END GEN INTROSPECTOR
};

struct self_update_result {
	using result_type = self_update_result_type;

	result_type type = result_type::NONE;
	bool exit_with_failure_if_not_upgraded = false;
};

namespace augs {
	class image;
	struct font_loading_input;
}

self_update_result check_and_apply_updates(
	const augs::path_type& current_appimage_path,
	bool only_check_availability_and_quit,
	const self_update_settings& settings,
	const augs::font_loading_input* in = nullptr,
	std::optional<augs::window_settings> window_settings = std::nullopt
);
