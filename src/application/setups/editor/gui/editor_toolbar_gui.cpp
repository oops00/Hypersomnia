#include "augs/misc/imgui/imgui_utils.h"
#include "augs/misc/imgui/imgui_control_wrappers.h"
#include "augs/misc/imgui/imgui_controls.h"
#include "augs/misc/imgui/imgui_game_image.h"

#include "application/setups/editor/editor_setup.hpp"
#include "application/setups/editor/gui/editor_toolbar_gui.h"
#include "application/setups/editor/project/editor_project.h"
#include "application/setups/editor/editor_setup.h"
#include "application/setups/editor/gui/widgets/icon_button.h"

#include "3rdparty/imgui/imgui_internal.h"
#include "augs/window_framework/window.h"

template <class T>
void DockingToolbar(const char* name, ImGuiAxis* p_toolbar_axis, bool& is_docked, T icons_callback)
{

    const ImVec2 icon_size(32, 32);
    // [Option] Automatically update axis based on parent split (inside of doing it via right-click on the toolbar)
    // Pros:
    // - Less user intervention.
    // - Avoid for need for saving the toolbar direction, since it's automatic.
    // Cons: 
    // - This is currently leading to some glitches.
    // - Some docking setup won't return the axis the user would expect.
    const bool TOOLBAR_AUTO_DIRECTION_WHEN_DOCKED = true;

    // ImGuiAxis_X = horizontal toolbar
    // ImGuiAxis_Y = vertical toolbar
    ImGuiAxis toolbar_axis = *p_toolbar_axis;

    // 1. We request auto-sizing on one axis
    // Note however this will only affect the toolbar when NOT docked.
	ImVec2 requested_size = (toolbar_axis == ImGuiAxis_X) ? ImVec2(-1.0f, icon_size.y + ImGui::GetStyle().WindowPadding.y*2) : ImVec2(icon_size.x + ImGui::GetStyle().WindowPadding.x * 2, -1.0f);
    ImGui::SetNextWindowSize(requested_size);

    // 2. Specific docking options for toolbars.
    // Currently they add some constraint we ideally wouldn't want, but this is simplifying our first implementation
    ImGuiWindowClass window_class;
    window_class.DockingAllowUnclassed = true;
	//window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoCloseButton;
    window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_HiddenTabBar; // ImGuiDockNodeFlags_NoTabBar // FIXME: Will need a working Undock widget for _NoTabBar to work
    window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoDockingSplitMe;
    window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoDockingOverMe;
    window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoDockingOverOther;
    if (toolbar_axis == ImGuiAxis_X)
        window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoResizeY;
    else
        window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoResizeX;
    ImGui::SetNextWindowClass(&window_class);

    // 3. Begin into the window
    ImGui::Begin(name, NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

    // 4. Overwrite node size
    ImGuiDockNode* node = ImGui::GetWindowDockNode();
    if (node != NULL)
    {
        // Overwrite size of the node
        ImGuiStyle& style = ImGui::GetStyle();
        const ImGuiAxis toolbar_axis_perp = (ImGuiAxis)(toolbar_axis ^ 1);
        const float TOOLBAR_SIZE_WHEN_DOCKED = style.WindowPadding[toolbar_axis_perp] * 2.0f + icon_size[toolbar_axis_perp];
        node->WantLockSizeOnce = true;
        node->Size[toolbar_axis_perp] = node->SizeRef[toolbar_axis_perp] = TOOLBAR_SIZE_WHEN_DOCKED;

        if (TOOLBAR_AUTO_DIRECTION_WHEN_DOCKED)
            if (node->ParentNode && node->ParentNode->SplitAxis != ImGuiAxis_None)
                toolbar_axis = (ImGuiAxis)(node->ParentNode->SplitAxis ^ 1);
    }
    
    // 5. Dummy populate tab bar
	//ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
	//ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    //UndockWidget(icon_size, toolbar_axis);
	icons_callback(toolbar_axis);

	//ImGui::PopStyleVar(1);

	is_docked = ImGui::IsWindowDocked();

    // 6. Context-menu to change axis
	if (!is_docked || !TOOLBAR_AUTO_DIRECTION_WHEN_DOCKED)
    {
        if (ImGui::BeginPopupContextWindow())
        {
            ImGui::TextUnformatted(name);
            ImGui::Separator();
            if (ImGui::MenuItem("Horizontal", "", (toolbar_axis == ImGuiAxis_X)))
                toolbar_axis = ImGuiAxis_X;
            if (ImGui::MenuItem("Vertical", "", (toolbar_axis == ImGuiAxis_Y)))
                toolbar_axis = ImGuiAxis_Y;
            ImGui::EndPopup();
        }
    }

    ImGui::End();

    // Output user stored data
    *p_toolbar_axis = toolbar_axis;
}
void editor_toolbar_gui::perform(const editor_toolbar_input in) {
	using namespace augs::imgui;
	using namespace editor_widgets;

	if (!show) {
		return;
	}

	const auto scoped_style = scoped_style_var(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

	const auto WinPadding = ImGui::GetStyle().WindowPadding;
	const auto icon_size = 32.0f;
	const auto min_window_size = ImVec2(icon_size + WinPadding.x * 2, icon_size + WinPadding.y * 2);
	(void)min_window_size;

	const auto final_padding = [&]() {
		if (is_docked) {
			if (toolbar_axis == ImGuiAxis_X) {
				return ImVec2(WinPadding.x * 2.0f, WinPadding.y);
			}
			else {
				return ImVec2(WinPadding.x, WinPadding.y * 2.0f);
			}
		}

		return ImVec2(WinPadding.x * 2.8f, WinPadding.y * 2.8f);
	}();

	auto comfier_padding = scoped_style_var(ImGuiStyleVar_WindowPadding, final_padding);

	const bool node_or_layer_inspected = 
		in.setup.inspects_any<editor_node_id>()
		|| in.setup.inspects_any<editor_layer_id>()
	;
	(void)node_or_layer_inspected;

	int current_icon_id = 0;

	const bool any_node_selected = [&]() {
		if (in.setup.inspects_any<editor_node_id>()) {
			return true;
		}

		return false;
	}();

	using ID = assets::necessary_image_id;

	auto img = [&](const auto id) {
		return in.necessary_images.at(id);
	};

	auto do_icon = [this, &current_icon_id, img, icon_size](auto img_id, auto tooltip, bool enabled = true, bool currently_active = false, rgba tint = white) {
		auto scope = scoped_id(current_icon_id++);

		std::array<rgba, 3> icon_bg_cols = {
			rgba(0, 0, 0, 0),
			rgba(35, 60, 90, 255),
			rgba(35+10, 60+10, 90+10, 255)
		};

		if (currently_active) {
			icon_bg_cols[0] = icon_bg_cols[1];
			tint *= green;
		}

		auto result = icon_button(
			"##Transform",
			img(img_id),
			[](){},
			tooltip,
			enabled,
			tint,
			icon_bg_cols,
			icon_size
		);

		if (toolbar_axis == ImGuiAxis_X) {
			ImGui::SameLine();
		}

		return result;
	};

	auto category_separator = [&]() {
		if (toolbar_axis == ImGuiAxis_X) {
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();
		}
		else {
			ImGui::Separator();
		}
	};

	auto icons_callback = [&](const ImGuiAxis) {
		if (!is_docked) {
			auto no_space = scoped_style_var(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
			ImGui::Separator();

			//auto no_space = scoped_style_var(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 5.0f));
			auto pos = ImGui::GetCursorPos();
			pos.y += 5;
			ImGui::SetCursorPos(pos);
		}

		//auto no_space = scoped_style_var(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

		const auto op = in.setup.get_current_node_transforming_op();

		if (do_icon(ID::EDITOR_TOOL_MOVE, "Move selection (T)", any_node_selected, op == node_mover_op::TRANSLATING)) {
			const auto screen_center = in.window.get_screen_size() / 2;
			ImGui::GetIO().MousePos = ImVec2(screen_center);
			in.window.set_cursor_pos(screen_center);

			in.setup.start_moving_selection();
		}

		if (do_icon(ID::EDITOR_TOOL_ROTATE_ARBITRARY, "Rotate selection (R)", any_node_selected, op == node_mover_op::ROTATING)) {
			in.setup.start_rotating_selection();
		}

		const auto resize_desc = "Resize selection\n(move cursor close to selection edge and press E)\n(Ctrl+E to pick two edges simultaneously)";

		if (do_icon(ID::EDITOR_TOOL_RESIZE, resize_desc, any_node_selected, op == node_mover_op::RESIZING)) {

		}

		if (do_icon(ID::EDITOR_TOOL_ROTATE_CCW, "Rotate selection by -90 degrees (Shift+R)", any_node_selected)) {
			in.setup.rotate_selection_once_by(-90);
		}

		if (do_icon(ID::EDITOR_TOOL_ROTATE_CW, "Rotate selection by 90 degrees (Ctrl+R)", any_node_selected)) {
			in.setup.rotate_selection_once_by(90);
		}


		if (do_icon(ID::EDITOR_TOOL_FLIP_HORIZONTALLY, "Flip selection horizontally (Shift+H)", any_node_selected)) {
			in.setup.flip_selection_horizontally();
		}

		if (do_icon(ID::EDITOR_TOOL_FLIP_VERTICALLY, "Flip selection vertically (Shift+V)", any_node_selected)) {
			in.setup.flip_selection_vertically();
		}

		category_separator();

		const bool grid_enabled = in.setup.is_grid_enabled();
		const bool snapping_enabled = in.setup.is_snapping_enabled();

		const auto grid_icon = grid_enabled ? ID::EDITOR_TOOL_GRID_ENABLED : ID::EDITOR_TOOL_GRID_DISABLED;
		const auto snap_icon = snapping_enabled ? ID::EDITOR_TOOL_SNAPPING_ENABLED : ID::EDITOR_TOOL_SNAPPING_DISABLED;
		const auto grid_desc = grid_enabled ? "Showing grid (G to toggle)" : "Hiding grid (G to toggle)";
		const auto snap_desc = snapping_enabled ? "Snapping to grid enabled (S to toggle)" : "Snapping to grid disabled (S to toggle)";

		const auto current_units = typesafe_sprintf("Current grid size: %x pixels", in.setup.get_current_grid_size());

		if (do_icon(grid_icon, grid_desc)) {
			in.setup.toggle_grid();
		}

		if (do_icon(ID::EDITOR_TOOL_GRID_SPARSER, "Sparser grid ([)\n" + current_units, grid_enabled)) {
			in.setup.sparser_grid();
		}

		if (do_icon(ID::EDITOR_TOOL_GRID_DENSER, "Denser grid (])\n" + current_units, grid_enabled)) {
			in.setup.denser_grid();
		}

		if (do_icon(snap_icon, snap_desc)) {
			in.setup.toggle_snapping();
		}

		category_separator();

		if (do_icon(ID::EDITOR_TOOL_PLAYTEST, "Playtest (P)")) {
			//in.setup.start_rotating_selection();
		}

		if (do_icon(ID::EDITOR_TOOL_HOST_SERVER, "Host a server (Ctrl+P)")) {
			//in.setup.start_rotating_selection();
		}
	};

	DockingToolbar(get_title().c_str(), &toolbar_axis, is_docked, icons_callback);
}
