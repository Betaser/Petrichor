#include <iostream>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <algorithm>
#include <format>

#include "mylib.hpp"
#include "level_editor.hpp"
#include "constants.cpp"
#include "tree_metadata.cpp"

LevelEditor::LevelEditor() {
	debug_button = nullptr;
	show_instructions = false;
	std::println("init level editor");
	time = 0;
	selected_index = 0;
	using_depth_ui = false;
	last_selected_index = selected_index;

	load_shader(select_shader, "assets/select.fs");
	init_selection_texture();
	initialize_ui();
}

LevelEditor::~LevelEditor() {
	std::println("deinit level editor");
	unload_texture(selected_tex);
	unload_shader(select_shader);
	std::println("select shader w/ id {} loads/unloads {}", select_shader.id, select_shader.load_unloads);
	std::println("selected_tex w/ id {} loads/unloads {}", selected_tex.id, selected_tex.load_unloads);
}

void LevelEditor::make_initialized_tree(std::function<void()> tree_maker, Game& game, const TreeMetadata& metadata) {
	tree_maker();
	auto& tree = *game.trees.back();
	// std::println("make tree w/ id {}", tree.id);

	bool ids_available = !deleted_tree_ids.empty();
	if (ids_available) {
		size_t popped_id = deleted_tree_ids[0];
		deleted_tree_ids.erase(deleted_tree_ids.begin());
		tree.id = popped_id;
	}

	TreeMetadata temp(metadata.rotation, metadata.offset, update_tree_for_depth_ui(game, *game.trees[game.trees.size() - 1]));
	if (ids_available)
		tree_metadatas[tree.id] = temp;
	else
		tree_metadatas.push_back(temp);

	randomize_tendrils(game, game.trees.size() - 1);

	// Now we're gonna add the trunk.
	TrunkSegment segment {
		.top {
			.depth = 0,
			.position { 300, 300 },
			.radius = 20
		},
		.bottom {
			.depth = 100,
			.position { 300, 200 },
			.radius = 15
		}
	};
	tree.trunk_segments.emplace_back(segment);
}

void LevelEditor::initialize_ui() {
	buttons.clear();
	int screenWidth = 800;
	auto debug_btn = Button(
		this,
		{ (float) screenWidth - 190, 110 }, 
		{ 80, 80 }, 
		"Show debug keybinds",
		[](Button& b) {
			b.state.text = "Press me to toggle instructions";
		},
		[](Button& b) {
			auto owner = dynamic_cast<LevelEditor*>(b.state.owner);
			owner->show_instructions = !owner->show_instructions;
		});
	buttons.emplace_back(debug_btn);
	debug_button = &buttons[0];
}

void LevelEditor::randomize_tendrils(Game& game, size_t tree_index) {
	// Try using randomly generated tendrils too
	const Vector2 start_location { 100, 100 };

	auto& tree = game.trees[tree_index];
	std::vector<std::vector<Branch>> tendrils = tree->random_tendril_config(400, 20, 1.2, 0.1, start_location);
	tree->branches = Tree::branches_from_tendrils(tendrils);
	tree->tendrils = tendrils;

	const auto& meta = tree_metadatas[tree->id];
	tree_metadatas[tree->id] = TreeMetadata(meta.rotation, meta.offset, meta.mark);
	tree->on_updated_branch();

	update_selected_verts(game);

	tree->update_texture();
}

void LevelEditor::update_selected_verts(Game& game) {
	auto& tree = game.trees[selected_index];
	auto& meta = tree_metadatas[tree->id];
	// std::println("meta branch origin {}", to_str(branches[0].back(), 2));

	const auto& branches = tree->original_branches;

	if (branches.size() != tree->branches.size())
		std::cerr << "metadata branches size " << branches.size() << " selected branches size " << tree->branches.size() << "\n";

	// TODO: Use step function instead?
	// const float rotation = floor(meta.rotation / (2.0 * PI / 30)) * (2.0 * PI / 30);
	const float rotation = snap(meta.rotation, 2.0 * PI / 30);

	for (size_t i = 0; i < branches.size(); i++) {
		auto& sel_verts = tree->branches[i].verts;
		const auto& verts = branches[i].verts;
		for (size_t j = 0; j < verts.size(); j++) {
			sel_verts[j] = rotate(branches[0].back(), verts[j], rotation) + meta.offset;
		}
	}
}

