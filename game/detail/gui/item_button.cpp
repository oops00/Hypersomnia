#include "game/detail/gui/item_button.h"
#include "game/detail/gui/pixel_line_connector.h"
#include "game/detail/gui/grid.h"
#include "game/detail/gui/gui_context.h"
#include "game/detail/gui/drag_and_drop.h"
#include "game/detail/gui/root_of_inventory_gui.h"
#include "game/detail/gui/gui_element_tree.h"

#include "game/transcendental/cosmos.h"
#include "game/transcendental/entity_handle.h"
#include "game/transcendental/step.h"
#include "augs/graphics/renderer.h"
#include "augs/templates.h"

#include "augs/gui/stroke.h"

#include "game/enums/item_category.h"
#include "game/detail/inventory_slot.h"
#include "game/detail/inventory_utils.h"
#include "game/components/gui_element_component.h"
#include "game/components/sprite_component.h"
#include "game/components/item_component.h"
#include "game/systems_stateless/gui_system.h"
#include "game/systems_stateless/input_system.h"
#include "game/resources/manager.h"

#include "augs/ensure.h"

bool item_button::is_being_wholely_dragged_or_pending_finish(const const_gui_context& context, const const_this_pointer& this_id) {
	const auto& rect_world = context.get_rect_world();
	const auto& element = context.get_gui_element_component();
	const auto& cosmos = context.get_step().get_cosmos();

	if (rect_world.is_being_dragged(this_id)) {
		const bool is_drag_partial = element.dragged_charges < cosmos[this_id.get_location().item_id].get<components::item>().charges;
		return !is_drag_partial;
	}

	return false;
}

item_button::item_button(rects::xywh<float> rc) : base(rc) {
	unset_flag(augs::gui::flag::CLIP);
	unset_flag(augs::gui::flag::SCROLLABLE);
	unset_flag(augs::gui::flag::FOCUSABLE);
}

void item_button::draw_dragged_ghost_inside(const viewing_gui_context& context, const const_this_pointer& this_id, draw_info in) {
	drawing_flags f;
	f.draw_inside = true;
	f.draw_border = false;
	f.draw_connector = false;
	f.decrease_alpha = true;
	f.decrease_border_alpha = false;
	f.draw_container_opened_mark = false;
	f.draw_charges = false;
	f.absolute_xy_offset = griddify(context.get_rect_world().current_drag_amount);

	draw_proc(context, this_id, in, f);
}

void item_button::draw_complete_with_children(const viewing_gui_context& context, const const_this_pointer& this_id, draw_info in) {
	drawing_flags f;
	f.draw_inside = true;
	f.draw_border = true;
	f.draw_connector = true;
	f.decrease_alpha = false;
	f.decrease_border_alpha = false;
	f.draw_container_opened_mark = true;
	f.draw_charges = true;

	draw_children(context, this_id, in);
	draw_proc(context, this_id, in, f);
}

void item_button::draw_grid_border_ghost(const viewing_gui_context& context, const const_this_pointer& this_id, draw_info in) {
	drawing_flags f;
	f.draw_inside = false;
	f.draw_border = true;
	f.draw_connector = false;
	f.decrease_alpha = true;
	f.decrease_border_alpha = true;
	f.draw_container_opened_mark = false;
	f.draw_charges = true;
	f.absolute_xy_offset = griddify(context.get_rect_world().current_drag_amount);

	draw_proc(context, this_id, in, f);
}

void item_button::draw_complete_dragged_ghost(const viewing_gui_context& context, const const_this_pointer& this_id, draw_info in) {
	const auto& cosmos = context.get_step().get_cosmos();
	auto parent_slot = cosmos[cosmos[this_id.get_location().item_id].get<components::item>().current_slot];
	ensure(parent_slot.alive());

	draw_dragged_ghost_inside(context, this_id, in);
}

