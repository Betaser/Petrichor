#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <format>

#include "level_editor.hpp"
#include "level_editor_ui.cpp"
#include "tree_metadata.cpp"
#include "../globals/mylib.hpp"
#include "../globals/constants.cpp"

LevelEditor::LevelEditor(Game& game) {
	std::println("init level editor");

	view_selector.screen_height = &game.screen_height;

	init_selection_texture();
	load_shader(select_shader, "assets/select.fs");
	reinit(game);
}

LevelEditor::~LevelEditor() {
	std::println("deinit level editor");
	unload_texture(selected_tex);
	unload_shader(select_shader);
	std::println("select shader w/ id {} loads/unloads {}", select_shader.id, select_shader.load_unloads);
	std::println("selected_tex w/ id {} loads/unloads {}", selected_tex.id, selected_tex.load_unloads);
}

void LevelEditor::reinit(Game& game) {
	tree_metadatas.clear();
	ui_elem_manager.init();
	tree_to_managed_buttons.clear();

	depth_ui.height = (float) game.screen_height - 2 * depth_ui.SPACING;
	depth_ui.top_left = {
		(float) game.screen_width - depth_ui.WIDTH - depth_ui.SPACING,
		depth_ui.SPACING
	};

	show_instructions = false;
	ui_hovered = false;
	time = 0;
	initialize_ui(game);

	invalidate_selections();	
	std::println("reinit level editor");
}

void LevelEditor::make_initialized_tree(std::function<void()> tree_maker, Game& game, const TreeMetadata& metadata) {
	tree_maker();
	auto& tree = *game.trees.back();
	std::println("make tree w/ id {}", (size_t) tree.id);

	bool ids_available = !deleted_tree_ids.empty();
	if (ids_available) {
		auto popped_id = deleted_tree_ids[0];
		deleted_tree_ids.erase(deleted_tree_ids.begin());
		tree.id = popped_id;
	}

	const auto& depth_rect = update_tree_for_depth_ui(game, *game.trees[game.trees.size() - 1]);
	TreeMetadata temp(metadata.rotation, metadata.offset);
	if (ids_available)
		tree_metadatas[(size_t) tree.id] = temp;
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
	tree.trunk_segments.push_back(segment);
	
	std::vector<TrunkFace> example_faces {
		{
			.depth = 0,
			.position = { 300, 300 },
			.radius = 20,
		},
		{
			.depth = 100,
			.position = { 200, 150 },
			.radius = 10,
		}
	};
	for (auto& face : example_faces)
		tree.trunk_faces.push_back(face);

	// Create the depth editing button
	auto depth_btn = make_depth_button(depth_rect, game, tree.id);

	std::string name = ui_elem_manager.add(depth_btn, "depth_btn");
	tree_to_managed_buttons[tree.id] = name;
}

CHANGETHIS
void LevelEditor::randomize_tendrils(Game& game, const size_t tree_index, const Rand& rand) {
	// Try using randomly generated tendrils too
	const Vector2 start_location { 100, 100 };

	auto& tree = game.trees[tree_index];
	std::vector<std::vector<Branch>> tendrils = tree->random_tendril_config(400, 20, 1.2, 0.1, start_location);
	tree->branches = Tree::branches_from_tendrils(tendrils);
	tree->tendrils = tendrils;

	const auto& meta = tree_metadatas[(size_t) tree->id];
	tree_metadatas[(size_t) tree->id] = TreeMetadata(meta.rotation, meta.offset);
	tree->on_updated_branch();

	update_tree_verts(game, tree_index);

	tree->update_texture();
}

