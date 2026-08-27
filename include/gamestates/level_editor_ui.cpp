#include "level_editor.hpp"

using ViewSelector = LevelEditor::ViewSelector;

Rectangle ViewSelector::bounds() const {
	const float posX = left_padding;
	// Background
	const float posY = (float) *screen_height - btm_padding - height;
	return { posX, posY, width, height };
}

int ViewSelector::calc_font_size() const {
	// ???
	return 15;
}

void ViewSelector::iterate_views(std::function<void(const LevelEditor::View, Rectangle bounds)> func) const {
	for (int i = 0; i < LevelEditor::SIZE; i++) {
		const float view_delta_y = (float) i * (view_top_padding + view_height);
		const Rectangle bounds {
			.x = left_padding + view_left_padding,
			.y = (float) *screen_height - btm_padding - height + view_top_padding + view_delta_y,
			.width = view_width,
			.height = view_height
		};
		func((View) i, bounds);
	}
}

void LevelEditor::initialize_extra_buttons(Game& game, int screen_width, std::function<float(float)> adjust_t, ColorStateLerpFn color_lerp) {
	extra_button_manager.owner = this;
	Region<ColorState>* extra_button_region = nullptr;

	{
		Color extra_buttons_bg_color {
			.r = 40,
			.g = 30,
			.b = 65,
			.a = 55
		};
		auto region = (new Region<ColorState>())->init(
			1.2,
			&time,
			{
				(float) screen_width - 200,
				450,
				150,
				50
			},
			color_lerp,
			{ 0, extra_buttons_bg_color },
			{ 0, { 90, 40, 40, 180 } },
			extra_buttons_bg_color);
		
		region->adjust_t = adjust_t;
		region->render_fn = [](UiElement* ui_element) {
			DrawRectangleRec(ui_element->bounds, ui_element->color);
		};

		extra_button_manager.region_name = ui_elem_manager.add(region, "generic_region");
		extra_button_region = region;
	}

	auto make_centered_rect = [](Vector2 dims, Rectangle ref_rect) {
		return Rectangle {
			.x = ref_rect.x + ref_rect.width / 2 - dims.x / 2,
			.y = ref_rect.y + ref_rect.height / 2 - dims.y / 2,
			.width = dims.x,
			.height = dims.y
		};
	};

	// Make center on Petra button
	{
		auto button = new Button<Petra*>(
			&game.level.petra,
			make_centered_rect(
				{ 130, 40 },
				extra_button_region->bounds),
			"Center on Petra",
			[](auto& _) {},
			[](auto& self) {
				std::println("Move cam to petra at {}", to_str(self.data->pos, 2));
			},
			MAGENTA);
		button->render_fn = [](UiElement* ui_element) {
			auto& self = *dynamic_cast<Button<Petra*>*>(ui_element);
			DrawRectangleRounded(self.bounds, 0.3, 1, self.color);
			DrawText(self.state.text.c_str(), self.bounds.x, self.bounds.y, 24, WHITE);
		};

		const auto& name = ui_elem_manager.add(button, "center_petra_btn");
		extra_button_manager.extra_state_to_group[OnCameraView] = { { name } };
	}

	// Make merge tendrils button
	{
		auto button = new Button<LevelEditor*>(
			this,
			make_centered_rect(
				{ 110, 40 },
				extra_button_region->bounds),
			"Merge tendrils",
			[](auto& _) {
				std::println("HOVER merge tendrils");
			},
			[](Button<LevelEditor*>& self) {
				// const LevelEditor* data = self.data;
				auto& all_config_info = self.data->all_config_info;
				auto& selections = self.data->selections;
				// Default to the merged tree output being based on the first selection.
				Tree* kept_tree_addr = all_config_info[selections[0].index].ptr->tree_owner;

				// Move branches to chosen tree
				for (const auto& selection : self.data->selections) {
					auto& sel_config_info = all_config_info[selection.index];
					Tree* sel_tree_addr = sel_config_info.ptr->tree_owner;
					if (sel_tree_addr == kept_tree_addr)
						continue;

					auto it = sel_tree_addr->tendril_configs.begin() + sel_config_info.tree_owner_index;
					kept_tree_addr->tendril_configs.push_back(std::move(*it));
					sel_tree_addr->tendril_configs.erase(it);

					sel_config_info.ptr->tree_owner = kept_tree_addr;
				}

				self.data->extra_button_manager.set_active_extra_button_group(None);
			},
			GREEN);
		button->render_fn = [](UiElement* ui_element) {
			auto& self = *dynamic_cast<Button<LevelEditor*>*>(ui_element);
			DrawRectangleRounded(self.bounds, 0.3, 1, self.color);
			DrawText(self.state.text.c_str(), self.bounds.x, self.bounds.y, 24, WHITE);
		};

		const auto& name = ui_elem_manager.add(button, "merge_trees_btn");
		extra_button_manager.extra_state_to_group[OnMultipleSelected] = { { name } };
	}

	extra_button_manager.extra_state_to_group[None] = { {} };
}

