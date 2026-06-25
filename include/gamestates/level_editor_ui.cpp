#include "level_editor.hpp"
#include "../scene_elements/region.cpp"

typedef LevelEditor::ViewSelector ViewSelector;

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

void LevelEditor::initialize_ui(const int screen_width) {
	auto debug_btn = new Button(
		this,
		to_rect({ (float) screen_width - 190, 110 }, { 80, 80 }),
		"Show debug keybinds",
		[](auto& b) {
			b.idle_state.text = "Press me to toggle instructions";
		},
		[](auto& b) {
			auto owner = dynamic_cast<LevelEditor*>(b.state.owner);
			owner->show_instructions = !owner->show_instructions;
		});
	debug_btn_str = ui_elem_manager.add(std::unique_ptr<UiElement>(debug_btn), "debug_btn");

	typedef TRegion<Color>::TState ColorState;
	auto color_lerp = [](auto& self, auto& curr, auto& target, float t) {
		self.color = ColorLerp(target.hover_data, curr.hover_data, t);
	};
	auto adjust_t = [](float t) {
		return t * t;
	};

	auto depth_ui_region = new TRegion<Color>(
		0.9,
		&time,
		to_rect(depth_ui.top_left, { depth_ui.WIDTH, depth_ui.height }),
		color_lerp,
		std::make_unique<ColorState>(0, depth_ui.BACKGROUND_COLOR),
		std::make_unique<ColorState>(0, Color { 90, 40, 40, 180 }),
		depth_ui.BACKGROUND_COLOR);

	depth_ui_region->adjust_t = adjust_t;
	depth_ui_region->render_fn = [](auto self) {
		DrawRectangleRec(self->bounds, self->color);
	};

	ui_elem_manager.add(std::unique_ptr<UiElement>(depth_ui_region), "depth_ui_region");

	// A horz bar at the top of the screen
	{
		auto cam_depth_region = new TRegion<Color>(
			0.9,
			&time,
			get_cam_depth(screen_width),
			color_lerp,
			std::make_unique<ColorState>(0, ColorAlpha(DARKGRAY, 0.5)),
			std::make_unique<ColorState>(0, Color { 90, 40, 40, 180 }),
			depth_ui.BACKGROUND_COLOR);
		cam_depth_region->adjust_t = adjust_t;
		cam_depth_region->render_fn = [](auto self) {
			DrawRectangleRounded(self->bounds, 0.4, 1, self->color);
		};

		ui_elem_manager.add(std::unique_ptr<UiElement>(cam_depth_region), "cam_depth_region");
	}

	// View selector
	{
		auto view_selector_region = new TRegion<Color>(
			0.9,
			&time,
			view_selector.bounds(),
			color_lerp,
			std::make_unique<ColorState>(0, view_selector.background_color),
			std::make_unique<ColorState>(0, Color { 90, 40, 40, 180 }),
			view_selector.background_color);
		view_selector_region->adjust_t = adjust_t;
		view_selector_region->render_fn = [&](auto self) {
			DrawRectangleRounded(self->bounds, view_selector.roundness, 1, self->color);
		};

		ui_elem_manager.add(std::unique_ptr<UiElement>(view_selector_region), "view_selector_region");

		size_t vbn_i = 0;
		view_selector.iterate_views([&, &vs = view_selector](auto view, auto bounds) {
			const std::string text = view_names[(int) view];
			auto vs_btn = new Button(
				nullptr,
				bounds,
				text,
				[](auto& _) {},
				[&, view](auto& self) {
					auto b = dynamic_cast<Button&>(self);
					std::println("my text: {}", b.state.text);
					set_active(view, !get_active(view));
				},
				vs.view_color);

			vs_btn->render_fn = [&, text, view](auto self) {
				const auto base_color = get_active(view)
					? vs.view_selected_color
					: vs.view_color;
				const auto color = self->hovered
					? ColorLerp(base_color, BLACK, 0.4)
					: base_color;
		
				DrawRectangleRounded(
					self->bounds,
					0.4,
					1,
					color);

				const int font_size = vs.calc_font_size();
				DrawText(text.c_str(), (int) self->bounds.x, (int) self->bounds.y, font_size, vs.font_color);
			};

			view_button_names[vbn_i++] = ui_elem_manager.add(std::unique_ptr<UiElement>(vs_btn), "vs_button");
		});
	}
}

Button* LevelEditor::make_depth_button(const Rectangle& depth_rect, Game& game, const size_t tree_id) {
	auto depth_btn = new Button(
	nullptr,
		depth_rect,
		"",
		[](auto& _) {},
		[&](auto& self) {
			if (!is_selecting(game))
				return;

			const float MAX_DEPTH = 100;
			const float cursor_sidebar_y_pos = std::min(depth_ui.SPACING + depth_ui.height, std::max(depth_ui.SPACING, GetMousePosition().y));
			auto& selected = game.trees[selected_index];
			selected->depth = (cursor_sidebar_y_pos - depth_ui.SPACING) / depth_ui.height * MAX_DEPTH;
			self.bounds = update_tree_for_depth_ui(game, *selected);
		},
		ORANGE);

	depth_btn->render_fn = [&, tree_id](auto ui_element) {
		const auto& self = *dynamic_cast<const Button*>(ui_element);
		Color color = depth_ui.MARK_COLOR;
		if (is_selecting(game)) {
			const size_t selected_id = game.trees[selected_index]->id;
			if (tree_id == selected_id)
				color = ColorLerp(self.color, depth_ui.MARK_COLOR, 0.7);
		}
		if (self.in_use())
			color = ColorLerp(color, { 0, 90, 150, 170 }, 0.5);
		
		Rectangle render_bounds {
			self.bounds.x,
			self.bounds.y + 4,
			self.bounds.width,
			self.bounds.height - 8
		};
		DrawRectangle((int) render_bounds.x, (int) render_bounds.y, (int) render_bounds.width, (int) render_bounds.height, color);
	};

	return depth_btn;
}