// Selection is slightly larger than size of tree texture.
void LevelEditor::init_selection_texture() {
	std::println("init selection texture");

	auto blank = GenImageColor(10, 10, BLANK);
	load_texture_from_image(selected_tex, blank);
	UnloadImage(blank);
}

void LevelEditor::update(Game& game) {
	time += GetFrameTime();

	const bool selecting = is_selecting(game);

	// Right click to select, chooses closest tree
	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
		auto mouse_pos = GetMousePosition();
		float shortest = INFINITY;
		for (size_t i = 0; i < game.trees.size(); i++) {
			const auto& tree = game.trees[i];
			Vector2 small, big;
			tree->bounding_box(small, big);
			auto mid = (small + big) / 2;
			float dist = length(mid - mouse_pos);
			if (dist < shortest) {
				selected_index = i;
				shortest = dist;
			}
		}
	}

	bool using_ui = using_depth_ui;
	// Yes, let's eventually move this button checking bounds to a designated class
	const bool using_debug_btn_ui = debug_button->state.hovered;
	if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
		using_ui |= using_debug_btn_ui;

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
		if (using_debug_btn_ui)
			debug_button->state.hit = true;
	}

	if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) {
		std::println("Save trees to {}", Constants::test_level_path);

		// Move to a function probably
		auto repr = convert_trees_to_chars(game.trees);
		std::ofstream file;
		file.open(Constants::test_level_path);
		file << repr;
		file.close();
	}

	if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)) {
		// And, metadatas are zeroed out.
		tree_metadatas.clear();
		std::println("Load file {}", Constants::test_level2_path);
		game.load_trees(
			Constants::test_level2_path, 
			[&](TreeMetadata& meta, Tree& tree) {
				meta.mark = update_tree_for_depth_ui(game, tree);
				tree_metadatas.push_back(meta);	
			});

		// Maybe metadata is bad, print it out:
		for (const auto& meta : tree_metadatas) {
			std::println("meta: offset {} rot {}", to_str(meta.offset, 2), meta.rotation);
		}
		for (size_t i = 0; i < game.trees.size(); i++) {
			selected_index = i;
			update_selected_verts(game);
			game.trees[i]->update_texture();
		}
		invalidate_selected_index(game);

		return;
	}

	if (selecting) {
		// Selected tree is not a thing yet.
		auto& selected = game.trees[selected_index];
		auto& meta = tree_metadatas[selected->id];

		if (!using_ui && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			selection_offset = GetMousePosition() - meta.offset;

		// Debug testing
		if (IsKeyPressed(KEY_Q)) {
			// Delete all but the first tree
			const size_t selected_id = selected->id;
			game.trees.erase(game.trees.begin() + 1, game.trees.end());
			deleted_tree_ids.emplace_back(selected_id);
			invalidate_selected_index(game);
			return;
		}

		// Deletion, should be tough
		if (IsKeyPressed(KEY_BACKSPACE) && game.trees.size() > 1) {
			const size_t selected_id = selected->id;
			// Make sure to extract everything you need from selected BEFORE erasing it
			game.trees.erase(game.trees.begin() + selected_index);
			deleted_tree_ids.emplace_back(selected_id);

			// Do NOT delete tree_metadatas, it gets reused.
			invalidate_selected_index(game);

			std::println("deleted {}", selected_id);
			return;
		}

		// Change depth, by clicking close enough to the mark and moving your mouse while holding click
		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
			Rectangle r = meta.mark;
			// Grow r a bit
			float margin = 5;
			r.x -= margin;
			r.y -= margin;
			r.width += 2 * margin;
			r.height += 2 * margin;
			if (pt_in_rect(GetMousePosition(), { r.x, r.y }, { r.width, r.height }))
				using_depth_ui = true;
		} 
		else
			using_depth_ui = false;

		if (using_depth_ui) {
			// Then make depth move to your mouse, and call update
			// I suppose we make the depth go from 0 at the top to like 100 at the bottom?
			const float MAX_DEPTH = 100;
			float clamped_sidebar_y_pos = std::min(depth_ui.SPACING + depth_ui.height, std::max(0.0f, GetMousePosition().y));
			selected->depth = (clamped_sidebar_y_pos - depth_ui.SPACING) / depth_ui.height * MAX_DEPTH;

			meta.mark = update_tree_for_depth_ui(game, *game.trees[selected_index]);
		}

		// Duplicate. Means we copy over metadata
		if (IsKeyPressed(KEY_F)) {
			duplicate_selected_tendril(game);
			// Don't want to deal with selection having changed during this if statement affecting expectations for the rest of this function
			return;
		}

		// rotation
		float rotation_input = 0;
		if (IsKeyDown(KEY_A))
			rotation_input = -0.05;
		if (IsKeyDown(KEY_D))
			rotation_input = 0.05; 

		meta.rotation += rotation_input;

		if (rotation_input != 0) {
			update_selected_verts(game);
			selected->update_texture();
		}

		// Adapted from main's while loop
		if (IsKeyPressed(KEY_R)) {
			selected->rand.set_seed(++selected->rand.seed);
			randomize_tendrils(game, selected_index);
		}

		// offset
		if (!using_ui && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
			meta.offset = GetMousePosition() - selection_offset;

			update_selected_verts(game);
			// just translate it instead of reloading shaders.
			Vector2 small, big;
			selected->bounding_box(small, big);
			selected->texture_pos = small;
			auto tree_tex_bounds = (Vector2I { selected->blank_tex.width, selected->blank_tex.height }).to_vec2();
			tree_tex_bounds += select_extra_bounds;
		}

		if (last_selected_index != selected_index)
			last_selected_index = selected_index;
	}
}