void LevelEditor::initialize_ui(Game& game) {
	const int screen_width = game.screen_width;
	ColorStateLerpFn color_lerp = [](Region<ColorState>& self, Region<ColorState>::State& curr, Region<ColorState>::State& target, float t) {
		self.color = ColorLerp(target.data.hover_color, curr.data.hover_color, t);
	};
	auto adjust_t = [](float t) {
		return t * t;
	};

	debug_button = new Button<LevelEditor*>(
		this,
		to_rect({ (float) screen_width - 190, 110 }, { 80, 80 }),
		"Show debug keybinds",
		[](auto& b) {
			b.idle_state.text = "Press me to toggle instructions";
		},
		[](auto& b) {
			bool& show_instructions = b.data->show_instructions;
			show_instructions = !show_instructions;
		});
	ui_elem_manager.add(debug_button, "debug_button");

	auto depth_ui_region = (new Region<ColorState>())->init(
		0.9,
		&time,
		to_rect(depth_ui.top_left, { depth_ui.WIDTH, depth_ui.height }),
		color_lerp,
		{ 0, depth_ui.BACKGROUND_COLOR },
		{ 0, { 90, 40, 40, 180 } },
		depth_ui.BACKGROUND_COLOR);

	depth_ui_region->adjust_t = adjust_t;
	depth_ui_region->render_fn = [](auto self) {
		DrawRectangleRec(self->bounds, self->color);
	};

	ui_elem_manager.add(depth_ui_region, "depth_ui_region");

	// A horz bar at the top of the screen
	{
		auto cam_depth_region = (new Region<ColorState>)->init(
			0.9,
			&time,
			get_cam_depth(screen_width),
			color_lerp,
			{ 0, ColorAlpha(DARKGRAY, 0.5) },
			{ 0, { 90, 40, 40, 180 } },
			depth_ui.BACKGROUND_COLOR);
		cam_depth_region->adjust_t = adjust_t;
		cam_depth_region->render_fn = [](auto self) {
			DrawRectangleRounded(self->bounds, 0.4, 1, self->color);
		};

		ui_elem_manager.add(cam_depth_region, "cam_depth_region");
	}

	// View selector
	{
		auto view_selector_region = (new Region<ColorState>)->init(
			0.9,
			&time,
			view_selector.bounds(),
			color_lerp,
			{ 0, view_selector.background_color },
			{ 0, { 90, 40, 40, 180 } },
			view_selector.background_color);
		view_selector_region->adjust_t = adjust_t;
		view_selector_region->render_fn = [&](auto self) {
			DrawRectangleRounded(self->bounds, view_selector.roundness, 1, self->color);
		};

		ui_elem_manager.add(view_selector_region, "view_selector_region");

		size_t vbn_i = 0;
		view_selector.iterate_views([&vbn_i, le = this, &vs = view_selector](auto view, auto bounds) {
			const std::string text = le->view_names[(int) view];
			
			auto vs_btn = new Button<ViewSelectorState>(
				{ view, le },
				bounds,
				text,
				[](auto& _) {},
				[](auto& self) {
					LevelEditor* le = self.data.level_editor;
					le->set_active(self.data.view, !le->get_active(self.data.view));
				},
				vs.view_color);
			vs_btn->render_fn = [&vs](auto ui_element) {
				auto& self = *dynamic_cast<Button<ViewSelectorState>*>(ui_element);
				const auto base_color = self.data.level_editor->get_active(self.data.view)
					? vs.view_selected_color
					: vs.view_color;
				const auto color = self.hovered && !self.data.level_editor->get_active(self.data.view)
					? ColorLerp(base_color, BLACK, 0.4)
					: base_color;
		
				DrawRectangleRounded(
					self.bounds,
					0.4,
					1,
					color);

				const int font_size = vs.calc_font_size();
				DrawText(self.state.text.c_str(), (int) self.bounds.x, (int) self.bounds.y, font_size, vs.font_color);
			};

			// le->view_button_names[vbn_i++] = le->ui_elem_manager.add(vs_btn, "vs_button");
			le->ui_elem_manager.add(vs_btn, "vs_button");
			le->view_buttons[vbn_i++] = vs_btn;
		});
	}
	
	// Two tools for your left mouse click to be doing; either placing a tendril config or placing a tree trunk segment
	{
		// Rn we just make the bounds be something random
		struct Config {
			std::string text;
			Color base_color;
			Color hovered_color;
		};
		const std::vector<Config> TOOL_CONFIGS = {
			{
				"Tendril Config",
				RED,
				ColorLerp(RED, YELLOW, 0.6)
			},
			{
				"Tree Trunk",
				ORANGE,
				ColorLerp(ORANGE, YELLOW, 0.5)
			},
			{
				"Move Selection",
				GREEN,
				ColorLerp(GREEN, YELLOW, 0.5)
			}
		};
		for (size_t i = 0; i < MouseTool::MOUSE_TOOL_SIZE; i++) {
			Rectangle bounds {
				300 + (float) i * 120,
				300,
				100,
				100
			};
			MouseTool::ToolType btn_tool_type = (MouseTool::ToolType) i;

			const auto [text, base_color, hover_color] = TOOL_CONFIGS[i];
			auto mouse_tool_btn = new Button<MouseTool::State>(
				{ &mouse_tool_used, btn_tool_type },
				bounds,
				text,
				[](auto& _) {},
				[](auto& self) {
					self.data.used->type = self.data.tool_type;
					std::println("Ran mousetoolbtn onpress, tool_type is now {}", (int) self.data.used->type);
				},
				base_color,
				hover_color,
				WHITE);

			// FIXED: hc = hover_color instead of &hc = hover_color
			mouse_tool_btn->render_fn = [&game, hc = hover_color](auto ui_element) {
				auto& self = *dynamic_cast<Button<MouseTool::State>*>(ui_element);
				self.hovered = self.hovered || self.data.used->type == self.data.tool_type;
				// Idk it needs some pop, so let's make the hover_color waver
				self.hover_color = ColorLerp(hc, YELLOW, 0.5 + 0.5 * sin(game.overall_time * 2 * PI));

				Button<MouseTool::State>::render_button(ui_element);
			};

			// mouse_tool_used.names[i] = ui_elem_manager.add(mouse_tool_btn, text);
			ui_elem_manager.add(mouse_tool_btn, text);
			mouse_tool_used.buttons[i] = mouse_tool_btn;
		}
	}

	// Extra buttons stuff 
	initialize_extra_buttons(game, screen_width, adjust_t, color_lerp);

	// Pivot point stuff
	{
		Vector2 pos = calc_default_pivot_point(game);
		auto btn = new Button<std::optional<Vector2>>(
			pos,
			to_rect(pos, { 20, 20 }),
			"",
			[](auto& _) {},
			[](auto& _) {
				std::println("Pivot point selected, TODO: Impl moving it");	
			},
			MAGENTA);
		btn->render_fn = [](UiElement* ui_element) {
			auto& self = *dynamic_cast<Button<std::optional<Vector2>>*>(ui_element);
			if (self.data)
				DrawRectangleRounded(self.bounds, 0.2, 1, self.color);
		};
		pivot_point_button = btn;
		ui_elem_manager.add(btn, "pivot_point_btn");
	}
}

