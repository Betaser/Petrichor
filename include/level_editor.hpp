#ifndef LEVEL_EDITOR_H
#define LEVEL_EDITOR_H

#include "game.hpp"
#include "button.hpp"
#include <vector>

struct TreeMetadata {
	// Rotation affects all tendrils
	std::vector<Branch> branches;
	Vector2 offset {};
	float rotation = 0;
	Rectangle mark {};

	static TreeMetadata zero() {
		return {};
	}
	TreeMetadata(float rotation, Vector2 offset, Tree& tree, Rectangle mark) {
		std::cout << "init tree metadata\n";
		this->rotation = rotation;
		this->offset = offset;
		this->mark = mark;

		branches.reserve(tree.branches.size());
		for (auto& branch : tree.branches)
			branches.emplace_back(branch);
	}
	~TreeMetadata() {
		std::cout << "deinit tree metadata\n";
	}

	private:
	TreeMetadata() {}
};

struct LevelEditor : public Button::Owner {
	std::vector<Button> buttons;
	Button* debug_button;
	std::vector<TreeMetadata> tree_metadatas;
	size_t selected_index = 0;
	size_t last_selected_index = 0;
	std::vector<size_t> deleted_tree_ids;
	TextureWithCheck selected_tex;
	Vector2 selection_offset {};
	float time = 0;
	bool using_depth_ui = 0;

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
	} depth_ui;

	LevelEditor();
	~LevelEditor();

	// if not sure about tree_maker, use game.make_tree();
	void make_initialized_tree(std::function<void()> tree_maker, Game& game, const TreeMetadata& metadata);
	void initialize_ui();
	void randomize_tendrils(Game& game);
	void update_selected_verts(Game& game);
	void load_selection_shader(Game& game);
	void update(Game& game);
	void render(Game& game) const;

	void duplicate_selected_tree(Game& game);

	private:
	bool show_instructions = false;
	Vector2 select_extra_bounds { 10, 10 };
	ShaderWithCheck select_shader;

	// Does a full recalculation for every tree, but eh.
	Rectangle update_tree_for_depth_ui(Game& game, size_t tree_index);
	void render_depth_ui(size_t selected_id) const;
	bool is_selecting(Game& game) const;
	void invalidate_selected_index(Game& game);
};

#endif