void LevelEditor::update_selected_verts(Game& game) {
	for (const auto& [selected_index, _] : selections)
		update_tree_verts(game, selected_index);
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

	const auto mouse_pos = GetMousePosition();
	auto ui_elem_manager_view = ui_elem_manager.update(mouse_pos);

	// Right click to select, chooses closest tree
	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
		if (IsKeyDown(KEY_LEFT_SHIFT))
			tree_multi_select(game, mouse_pos);
		else 
			tree_single_select(game, mouse_pos);
		set_active_extra_button_group(selections.size() > 1
			? OnMultipleSelected
			: None);
	}

	const bool selecting = is_selecting();

	// Let's test that it fails.
	auto debug_button = ui_elem_manager.get<Button<nullptr_t>>(debug_btn_str);
	
	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
		// if (debug_button->hovered)
		debug_button->state.hit = true;

		for (const auto& name : view_button_names)
			ui_elem_manager.get<Button<View>>(name)->state.hit = true;

		const auto& active_group = extra_state_to_group[active_extra_button];
		for (const auto& name : active_group.names) {
			std::println("reinterpret {} as Button<nullptr_t>", name);
			ui_elem_manager.reinterpret<Button<nullptr_t>>(name)->state.hit = true;
		}
	}

	// Yeah this looks weird but we use hit + hovered to do logic.
	if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
		if (selecting) {
			for (const auto& [selected_index, _] : selections) {
				auto& id = game.trees[selected_index]->id;
				auto depth_button = ui_elem_manager.get<Button<DepthState>>(tree_to_managed_buttons[id]);
				depth_button->state.hit = true;
			}
		}
	}

	const bool last_ui_hovered = ui_hovered;
	ui_hovered = ui_elem_manager_view.any_hovered();	
	const bool using_ui = ui_elem_manager_view.any_in_use();

	if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) {
		std::println("Save trees to {}", Constants::test_level_path);

		auto repr = trees_to_chars(game.trees);
		std::ofstream file;
		file.open(Constants::test_level_path);
		file << repr;
		file.close();
	}

	if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)) {
		// Why not just reinit
		reinit(game);

		std::println("Load file {}", Constants::test_level2_path);
		game.load_trees(
			Constants::test_level2_path, 
			[&](TreeMetadata& meta, Tree& tree) {
				auto depth_rect = update_tree_for_depth_ui(game, tree);
				auto button = make_depth_button(depth_rect, game, tree.id);
				const auto name = ui_elem_manager.add(button, "depth_btn");
				tree_to_managed_buttons[tree.id] = name;
				tree_metadatas.push_back(meta);	
			});

		selections.resize(game.trees.size());
		size_t n = 0;
		std::generate(selections.begin(), selections.end(), 
			[&]() { 
				return Selection { n, tree_metadatas[n++].offset };
			});
		update_selected_verts(game);
		for (const auto& tree : game.trees)
			tree->update_texture();
		invalidate_selections();

		return;
	}

	if (selecting) {
		// Deletion, should be tough
		if (IsKeyPressed(KEY_BACKSPACE)) {
			for (auto& [selected_index, _] : selections) {
				if (game.trees.size() == 1)
					break;

				delete_tree(game, selected_index);
			}

			return;
		}

		for (auto& [selected_index, selection_offset] : selections) {
			// Duplicate. Means we copy over metadata
			if (IsKeyPressed(KEY_F)) {
				duplicate_selected_tendril(game);
				// Don't want to deal with selection having changed during this if statement affecting expectations for the rest of this function
				return;
			}

			// Selected tree is not a thing yet.
			auto& selected = game.trees[selected_index];
			auto& meta = tree_metadatas[(size_t) selected->id];

			if ((!using_ui && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			 || (last_ui_hovered != ui_hovered))
				selection_offset = mouse_pos - meta.offset;

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
				meta.offset = mouse_pos - selection_offset;

				update_selected_verts(game);
				// just translate it instead of reloading shaders.
				Vector2 small, big;
				selected->bounding_box(small, big);
				selected->texture_pos = small;
				auto tree_tex_bounds = (Vector2I { selected->blank_tex.width, selected->blank_tex.height }).to_vec2();
				tree_tex_bounds += select_extra_bounds;
			}
		}
	}

	// Use mouse scroll wheel to control depth
	adjust_cam_depth(game);
}

