#ifndef LEVEL_EDITOR_H
#define LEVEL_EDITOR_H

#include <vector>
#include <bitset>

#include "../globals/game.hpp"
#include "../scene_elements/button.hpp"
#include "../scene_elements/ui_element_manager.cpp"
#include "tree_metadata.cpp"

struct LevelEditor : public Button::Owner {
	struct Selection {
		size_t index;
		Vector2 offset;
	};
	std::vector<std::unique_ptr<Tree>> saved_trees;
	UiElementManager ui_elem_manager;
	std::map<size_t, std::string> tree_to_managed_buttons;
	std::vector<TreeMetadata> tree_metadatas;
	std::vector<Selection> selections;
	std::vector<size_t> deleted_tree_ids;
	TextureWithCheck selected_tex;
	bool ui_hovered = false;
	float time = 0;

	struct DepthUi {
		const float WIDTH = 30;
		const float SPACING = 25;
		const float MAX_DEPTH = 100;
		float height = -9999;
		Vector2 top_left { -9999, -9999 };
		const Color BACKGROUND_COLOR { 0, 0, 30, 95 };
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

	std::array<std::string, SIZE> view_names {
		"Highlight Selections",
		"Camera Panning",
		"Focus Selections",
	};
	std::array<std::string, SIZE> view_button_names;
	std::bitset<SIZE> views_active { 0 };

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
		// Yellow
		Color view_selected_color { .r = 255, .g = 210, .b = 0, .a = 200 };

		int calc_font_size() const;
		void iterate_views(std::function<void(const View, Rectangle dims)> func) const;
		Rectangle bounds() const;
	};

	ViewSelector view_selector;

	LevelEditor(Game& game);
	~LevelEditor();

	void reinit(Game& game);
	// if not sure about tree_maker, use game.make_tree();
	void make_initialized_tree(std::function<void()> tree_maker, Game& game, const TreeMetadata& metadata);
	void initialize_ui(const int screen_width);
	void randomize_tendrils(Game& game, const size_t tree_index);
	void update_selected_verts(Game& game);
	void init_selection_texture();
	void update(Game& game);
	void render(Game& game) const;
	void invalidate_selections();

	void duplicate_selected_tendril(Game& game);

	private:
	std::string debug_btn_str;
	bool show_instructions = false;
	Vector2 select_extra_bounds { 10, 10 };
	ShaderWithCheck select_shader;
	float cam_depth = 0;
	float min_cam_depth;
	float max_cam_depth;

	Rectangle get_cam_depth(const int screen_width) const;
	void render_cam_depth(Game& game) const;
	void adjust_cam_depth(Game& game);
	std::string convert_trees_to_chars(std::vector<std::unique_ptr<Tree>>& trees) const;

	bool is_selecting() const;
	void set_active(View view, bool active);
	bool get_active(View view) const;
	// Does a full recalculation for every tree, but eh.
	Rectangle update_tree_for_depth_ui(Game& game, const Tree& tree);
	Button* make_depth_button(const Rectangle& depth_rect, Game& game, const size_t tree_id);
	bool find_cursor_selection(Game& game, Vector2 cursor, Selection* selection);
	void tree_single_select(Game& game, Vector2 mouse_pos);
	void tree_multi_select(Game& game, Vector2 mouse_pos);
	void update_tree_verts(Game& game, const size_t tree_index);
	bool contains_selection(const size_t index) const;
	int from_selected_by_id(Game& game, const size_t tree_id) const;
};

#endif
