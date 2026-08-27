#ifndef LEVEL_EDITOR_H
#define LEVEL_EDITOR_H

#include <vector>
#include <bitset>

#include "../globals/game.hpp"
#include "../scene_elements/button.hpp"
#include "../scene_elements/ui_element_manager.cpp"
#include "tree_metadata.cpp"
#include "../scene_elements/region.cpp"

struct LevelEditor {
	struct Selection {
		size_t index;
		Vector2 offset;
	};

	struct DepthUi {
		const float WIDTH = 30;
		const float SPACING = 25;
		const float MAX_DEPTH = 100;
		float height = -9999;
		Vector2 top_left { -9999, -9999 };
		const Color BACKGROUND_COLOR { 0, 0, 30, 95 };
		const int MARK_HEIGHT = 20;
		const Color MARK_COLOR { 255, 255, 200, 255 };
	};

	enum View {
		// Pulse red for selected tendril configs
		SelectedView,
		// (TODO) Can click and drag anywhere to alter the camera
		CameraView,
		// Only shows the trees that selected configs belong to
		// (TODO) and scrolling now shows each branch sorted by depth, so scrolling jumps by depth
		FocusTreeView,
		SIZE,
	};

	struct ViewSelectorState {
		View view;
		LevelEditor* level_editor;
	};

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

	struct DepthState {
		TendrilConfig::Id id;
		float selection_offset;
	};

	std::vector<std::unique_ptr<Tree>> saved_trees;
	UiElementManager ui_elem_manager;
	std::map<TendrilConfig::Id, std::string> config_to_managed_buttons;
	std::vector<BranchMetadata> branch_metadatas;
	std::vector<Selection> selections;
	std::vector<TendrilConfig::Id> deleted_config_ids;
	TextureWithCheck selected_tex;
	bool ui_hovered = false;
	float time = 0;
	DepthUi depth_ui;
	ViewSelector view_selector;

	std::array<std::string, SIZE> view_names {
		"Highlight Selections",
		"Camera Panning",
		"Focus Trees",
	};
	// Prefer to use normal ptr collections unless using more features of the element manager are desired.
	// std::array<std::string, SIZE> view_button_names;
	std::array<Button<ViewSelectorState>*, SIZE> view_buttons;
	std::bitset<SIZE> views_active { 0 };

	LevelEditor(Game& game);
	~LevelEditor();

	void reinit(Game& game);
	void make_initialized_config(Tree& tree, const Rand& rand, const BranchMetadata& metadata);
	void randomize_tendrils(TendrilConfig& config, const BranchMetadata& meta);
	void randomize_tendrils(size_t config_index);
	void update_selected_verts();
	void init_selection_texture();
	void update(Game& game);
	void render(Game& game) const;
	void invalidate_selections();
	void duplicate_selected_tendril();

	private:
	enum ExtraButtonState {
		// Handle the none case without using an enum member
		OnCameraView, // Go to Petra
		OnMultipleSelected, // Merge trees
		None,
		EXTRA_SIZE
	};

	struct ColorState {
		float last_hover_time;
		Color hover_color;
	};
	using ColorStateLerpFn = std::function<void(Region<ColorState>&, Region<ColorState>::State&, Region<ColorState>::State&, float)>;

	// Initialized while "initialize_ui" runs.
	struct ExtraButtonManager {
		ExtraButtonState active_state = None;
		std::array<UiElementGroup, EXTRA_SIZE> extra_state_to_group;
		std::string region_name;
		LevelEditor* owner;

		void set_hit_state_true();
		void set_active_extra_button_group(ExtraButtonState button_state);
	};

	struct ConfigInfo {
		TendrilConfig* ptr;
		size_t tree_owner_index;
	};

	struct MouseTool {
		enum ToolType {
			PlaceTendrilConfig,
			PlaceTreeTrunk,
			// Selection are prioritized
			SelectionCentric,
			MOUSE_TOOL_SIZE
		};
		struct State {
			MouseTool* used;
			MouseTool::ToolType tool_type;
		};
		std::array<Button<State>*, MOUSE_TOOL_SIZE> buttons;

		// A sensible default.
		ToolType type = SelectionCentric;
	};

	ExtraButtonManager extra_button_manager;
	Button<LevelEditor*>* debug_button;
	// Don't need to have this around tbh, should be part of the ui_elem_manager
	bool show_instructions = false;
	Vector2 select_extra_bounds { 10, 10 };
	ShaderWithCheck select_shader;
	float cam_depth = 0;
	float min_cam_depth;
	float max_cam_depth;
	std::vector<ConfigInfo> all_config_info;
	std::unique_ptr<TendrilConfig> mouse_tool_preview_config = nullptr;
	MouseTool mouse_tool_used;
	bool mouse_tool_permits_selection_movement = false;

	Rectangle get_cam_depth(const int screen_width) const;
	void render_cam_depth(Game& game) const;
	void adjust_cam_depth();
	std::string trees_to_chars(std::vector<std::unique_ptr<Tree>>& trees) const;

	bool is_selecting() const;
	void set_active(View view, bool active);
	bool get_active(View view) const;
	// Does a full recalculation for every tree, but eh.
	Rectangle update_config_for_depth_ui(const TendrilConfig& config);
	bool is_cursor_hovering_selection(Vector2 cursor) const;
	bool find_cursor_selection(Vector2 cursor, Selection* selection);
	void tree_single_select(Vector2 mouse_pos);
	void tree_multi_select(Vector2 mouse_pos);
	void branch_verts_from_metadata(TendrilConfig& config, const BranchMetadata& meta);
	void branch_verts_from_metadata(size_t config_index);
	bool contains_selection(size_t index) const;
	int from_selected_by_id(TendrilConfig::Id config_id) const;
	void delete_config(size_t config_index);

	// Impl extracted out to level_editor_ui.cpp
	void initialize_ui(Game& game);

	void initialize_extra_buttons(Game& game, int screen_width, std::function<float(float)> adjust_t, ColorStateLerpFn color_lerp);
	Button<DepthState>* make_depth_button(const Rectangle& depth_rect, TendrilConfig::Id config_id);
	std::vector<size_t> calc_depth_indices() const;
};

#endif
