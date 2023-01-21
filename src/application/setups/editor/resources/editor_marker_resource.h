#pragma once
#include "augs/math/vec2.h"
#include "game/cosmos/entity_flavour_id.h"
#include "game/enums/marker_type.h"

using ad_hoc_entry_id = uint32_t;

struct editor_point_marker_resource_editable {
	// GEN INTROSPECTOR struct editor_point_marker_resource_editable
	point_marker_type type = point_marker_type::TEAM_SPAWN;
	// END GEN INTROSPECTOR
};

struct editor_point_marker_node;
struct editor_point_marker_resource {
	using node_type = editor_point_marker_node;

	editor_point_marker_resource_editable editable;

	mutable std::variant<typed_entity_flavour_id<point_marker>> scene_flavour_id;

	std::string unique_name;
	const auto& get_display_name() const {
		return unique_name;
	}

	static const char* get_type_name() {
		return "Point marker";
	}
};

struct editor_area_marker_resource_editable {
	// GEN INTROSPECTOR struct editor_area_marker_resource_editable
	area_marker_type type = area_marker_type::BOMBSITE;
	// END GEN INTROSPECTOR
};

struct editor_area_marker_node;
struct editor_area_marker_resource {
	using node_type = editor_area_marker_node;

	editor_area_marker_resource_editable editable;

	mutable std::variant<typed_entity_flavour_id<box_marker>> scene_flavour_id;

	std::string unique_name;
	const auto& get_display_name() const {
		return unique_name;
	}

	static const char* get_type_name() {
		return "Area marker";
	}
};
