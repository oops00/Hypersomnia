#pragma once
#include <vector>
#include "augs/math/vec2.h"
#include "game/transcendental/entity_handle_declaration.h"

#include "augs/misc/constant_size_vector.h"
#include "game/container_sizes.h"

class recoil_player {
	int delta_offset = 0;
public:
	augs::constant_size_vector<vec2, RECOIL_PLAYER_OFFSET_COUNT> offsets;
	unsigned current_offset = 0;
	int reversed = false;
	unsigned repeat_last_n_offsets = 5;

	float single_cooldown_duration_ms = 50.0;
	float remaining_cooldown_duration = -1.0;
	float scale = 1.0;

	template <class Archive>
	void serialize(Archive& ar) {
		ar(
			CEREAL_NVP(delta_offset),

			CEREAL_NVP(offsets),
			CEREAL_NVP(current_offset),
			CEREAL_NVP(reversed),
			CEREAL_NVP(repeat_last_n_offsets),

			CEREAL_NVP(single_cooldown_duration_ms),
			CEREAL_NVP(remaining_cooldown_duration),
			CEREAL_NVP(scale)
		);
	}

	vec2 shoot_and_get_offset();
	
	void shoot_and_apply_impulse(entity_handle recoil_body, float scale, bool angular_impulse = false, float additional_angle = 0.f,
		bool positional_impulse = false, float positional_rotation = 0.f);
	
	void cooldown(float amount_ms);
};
