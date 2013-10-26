npc_size_multiplier = vec2(0.3, 0.3)

legs_animation = create_animation {
	frames = {
		{ model = nil, 			duration_ms = 2.0 },
		{ model = { image = images.legs_5,  size_multiplier = vec2(2, 2)*npc_size_multiplier }, duration_ms = 2.0 },
		{ model = { image = images.legs_4,  size_multiplier = vec2(2, 2)*npc_size_multiplier }, duration_ms = 2.0 },
		{ model = { image = images.legs_3,  size_multiplier = vec2(2, 2)*npc_size_multiplier }, duration_ms = 2.0 },
		{ model = { image = images.legs_2,  size_multiplier = vec2(2, 2)*npc_size_multiplier }, duration_ms = 2.0 },
		{ model = { image = images.legs_1,  size_multiplier = vec2(2, 2)*npc_size_multiplier }, duration_ms = 2.0 },
		{ model = { image = images.legs_2,  size_multiplier = vec2(2, 2)*npc_size_multiplier }, duration_ms = 2.0 },
		{ model = { image = images.legs_3,  size_multiplier = vec2(2, 2)*npc_size_multiplier }, duration_ms = 2.0 },
		{ model = { image = images.legs_4,  size_multiplier = vec2(2, 2)*npc_size_multiplier }, duration_ms = 2.0 },
		{ model = { image = images.legs_5,  size_multiplier = vec2(2, 2)*npc_size_multiplier }, duration_ms = 2.0 },
		{ model = nil, duration_ms = 2.0 },
		{ model = { image = images.legs_6,  size_multiplier = vec2(2, 2)*npc_size_multiplier }, duration_ms = 2.0 },
		{ model = { image = images.legs_7,  size_multiplier = vec2(2, 2)*npc_size_multiplier }, duration_ms = 2.0 },
		{ model = { image = images.legs_8,  size_multiplier = vec2(2, 2)*npc_size_multiplier }, duration_ms = 2.0 },
		{ model = { image = images.legs_9,  size_multiplier = vec2(2, 2)*npc_size_multiplier }, duration_ms = 2.0 },
		{ model = { image = images.legs_10, size_multiplier = vec2(2, 2)*npc_size_multiplier }, duration_ms = 2.0 },
		{ model = { image = images.legs_9,  size_multiplier = vec2(2, 2)*npc_size_multiplier }, duration_ms = 2.0 },
		{ model = { image = images.legs_8,  size_multiplier = vec2(2, 2)*npc_size_multiplier }, duration_ms = 2.0 },
		{ model = { image = images.legs_7,  size_multiplier = vec2(2, 2)*npc_size_multiplier }, duration_ms = 2.0 },
		{ model = { image = images.legs_6,  size_multiplier = vec2(2, 2)*npc_size_multiplier }, duration_ms = 2.0 }
	},
	
	loop_mode = animation.REPEAT
}

player_animation = create_animation {
	frames = {
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_1 }, duration_ms = 2.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_2 }, duration_ms = 2.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_3 }, duration_ms = 2.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_4 }, duration_ms = 2.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_5 }, duration_ms = 2.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_4 }, duration_ms = 2.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_3 }, duration_ms = 2.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_2 }, duration_ms = 2.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_1 }, duration_ms = 2.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_6 }, duration_ms = 2.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_7 }, duration_ms = 2.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_8 }, duration_ms = 2.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_9 }, duration_ms = 2.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_10}, duration_ms = 2.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_8 }, duration_ms = 2.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_7 }, duration_ms = 2.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_6 }, duration_ms = 2.0 }
	},
	
	loop_mode = animation.REPEAT
}


player_shotgun_animation = create_animation {
	frames = {
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_shotgun_1},  duration_ms = 5.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_shotgun_2},  duration_ms = 5.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_shotgun_3},  duration_ms = 5.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_shotgun_4},  duration_ms = 5.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_shotgun_5},  duration_ms = 5.0 }
	},
	
	loop_mode = animation.INVERSE
}

player_shotgun_shot_animation = create_animation {
	frames = {
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_shotgun_shot_1 },  duration_ms = 15.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_shotgun_shot_2 },  duration_ms = 15.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_shotgun_shot_3 },  duration_ms = 15.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_shotgun_shot_4 },  duration_ms = 15.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_shotgun_shot_5 },  duration_ms = 15.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_shotgun_shot_4 },  duration_ms = 15.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_shotgun_shot_3 },  duration_ms = 15.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_shotgun_shot_2 },  duration_ms = 15.0 },
		{ model = { size_multiplier = npc_size_multiplier, image = images.player_shotgun_shot_1 },  duration_ms = 15.0 }
	},
	
	loop_mode = animation.NONE
}

npc_animation_body_shotgun_set = create_animation_set {
	animations = { 
		{ event = animation_events.MOVE, animation_response = player_shotgun_animation },
		{ event = animation_events.SHOT, animation_response = player_shotgun_shot_animation }
	}
}

npc_animation_body_set = create_animation_set {
	animations = { 
		{ event = animation_events.MOVE, animation_response = player_animation }
	}
}

npc_animation_legs_set = create_animation_set {
	animations = { 
		{ event = animation_events.MOVE, animation_response = legs_animation }
	}
}