#include "sentience_system.h"

#include "game/messages/damage_message.h"
#include "entity_system/world.h"
#include "entity_system/entity.h"

#include "game/components/physics_component.h"

#include "game/detail/inventory_utils.h"

void sentience_system::apply_damage_and_initiate_deaths() {
	auto& damages = parent_world.get_message_queue<messages::damage_message>();

	for (auto& d : damages) {
		auto* sentience = d.subject->find<components::sentience>();

		augs::entity_id aimpunch_subject;
		aimpunch_subject = get_owning_transfer_capability(d.subject);

		if (sentience) {
			aimpunch_subject = d.subject;
			if (sentience->enable_health) {
				sentience->health -= d.amount;

				if (sentience->health < 0) {
					sentience->health = 0;
				}
				if (sentience->health > sentience->maximum_health) {
					sentience->health = sentience->maximum_health;
				}
			}

			if (sentience->enable_consciousness) {
				sentience->consciousness -= d.amount;

				if (sentience->consciousness < 0) {
					sentience->consciousness = 0;
				}

				if (sentience->consciousness > sentience->maximum_consciousness) {
					sentience->consciousness = sentience->maximum_consciousness;
				}
				
				sentience->consciousness = std::min(sentience->health, sentience->consciousness);
			}
		}

		if (aimpunch_subject.alive()) {
			auto* aimpunched_sentience = aimpunch_subject->find<components::sentience>();

			if (aimpunched_sentience
				&& aimpunch_subject.has(sub_entity_name::CHARACTER_CROSSHAIR)
				&& aimpunch_subject[sub_entity_name::CHARACTER_CROSSHAIR].has(sub_entity_name::CROSSHAIR_RECOIL_BODY)
				) {
				auto owning_crosshair_recoil = aimpunch_subject[sub_entity_name::CHARACTER_CROSSHAIR][sub_entity_name::CROSSHAIR_RECOIL_BODY];

				auto& recoil_physics = owning_crosshair_recoil->get<components::physics>();
				auto impulse = aimpunched_sentience->aimpunch.shoot_and_get_offset().rotate(
					// aimpunch_subject->get<components::transform>().rotation 
					d.impact_velocity.degrees()
					, vec2());
				recoil_physics.apply_impulse(impulse);
			}
		}
	}
}

void sentience_system::cooldown_aimpunches() {
	for (auto& t : targets) {
		t->get<components::sentience>().aimpunch.cooldown(delta_milliseconds());
	}
}