void LevelEditor::render(Game& game) const {
	if (is_selecting(game)) {
		int loc = GetShaderLocation(select_shader, "time");
		SetShaderValue(select_shader, loc, &time, SHADER_UNIFORM_FLOAT);

		auto& tree = game.trees[selected_index];
		Vector2 pos { tree->texture_pos - select_extra_bounds / 2 };
		Vector2 small, big;
		tree->bounding_box(small, big);
		auto dims = big - small + select_extra_bounds;

		int dims_loc = GetShaderLocation(select_shader, "dims");
		auto dims_i = Vector2I(dims);
		SetShaderValue(select_shader, dims_loc, &dims_i, SHADER_UNIFORM_IVEC2);

		BeginShaderMode(select_shader);
		DrawTexturePro(
			selected_tex,
			// source rect
			{
				.x = 0,
				.y = 0,
				.width = (float) selected_tex.width,
				.height = (float) selected_tex.height
			},
			// dest rect
			{
				.x = pos.x,
				.y = pos.y,
				.width = dims.x,
				.height = dims.y
			},
			{},
			0,
			WHITE);
		EndShaderMode();
	}

	if (show_instructions) {
		std::stringstream ss; ss
		<< "F = duplicate\n"
		<< "Right click to select\n"
		<< "R = randomize seed\n"
		<< "A = rotate counterclockwise\n"
		<< "D = rotate clockwise\n"
		<< "Backspace = delete\n"
		<< "Click on marks on sidebar to change depth\n"
		<< "Ctrl + S = save to " << Constants::test_level_path << "\n"
		<< "Ctrl + O = open " << Constants::test_level2_path << "\n"
		<< "TODO: G = guidelines (editor add ons.\n"
		<< "which are saved separate from level data)";
		std::string s_str = ss.str();
		unsigned char opacity = 255 * (0.3 * (0.5 * sin(time * 3.0) + 0.5) + 0.7);
		const int font_size = 30;
		int text_size = MeasureText(s_str.c_str(), font_size);
		DrawText(s_str.c_str(), 400 - text_size / 2, 80, font_size, { 255, 70, 70, opacity });
	}

	size_t id = is_selecting(game) ? game.trees[selected_index]->id : selected_index;
	render_depth_ui(id);
}

