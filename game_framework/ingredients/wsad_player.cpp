#include "ingredients.h"
#include "entity_system/entity.h"
#include "entity_system/world.h"

#include "game_framework/components/position_copying_component.h"
#include "game_framework/components/camera_component.h"
#include "game_framework/components/input_receiver_component.h"
#include "game_framework/components/crosshair_component.h"
#include "game_framework/components/sprite_component.h"
#include "game_framework/components/movement_component.h"
#include "game_framework/components/rotation_copying_component.h"
#include "game_framework/components/animation_component.h"
#include "game_framework/components/animation_response_component.h"
#include "game_framework/components/physics_component.h"
#include "game_framework/components/children_component.h"
#include "game_framework/components/trigger_detector_component.h"
#include "game_framework/components/driver_component.h"
#include "game_framework/components/force_joint_component.h"


#include "game_framework/shared/physics_setup_helpers.h"

#include "game_framework/globals/filters.h"
#include "game_framework/globals/input_profiles.h"

namespace ingredients {
	void wsad_player_setup_movement(augs::entity_id e) {
		components::movement& movement = e->get<components::movement>();

		movement.add_animation_receiver(e, false);

		movement.input_acceleration_axes.set(1, 1);
		movement.acceleration_length = 10000;
		
		//movement.input_acceleration_axes.set(8000, 8000);
		//movement.acceleration_length = -1;

		movement.max_speed_animation = 1000;
		movement.braking_damping = 24.5f;
		
		movement.enable_braking_damping = true;
	}

	void wsad_player_physics(augs::entity_id e) {
		body_definition body;
		fixture_definition info;
		info.from_renderable(e);

		info.filter = filters::controlled_character();
		info.density = 1.0;
		body.fixed_rotation = false;
		body.angled_damping = true;

		auto& physics = create_physics_component(body, e);
		add_fixtures(info, e);

		//physics.air_resistance = 5.f;
		physics.set_linear_damping(20);

		wsad_player_setup_movement(e);
	}

	void wsad_player_legs(augs::entity_id legs, augs::entity_id player) {
		components::sprite sprite;
		components::render render;
		components::animation animation;
		components::transform transform;
	}

	void wsad_player_crosshair(augs::entity_id e) {
		components::sprite sprite;
		components::render render;
		components::transform transform;
		components::crosshair crosshair;

		sprite.set(assets::texture_id::TEST_CROSSHAIR, rgba(255, 0, 0, 255));

		render.layer = render_layer::CROSSHAIR;
		render.interpolate = false;

		crosshair.sensitivity.set(3, 3);

		e->add(render);
		e->add(sprite);
		e->add(transform);
		e->add(input_profiles::crosshair());
		e->add(crosshair);

		ingredients::make_always_visible(e);
	}

	void wsad_player(augs::entity_id e, augs::entity_id crosshair_entity, augs::entity_id camera_entity) {
		components::sprite sprite;
		components::render render;
		components::animation animation;
		components::animation_response animation_response;
		components::transform transform;
		components::movement movement;
		components::rotation_copying rotation_copying;
		components::children children;
		components::trigger_detector detector;
		components::driver driver;
		components::force_joint force_joint;

		force_joint.force_towards_chased_entity = 85000.f;
		force_joint.distance_when_force_easing_starts = 20.f;
		force_joint.power_of_force_easing_multiplier = 1.f;

		driver.density_while_driving = 0.02f;
		driver.standard_density = 0.6f;

		movement.standard_linear_damping = 20.f;
		// driver.linear_damping_while_driving = 4.f;

		children.map_sub_entity(crosshair_entity, components::children::CHARACTER_CROSSHAIR);

		animation_response.response = assets::animation_response_id::TORSO_SET;

		sprite.set(assets::texture_id::TEST_PLAYER, rgba(255, 255, 255, 255));

		render.layer = render_layer::CHARACTER;

		rotation_copying.target = crosshair_entity;
		rotation_copying.look_mode = components::rotation_copying::look_type::POSITION;
		rotation_copying.use_physical_motor = true;

		e->add(transform);
		e->add(input_profiles::character());
		e->add(render);
		e->add(animation);
		e->add(animation_response);
		e->add(sprite);
		e->add(movement);
		e->add(rotation_copying);
		e->add(detector);
		e->add(driver);
		e->add(children);
		e->add(force_joint);
		e->disable(force_joint);
		
		wsad_player_setup_movement(e);

		components::camera::configure_camera_and_character_with_crosshair(camera_entity, e, crosshair_entity);
	}
}