rects::ltrb<float> item_button::iterate_children_attachments(
	const const_gui_context& context,
	const const_this_pointer& this_id,
	const bool draw,
	std::vector<vertex_triangle>* target,
	const augs::rgba border_col
) {
	const auto& cosmos = context.get_step().get_cosmos();
	const auto& item_handle = cosmos[this_id.get_location().item_id];

	components::sprite item_sprite = item_handle.get<components::sprite>();

	const auto& gui_def = resource_manager.find(item_sprite.tex)->gui_sprite_def;

	item_sprite.flip_horizontally = gui_def.flip_horizontally;
	item_sprite.flip_vertically = gui_def.flip_vertically;
	item_sprite.rotation_offset = gui_def.rotation_offset;

	item_sprite.color.a = border_col.a;

	components::sprite::drawing_input state(*target);
	state.positioning = components::sprite::drawing_input::positioning_type::LEFT_TOP_CORNER;

	const auto expanded_size = this_id->rc.get_size() - this_id->with_attachments_bbox.get_size();

	state.renderable_transform.pos = context.get_tree_entry(this_id).get_absolute_pos() - this_id->with_attachments_bbox.get_position() + expanded_size / 2 + vec2(1, 1);

	rects::ltrb<float> button_bbox = item_sprite.get_aabb(components::transform(), state.positioning);

	if (!this_id->is_container_open) {
		auto iteration_lambda = [&](const const_entity_handle& desc) {
			ensure(desc != item_handle)
				return;

			const auto& parent_slot = cosmos[desc.get<components::item>().current_slot];

			if (parent_slot.should_item_inside_keep_physical_body(item_handle)) {
				auto attachment_sprite = desc.get<components::sprite>();

				attachment_sprite.flip_horizontally = item_sprite.flip_horizontally;
				attachment_sprite.flip_vertically = item_sprite.flip_vertically;
				attachment_sprite.rotation_offset = item_sprite.rotation_offset;

				attachment_sprite.color.a = item_sprite.color.a;

				components::sprite::drawing_input attachment_state = state;
				
				auto offset = parent_slot.sum_attachment_offsets_of_parents(desc) -
					cosmos[item_handle.get<components::item>().current_slot].sum_attachment_offsets_of_parents(item_handle);

				if (attachment_sprite.flip_horizontally) {
					offset.pos.x = -offset.pos.x;
					offset.flip_rotation();
				}

				if (attachment_sprite.flip_vertically) {
					offset.pos.y = -offset.pos.y;
					offset.flip_rotation();
				}

				offset += item_sprite.size / 2;
				offset += -attachment_sprite.size / 2;

				attachment_state.renderable_transform += offset;

				if (draw) {
					attachment_sprite.draw(attachment_state);
				}

				rects::ltrb<float> attachment_bbox = attachment_sprite.get_aabb(offset, state.positioning);
				button_bbox.contain(attachment_bbox);
			}
		};

		item_handle.for_each_contained_item_recursive(iteration_lambda);
	}

	if (draw) {
		item_sprite.draw(state);
	}

	return button_bbox;
}