Rectangle LevelEditor::update_tree_for_depth_ui(Game& game, Tree& tree) {
	depth_ui.height = (float) game.screen_height - 2 * depth_ui.SPACING;
	depth_ui.top_left = {
		(float) game.screen_width - depth_ui.WIDTH - depth_ui.SPACING,
		depth_ui.SPACING
	};

	std::vector<float> depths(game.trees.size());
	for (size_t i = 0; i < game.trees.size(); i++)
		depths[i] = game.trees[i]->depth;
	
	std::sort(depths.begin(), depths.end());

	const float min_depth = std::min(0.0f, depths[0]);
	const float max_depth = std::max(depth_ui.MAX_DEPTH, depths.back());

	const float depth = tree.depth;
	const int spacing = -5;
	const int height = 5;

	float percent = (depth - min_depth) / (max_depth - min_depth);
	const float y_pos = percent * depth_ui.height + depth_ui.SPACING - height / 2;
	depth_ui.y_pos = y_pos;

	return { 
		.x = depth_ui.top_left.x + spacing, 
		.y = y_pos, 
		.width = depth_ui.WIDTH - 2 * spacing, 
		.height = height 
	};
}

void LevelEditor::render_depth_ui(size_t selected_id) const {
	DrawRectangle(depth_ui.top_left.x, depth_ui.top_left.y, 
		depth_ui.WIDTH, depth_ui.height, depth_ui.BACKGROUND_COLOR);

	std::vector<size_t> tree_ids = deleted_tree_ids;
	std::sort(tree_ids.begin(), tree_ids.end());
	size_t id_i = 0;

	for (size_t i = 0; i < tree_metadatas.size(); i++) {
		// We need to figure out what parts of tree_metadatas are not in use.
		if (0 <= id_i && id_i < tree_ids.size() && tree_ids[id_i] == i) {
			id_i++;
			continue;
		}

		Rectangle r = tree_metadatas[i].mark;
		// For now, color differently. Could use a shader maybe.
		if (i == selected_id) {
			DrawRectangle(r.x, r.y, r.width, r.height, ColorLerp(ORANGE, depth_ui.MARK_COLOR, 0.7));
		} 
		else {
			DrawRectangle(r.x, r.y, r.width, r.height, depth_ui.MARK_COLOR);
		}
	}
}

bool LevelEditor::is_selecting(Game& game) const {
	return selected_index < game.trees.size();
}

void LevelEditor::invalidate_selected_index(Game& game) {
	selected_index = game.trees.size();
}

void LevelEditor::duplicate_selected_tendril(Game& game) {
	// Make sure we do this first.
	// Depth is stored on tree, so it differs from treemetadata
	auto& selected = game.trees[selected_index];
	// Don't use selected directly after make_initialized_tree, because it gets deleted as the vector reallocates
	const float depth = selected->depth;
	const auto& meta = tree_metadatas[selected->id];

	selected_index = game.trees.size();

	make_initialized_tree([&game, &depth]() { 
		game.make_tree();
		auto& tree = *game.trees.back();
		tree.depth = depth;
	}, game, meta);
}

std::string LevelEditor::convert_trees_to_chars(std::vector<std::unique_ptr<Tree>>& trees) const {
	std::string ret = "";
	for (const auto& tree : trees) {
		auto& meta = tree_metadatas[tree->id];
		ret = std::format(
			"{}"
			"rotation:{:.6f}\n"
			"offset:{:.6f} {:.6f}\n"
			"seed:{}\n"
			"depth:{:.6f}\n", ret, meta.rotation, meta.offset.x, meta.offset.y, tree->rand.seed, tree->depth);
	}
	return ret;
}