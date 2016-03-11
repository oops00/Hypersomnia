#pragma once
#include "entity_system/processing_system.h"
#include "../components/gui_element_component.h"
#include "../messages/camera_render_request_message.h"
#include "../messages/gui_intents.h"
#include "../messages/raw_window_input_message.h"

#include "augs/gui/gui_world.h"
#include "gui/text_drawer.h"

struct game_gui_root : public augs::gui::rect {
	augs::gui::rect inventory_overroot;
	augs::gui::rect game_windows_root;

	void get_member_children(std::vector<augs::gui::rect_id>& children) final;
};

class gui_system : public augs::processing_system_templated<components::gui_element> {
	struct drag_and_drop_result {
		messages::gui_item_transfer_intent intent;
		item_button* dragged_item = nullptr;
		bool possible_target_hovered = false;
		bool will_drop_be_successful = false;
		std::wstring tooltip_text;
	};

	drag_and_drop_result prepare_drag_and_drop_result();

	friend class item_button;

	vec2i size;

	bool is_gui_look_enabled = false;
	bool preview_due_to_item_picking_request = false;

	vec2 gui_crosshair_position;
	augs::gui::text_drawer tooltip_drawer;

	void draw_cursor_and_tooltip(messages::camera_render_request_message);

	augs::gui::gui_world gui;
	game_gui_root game_gui_root;

	rects::xywh<float> get_rectangle_for_slot_function(slot_function);
	vec2 initial_inventory_root_position();
	augs::entity_id get_game_world_crosshair();

	std::vector<messages::raw_window_input_message> buffered_inputs_during_freeze;
	bool freeze_gui_model();
public:
	gui_system(world& parent_world);

	void resize(vec2i size) {
		this->size = size;
	}

	void rebuild_gui_tree_based_on_game_state();
	void translate_raw_window_inputs_to_gui_events();
	void suppress_inputs_meant_for_gui();

	void switch_to_gui_mode_and_back();

	void draw_gui_overlays_for_camera_rendering_request(messages::camera_render_request_message);
};