void item_button::draw_proc(const viewing_gui_context& context, const const_this_pointer& this_id, draw_info in, const drawing_flags& f) {
	if (is_inventory_root(context, this_id))
		return;

	const auto& cosmos = context.get_step().get_cosmos();
	const auto& item = cosmos[this_id.get_location().item_id];
	const auto& detector = this_id->detector;
	const auto& rect_world = context.get_rect_world();
	const auto& element = context.get_gui_element_component();
	const auto this_absolute_rect = context.get_tree_entry(this_id).get_absolute_rect();

	auto parent_slot = cosmos[item.get<components::item>().current_slot];

	rgba inside_col = cyan;
	rgba border_col = cyan;

	if (parent_slot->for_categorized_items_only) {
		border_col = pink;
		inside_col = violet;
	}

	inside_col.a = 20;
	border_col.a = 190;

	if (detector.is_hovered) {
		inside_col.a = 30;
		border_col.a = 220;
	}

	if (detector.current_appearance == augs::gui::appearance_detector::appearance::pushed) {
		inside_col.a = 60;
		border_col.a = 255;
	}

	if (f.decrease_alpha) {
		inside_col.a = 15;
	}

	if (f.decrease_border_alpha) {
		border_col = slightly_visible_white;
	}

	if (f.draw_inside) {
		draw_stretched_texture(context, this_id, in, augs::gui::material(assets::texture_id::BLANK, inside_col));

		iterate_children_attachments(context, this_id, true, &in.v, border_col);

		if (f.draw_charges) {
			auto& item_data = item.get<components::item>();

			int considered_charges = item_data.charges;

			if (rect_world.is_being_dragged(this_id)) {
				considered_charges = item_data.charges - element.dragged_charges;
			}

			long double bottom_number_val = -1.f;
			auto* container = item.find<components::container>();
			bool printing_charge_count = false;
			bool trim_zero = false;

			auto label_color = border_col;

			if (considered_charges > 1) {
				bottom_number_val = considered_charges;
				printing_charge_count = true;
			}
			else if (element.draw_free_space_inside_container_icons && item[slot_function::ITEM_DEPOSIT].alive()) {
				if (item.get<components::item>().categories_for_slot_compatibility.test(item_category::MAGAZINE)) {
					if (!this_id->is_container_open) {
						printing_charge_count = true;
					}
				}

				if (printing_charge_count) {
					bottom_number_val = count_charges_in_deposit(item);
				}
				else {
					bottom_number_val = item[slot_function::ITEM_DEPOSIT].calculate_free_space_with_parent_containers() / long double(SPACE_ATOMS_PER_UNIT);

					if (bottom_number_val < 1.0 && bottom_number_val > 0.0) {
						trim_zero = true;
					}

					label_color.rgb() = cyan.rgb();
				}

				//if (item[slot_function::ITEM_DEPOSIT]->for_categorized_items_only)
				//	label_color.rgb() = pink.rgb();
				//else
				//	label_color.rgb() = cyan.rgb();
			}

			if (bottom_number_val > -1.f) {
				std::wstring label_wstr;

				if (printing_charge_count) {
					//label_wstr = L'x';
					label_color.rgb() = white.rgb();
					label_wstr += to_wstring(bottom_number_val);
				}
				else
					label_wstr = to_wstring(bottom_number_val, 2);

				if (trim_zero && label_wstr[0] == L'0') {
					label_wstr.erase(label_wstr.begin());
				}

				// else label_wstr = L'{' + label_wstr + L'}';

				auto bottom_number = augs::gui::text::format(label_wstr, augs::gui::text::style(assets::font_id::GUI_FONT, label_color));

				augs::gui::text_drawer charges_caption;
				charges_caption.set_text(bottom_number);
				charges_caption.bottom_right(context.get_tree_entry(this_id).get_absolute_rect());
				charges_caption.draw(in);
			}
		}
	}

	if (f.draw_border) {
		augs::gui::solid_stroke stroke;
		stroke.set_material(augs::gui::material(assets::texture_id::BLANK, border_col));
		stroke.draw(in.v, this_absolute_rect);
	}

	if (f.draw_connector && parent_slot.get_container().get_owning_transfer_capability() != parent_slot.get_container()) {
		draw_pixel_line_connector(this_absolute_rect, context.get_tree_entry(location{ parent_slot.get_container() }).get_absolute_rect(), in, border_col);
	}

	if (f.draw_container_opened_mark) {
		if (item.find<components::container>()) {
			components::sprite container_status_sprite;
			if (this_id->is_container_open)
				container_status_sprite.set(assets::texture_id::CONTAINER_OPEN_ICON, border_col);
			else
				container_status_sprite.set(assets::texture_id::CONTAINER_CLOSED_ICON, border_col);

			components::sprite::drawing_input state(in.v);
			state.positioning = components::sprite::drawing_input::positioning_type::LEFT_TOP_CORNER;
			state.renderable_transform.pos.set(this_absolute_rect.r - container_status_sprite.size.x + 2, this_absolute_rect.t + 1
				//- container_status_sprite.size.y + 2
			);
			container_status_sprite.draw(state);
		}
	}
}

bool item_button::is_inventory_root(const const_gui_context& context, const const_this_pointer& this_id) {
	return this_id.get_location().item_id == context.get_gui_element_entity();
}