void LevelEditor::render(Game& game) const {
	// Sort trees by depth-based proximity, no?
	const auto& depth_indices = calc_depth_indices(game);

	for (const size_t& depth_i : depth_indices) {
		auto& tree = game.trees[depth_i];
		const float eff_depth = abs(tree->depth - cam_depth);
		float depth_alpha = std::max(0.0, 1.0 - eff_depth / (depth_ui.MAX_DEPTH * 0.8));
		Vector4 rgb_tint { .x = 0, .y = 0, .z = 0, .w = 0 };

		// Ew this won't align with what we're doing.
		const bool tree_selected = contains_selection(depth_i);

		if (get_active(FocusTreeView)) {
			if (!tree_selected) {
				depth_alpha *= 0.1 * (0.5 * sin(time * 2.3) + 0.5);
			}
		}

		// Selected view; highlight all parts of the tree cause it could be confusing what branches belong to which trees
		if (get_active(SelectedView)) {
			// Tint with non-waning rainbow colors
			const Color unselected_tints[] {
				ORANGE, YELLOW, GREEN, BLUE, MAGENTA, PURPLE	
			};
			Color color = ColorAlpha(unselected_tints[depth_i % (sizeof(unselected_tints) / sizeof(unselected_tints[0]))], 0.5);
			rgb_tint = to_vec4(color);

			if (tree_selected) {
				// wanes with time staying in Highlight.
				float wane_speed = 1.0;
				Color highlight = {
					.r = 255,
					.g = 50,
					.b = 50,
					.a = color.a
				};
				float wane = 0.4 * sin(time * 6.0) * wane_speed + 0.4;
				rgb_tint = to_vec4(ColorLerp(color, highlight, wane));
			}
		}

		tree->level_editor_render({
			.finishing_alpha = depth_alpha,
			.rgb_tint = rgb_tint
		});
	}

	if (is_selecting()) {
		set_shader_value(select_shader, "time", &time, SHADER_UNIFORM_FLOAT);

		for (const auto& [selected_index, _] : selections) {
			auto& tree = game.trees[selected_index];

			Vector2 pos { tree->texture_pos - select_extra_bounds / 2 };
			Vector2 small, big;
			tree->bounding_box(small, big);
			auto dims = big - small + select_extra_bounds;
			auto dims_i = Vector2I(dims);

			set_shader_value(select_shader, "dims", &dims_i, SHADER_UNIFORM_IVEC2);

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
		<< "Mouse scroll to change depth\n"
		<< "Right click + Shift = multiple select\n";

		std::string s_str = ss.str();
		unsigned char opacity = 255 * (0.3 * (0.5 * sin(time * 3.0) + 0.5) + 0.7);
		const int font_size = 30;
		int text_size = MeasureText(s_str.c_str(), font_size);
		DrawText(s_str.c_str(), 400 - text_size / 2, 80, font_size, { 255, 70, 70, opacity });
	}

	// Render the camera depth at top of screen
	render_cam_depth(game);

	ui_elem_manager.render();
}

void LevelEditor::invalidate_selections() {
	selections.clear();
	set_active_extra_button_group(None);
}

void LevelEditor::duplicate_selected_tendril(Game& game) {
	// Make sure we do this first.
	// Depth is stored on tree, so it differs from treemetadata
	std::vector<Selection> new_selections;
	for (const auto& [selected_index, _] : selections) {
		const Tree* selected = game.trees[selected_index].get();
		const auto& meta = tree_metadatas[(size_t) selected->id];

		new_selections.push_back({ game.trees.size(), meta.offset });

		make_initialized_tree([&]() { 
			game.make_tree();
			auto& tree = *game.trees.back();
			tree.depth = selected->depth;
			tree.rand.set_seed(selected->rand.seed);
		}, game, meta);
	}
	selections = new_selections;
}

Rectangle LevelEditor::get_cam_depth(const int screen_width) const {
	const float width = 0.8 * screen_width;
	const float height = 30;
	const float left = ((float) screen_width - width) / 2;
	const float top = 50;
	return {
		.x = left,
		.y = top,
		.width = width,
		.height = height
	};
}

void LevelEditor::render_cam_depth(Game& game) const {
	// Fill to show current depth
	const float normalized_depth = (cam_depth - min_cam_depth) / (max_cam_depth - min_cam_depth);
	const float adjusted_depth = normalized_depth * 0.95 + 0.05;
	const auto& [left, top, width, height] = get_cam_depth(game.screen_width);
	DrawRectangleRounded(
		{
			.x = left + 4,
			.y = top + 4,
			.width = width * adjusted_depth - 8,
			.height = height - 8
		},
		0.4,
		1,
		ColorAlpha(ORANGE, 0.7));
	// Add text below to remind me what this bar is for
	DrawText("Depth (use scroll)", (int) left, (int) (top - 15), 15, PINK);
}

void LevelEditor::adjust_cam_depth(Game& game) {
	const float scroll = GetMouseWheelMove();

	cam_depth += scroll;

	// Bounds are the same bounds that depth_ui uses
	std::vector<float> depths(game.trees.size());
	for (size_t i = 0; i < game.trees.size(); i++)
		depths[i] = game.trees[i]->depth;
	
	std::sort(depths.begin(), depths.end());

	min_cam_depth = std::min(0.0f, depths[0]);
	max_cam_depth = std::max(depth_ui.MAX_DEPTH, depths.back());

	cam_depth = std::min(std::max(min_cam_depth, cam_depth), max_cam_depth);
}

std::string LevelEditor::trees_to_chars(std::vector<std::unique_ptr<Tree>>& trees) const {
	std::string ret = "";
	for (const auto& tree : trees) {
		auto& meta = tree_metadatas[(size_t) tree->id];
		ret = std::format(
			"{}"
			"rotation:{:.6f}\n"
			"offset:{:.6f} {:.6f}\n"
			"seed:{}\n"
			"depth:{:.6f}\n", ret, meta.rotation, meta.offset.x, meta.offset.y, tree->rand.seed, tree->depth);
	}
	return ret;
}

bool LevelEditor::is_selecting() const {
	return selections.size() > 0;
}

void LevelEditor::set_active(View view, bool active) {
	views_active.set(view, active);
}

bool LevelEditor::get_active(View view) const {
	return views_active.test(view);
}

Rectangle LevelEditor::update_tree_for_depth_ui(Game& game, const Tree& tree) {
	std::vector<float> depths(game.trees.size());
	for (size_t i = 0; i < game.trees.size(); i++)
		depths[i] = game.trees[i]->depth;
	
	std::sort(depths.begin(), depths.end());

	const float min_depth = std::min(0.0f, depths[0]);
	const float max_depth = std::max(depth_ui.MAX_DEPTH, depths.back());

	const int spacing = -5;
	const float height = (float) depth_ui.MARK_HEIGHT;

	const float percent = (tree.depth - min_depth) / (max_depth - min_depth);
	const float y_pos = percent * depth_ui.height + depth_ui.SPACING - height / 2;

	return { 
		.x = depth_ui.top_left.x + spacing, 
		.y = y_pos, 
		.width = depth_ui.WIDTH - 2 * spacing, 
		.height = height 
	};
}

bool LevelEditor::find_cursor_selection(Game& game, Vector2 cursor, Selection* selection) {
	float shortest = INFINITY;

	for (size_t i = 0; i < game.trees.size(); i++) {
		const auto& tree = game.trees[i];
		Vector2 small, big;
		tree->bounding_box(small, big);
		if (!pt_in_rect(cursor, { small.x, small.y, big.x - small.x, big.y - small.y }))
			continue;

		auto mid = (small + big) / 2;
		float dist = length(mid - cursor);
		if (dist < shortest) {
			selection->index = i;
			selection->offset = tree_metadatas[i].offset;
			shortest = dist;
		}
	}

	return shortest != INFINITY;
}

void LevelEditor::tree_single_select(Game& game, Vector2 mouse_pos) {
	Selection selection;
	if (find_cursor_selection(game, mouse_pos, &selection))
		selections = { selection };
	else
		invalidate_selections();
}

void LevelEditor::tree_multi_select(Game& game, Vector2 mouse_pos) {
	Selection selection;
	if (find_cursor_selection(game, mouse_pos, &selection)) {
		if (!contains_selection(selection.index)) {
			selections.push_back(selection);
		}
		else {
			// selection.index is NOT inside of selections at selection.index
			std::erase_if(selections,
				[&](auto& sel) {
					return sel.index == selection.index;
				});
		}
	}
	else
		invalidate_selections();
}

void LevelEditor::update_tree_verts(Game& game, const size_t tree_index) {
	auto& tree = game.trees[tree_index];
	auto& meta = tree_metadatas[(size_t) tree->id];

	const auto& branches = tree->original_branches;

	if (branches.size() != tree->branches.size())
		std::cerr << "metadata branches size " << branches.size() << " selected branches size " << tree->branches.size() << "\n";

	const float rotation = snap(meta.rotation, 2.0 * PI / 30);

	for (size_t i = 0; i < branches.size(); i++) {
		auto& sel_verts = tree->branches[i].verts;
		const auto& verts = branches[i].verts;
		for (size_t j = 0; j < verts.size(); j++)
			sel_verts[j] = rotate(branches[0].back(), verts[j], rotation) + meta.offset;
	}
}

bool LevelEditor::contains_selection(const size_t index) const {
	return std::find_if(selections.begin(), selections.end(), 
		[&](auto& sel) {
			return sel.index == index;
		}) != selections.end();
}

int LevelEditor::from_selected_by_id(Game& game, const Tree::Id tree_id) const {
	const auto& search = std::find_if(selections.begin(), selections.end(),
		[&](auto& sel) {
			return game.trees[sel.index]->id == tree_id;
		});
	if (search == selections.end())
		return -1;
	return search->index;
}

void LevelEditor::set_active_extra_button_group(ExtraButtonState button_state) {
	active_extra_button = button_state;
	for (auto state = (ExtraButtonState) 0; 
		state < EXTRA_SIZE; 
		state = (ExtraButtonState) ((size_t) state + 1)) {
		const auto& group = extra_state_to_group[state];
		for (const auto& name : group.names)
			ui_elem_manager.set_active(name, state == button_state);
	}

	// None state should mean hidden extra buttons
	ui_elem_manager.set_active(extra_buttons_region_name, button_state != None);
}

void LevelEditor::delete_tree(Game& game, const size_t tree_index) {
	auto& tree = game.trees[tree_index];
	const auto tree_id = tree->id;
	auto name = tree_to_managed_buttons[tree_id];
	ui_elem_manager.remove(name);
	tree_to_managed_buttons.erase(tree_id);

	deleted_tree_ids.push_back(tree_id);
	game.trees.erase(game.trees.begin() + tree_index);

	std::println("deleted {}", (size_t) tree_id);
	invalidate_selections();
}

std::vector<size_t> LevelEditor::calc_depth_indices(Game& game) {
	std::vector<size_t> depth_indices(game.trees.size());
	for (size_t i = 0; i < game.trees.size(); i++) {
		auto& tree = game.trees[i];
		size_t j = i;
		for (; j > 0; j--) {
			size_t depth_index = depth_indices[j - 1];
			auto& existing_tree = game.trees[depth_index];
			if (tree->depth <= existing_tree->depth)
				break;

			// Shift it over. Insert after the loop.
			depth_indices[j] = depth_indices[j - 1];
		}
		depth_indices[j] = i;
	}

	return depth_indices;
}