Button<LevelEditor::DepthState>* LevelEditor::make_depth_button(const Rectangle& depth_rect, TendrilConfig::Id config_id) {
	auto depth_btn = new Button<LevelEditor::DepthState>(
		{ config_id, 0 },
		depth_rect,
		"",
		[](auto& _) {},
		[&](auto& self) {
			if (!is_selecting())
				return;

			const float MAX_DEPTH = 100;
			const float cursor_sidebar_y_pos = GetMousePosition().y;
			const float cursor_depth = (cursor_sidebar_y_pos - depth_ui.SPACING) / depth_ui.height * MAX_DEPTH;
			auto& config = all_config_info[from_selected_by_id((TendrilConfig::Id) self.data.id)].ptr;

			if (!self.state.last_hit)
				self.data.selection_offset = cursor_depth - config->depth;

			const float true_depth = std::min(MAX_DEPTH, std::max(0.0f, cursor_depth - self.data.selection_offset));
			config->depth = true_depth;
			self.bounds = update_config_for_depth_ui(*config);
		},
		ORANGE);

	depth_btn->render_fn = [&](UiElement* ui_element) {
		auto& self = *dynamic_cast<Button<DepthState>*>(ui_element);
		Color color = depth_ui.MARK_COLOR;

		const bool is_selected = from_selected_by_id(self.data.id) != -1;
		if (is_selected) {
			// Then draw a triangle pointer too, idk
			Vector2 leftmost {
				self.bounds.x + self.bounds.width + 5,
				self.bounds.y + self.bounds.height / 2
			};
			const float tri_width = 10;
			const float tri_height = 6;
			DrawTriangle(leftmost, 
				leftmost + Vector2 { tri_width,  tri_height / 2 }, 
				leftmost + Vector2 { tri_width, -tri_height / 2 },
				RED);

			color = ColorLerp(self.color, RED, 0.7);
		}

		if (self.in_use())
			color = ColorLerp(color, { 0, 90, 150, 170 }, 0.5);
		
		const float SEL_BUFFER = 0.15;
		Rectangle render_bounds {
			self.bounds.x,
			self.bounds.y + depth_ui.MARK_HEIGHT * SEL_BUFFER,
			self.bounds.width,
			self.bounds.height - depth_ui.MARK_HEIGHT * SEL_BUFFER * 2
		};

		if (get_active(FocusTreeView)) {
			if (!is_selected) {
				color = ColorAlpha(color, color.a * 0.3);
			}
		}

		DrawRectangle((int) render_bounds.x, (int) render_bounds.y, (int) render_bounds.width, (int) render_bounds.height, color);
	};

	return depth_btn;
}
