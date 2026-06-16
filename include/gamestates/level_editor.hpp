#ifndef LEVEL_EDITOR_H
#define LEVEL_EDITOR_H

#include <vector>

#include "../globals/game.hpp"
#include "../scene_elements/button.hpp"
#include "tree_metadata.cpp"

struct LevelEditor : public Button::Owner {
	std::vector<std::unique_ptr<Tree>> saved_trees;
	std::vector<Button> buttons;
	Button* debug_button;
	std::vector<TreeMetadata> tree_metadatas;
	size_t selected_index = 0;
	std::vector<size_t> deleted_tree_ids;
	TextureWithCheck selected_tex;
	Vector2 selection_offset {};
	float time = 0;
	bool using_depth_ui = false;
	bool using_view_selector = false;

	struct DepthUi {
		const float WIDTH = 30;
		const float SPACING = 25;
		const float MAX_DEPTH = 100;
		float height = -9999;
		Vector2 top_left { -9999, -9999 };
		float y_pos = -9999;
		const Color BACKGROUND_COLOR { 0, 0, 30, 255 };
		const int MARK_SPACING = -5;
		const int MARK_HEIGHT = 5;
		const Color MARK_COLOR { 255, 255, 200, 255 };
	};
	DepthUi depth_ui;

	// Let's see if enum non-class is enough
	enum View {
		// Pulse red for selected trees
		SelectedView,
		// (TODO) Can click and drag anywhere to alter the camera
		CameraView,
		// Only shows selected trees and scrolling now shows each branch sorted by depth
		FocusTreeView,
		SIZE,
	};
	static constexpr std::array<const std::string, SIZE> view_names {
		"Highlight Selections",
		"Camera Panning",
		"Focus Selections",
	};
	std::array<View, SIZE> views;

	// Bottom left view selector
	struct ViewSelector {
		float view_height = 50;
		float view_width = 150;
		float view_top_padding = 10;
		float view_left_padding = 10;
		float btm_padding = 50;
		float left_padding = 50;
		float width = view_width + 2 * view_left_padding;
		float height = (view_height + view_top_padding) 
			* (float) SIZE + view_top_padding;
		float roundness = 0.3;
		int* screen_height = nullptr;

		// Dark gray
		Color background_color { .r = 40, .g = 40, .b = 40, .a = 100 };
		Color font_color = WHITE;
		// Orange
		Color view_color { .r = 240, .g = 178, .b = 10, .a = 100 };

		int calc_font_size() const {
			// ???
			return 15;
		}

		void iterate_views(std::function<void(const View, Rectangle dims)> func) const {
			for (int i = 0; i < SIZE; i++) {
				const float view_delta_y = (float) i * (view_top_padding + view_height);
				const Rectangle dims {
					.x = left_padding + view_left_padding,
					.y = (float) *screen_height - btm_padding - height + view_top_padding + view_delta_y,
					.width = view_width,
					.height = view_height
				};
				func((View) i, dims);
			}
		}

		Rectangle bounds() const {
			const float posX = left_padding;
			// Background
			const float posY = (float) *screen_height - btm_padding - height;
			return { posX, posY, width, height };
		}

		void render() const {
			DrawRectangleRounded(bounds(), roundness, 1, background_color);

			// Views
			iterate_views([this](auto view, auto dims) {
				DrawRectangleRounded(
					dims,
					0.4,
					1,
					view_color);
				const int font_size = calc_font_size();
				const std::string text = view_names[(int) view];
				DrawText(text.c_str(), (int) dims.x, (int) dims.y, font_size, font_color);
				});
		}
	};
	ViewSelector view_selector;

	LevelEditor(int* scren_height);
	~LevelEditor();

	// if not sure about tree_maker, use game.make_tree();
	void make_initialized_tree(std::function<void()> tree_maker, Game& game, const TreeMetadata& metadata);
	void initialize_ui();
	void randomize_tendrils(Game& game, size_t tree_index);
	void update_selected_verts(Game& game);
	void init_selection_texture();
	void update(Game& game);
	void render(Game& game) const;
	void invalidate_selected_index(Game& game);

	void duplicate_selected_tendril(Game& game);

	private:
	bool show_instructions = false;
	Vector2 select_extra_bounds { 10, 10 };
	ShaderWithCheck select_shader;
	float cam_depth = 0;
	float min_cam_depth;
	float max_cam_depth;
	bool focus_on_selected = false;

	// Does a full recalculation for every tree, but eh.
	Rectangle update_tree_for_depth_ui(Game& game, Tree& tree);
	void render_depth_ui(size_t selected_id) const;
	void render_cam_depth(Game& game) const;
	bool is_selecting(Game& game) const;
	void adjust_cam_depth(Game& game);
	std::string convert_trees_to_chars(std::vector<std::unique_ptr<Tree>>& trees) const;
};

#endif