void item_button::perform_logic_step(const logic_gui_context& context, const this_pointer& this_id) {
	base::perform_logic_step(context, this_id);

	const auto& cosmos = context.get_step().get_cosmos();
	const auto& item = cosmos[this_id.get_location().item_id];

	if (is_inventory_root(context, this_id)) {
		this_id->set_flag(augs::gui::flag::ENABLE_DRAWING_OF_CHILDREN);
		this_id->set_flag(augs::gui::flag::DISABLE_HOVERING);
		return;
	}

	this_id->set_flag(augs::gui::flag::ENABLE_DRAWING_OF_CHILDREN, this_id->is_container_open && !is_being_wholely_dragged_or_pending_finish(context, this_id));
	this_id->set_flag(augs::gui::flag::DISABLE_HOVERING, is_being_wholely_dragged_or_pending_finish(context, this_id));

	vec2i parent_position;

	auto* sprite = item.find<components::sprite>();

	if (sprite) {
		this_id->with_attachments_bbox = iterate_children_attachments(context, this_id);
		vec2i rounded_size = this_id->with_attachments_bbox.get_size();
		rounded_size += 22;
		rounded_size += resource_manager.find(sprite->tex)->gui_sprite_def.gui_bbox_expander;
		rounded_size /= 11;
		rounded_size *= 11;
		//rounded_size.x = std::max(rounded_size.x, 33);
		//rounded_size.y = std::max(rounded_size.y, 33);
		this_id->rc.set_size(rounded_size);
	}

	auto parent_slot = cosmos[item.get<components::item>().current_slot];

	if (parent_slot->always_allow_exactly_one_item) {
		const auto& parent_button = context.dereference_location<const slot_button>({parent_slot.get_id()});

		this_id->rc.set_position(parent_button->rc.get_position());
	}
	else {
		this_id->rc.set_position(this_id->drag_offset_in_item_deposit);
	}
}

void item_button::consume_gui_event(const logic_gui_context& context, const this_pointer& this_id, const augs::gui::event_info info) {
	if (is_inventory_root(context, this_id))
		return;

	auto& cosmos = context.get_step().get_cosmos();
	const auto& item = cosmos[this_id.get_location().item_id];
	auto& element = context.get_gui_element_component();
	auto& rect_world = context.get_rect_world();

	this_id->detector.update_appearance(info);
	auto parent_slot = cosmos[item.get<components::item>().current_slot];
	const auto& parent_button = context.dereference_location<slot_button>({ parent_slot.get_id() });

	if (info == gui_event::ldrag) {
		if (!this_id->started_drag) {
			this_id->started_drag = true;

			element.dragged_charges = item.get<components::item>().charges;

			if (parent_slot->always_allow_exactly_one_item)
				if (context.get_tree_entry(parent_button).get_absolute_rect().hover(rect_world.last_state.mouse.pos)) {
					parent_button->houted_after_drag_started = false;
				}
		}
	}

	if (info == gui_event::wheel) {
		LOG("%x", info.scroll_amount);
	}

	if (info == gui_event::rclick) {
		this_id->is_container_open = !this_id->is_container_open;
	}

	if (info == gui_event::lfinisheddrag) {
		this_id->started_drag = false;

		auto& drag_result = prepare_drag_and_drop_result(context);

		if (drag_result.possible_target_hovered && drag_result.will_drop_be_successful()) {
			perform_transfer(cosmos[drag_result.simulated_request], context.get_step());
		}
		else if (!drag_result.possible_target_hovered) {
			vec2i griddified = griddify(rect_world.current_drag_amount);

			if (parent_slot->always_allow_exactly_one_item) {
				parent_button->user_drag_offset += griddified;
				parent_button->houted_after_drag_started = true;
				parent_button->perform_logic_step(context, parent_button);
			}
			else {
				this_id->drag_offset_in_item_deposit += griddified;
			}
		}
	}

	// if(being_dragged && inf == rect::gui_event::lup)
}

void item_button::draw_triangles(const viewing_gui_context& context, const const_this_pointer& this_id, draw_info in) {
	if (is_inventory_root(context, this_id)) {
		draw_children(context, this_id, in);
		return;
	}

	if (!is_being_wholely_dragged_or_pending_finish(context, this_id)) {
		this_id->draw_complete_with_children(context, this_id, in);
	}
}