#include <unordered_set>
#include <sstream>
#include <algorithm>
#include <format>

#include "level_editor.hpp"
#include "level_editor_ui.cpp"
#include "tree_metadata.cpp"
#include "../globals/mylib.hpp"
#include "../globals/constants.cpp"


void LevelEditor::ExtraButtonManager::set_active_extra_button_group(ExtraButtonState button_state) {
	active_state = button_state;
	for (auto state = (ExtraButtonState) 0; 
		state < EXTRA_SIZE; 
		state = (ExtraButtonState) ((size_t) state + 1)) {
		const auto& group = extra_state_to_group[state];
		for (const auto& name : group.names)
			owner->ui_elem_manager.set_active(name, state == button_state);
	}

	// None state should mean hidden extra buttons
	owner->ui_elem_manager.set_active(region_name, button_state != None);
}

void LevelEditor::ExtraButtonManager::set_hit_state_true() {
	const auto& active_group = extra_state_to_group[active_state];
	for (const auto& name : active_group.names) {
		std::println("reinterpret {} as Button<nullptr_t>", name);
		owner->ui_elem_manager.reinterpret<Button<nullptr_t>>(name)->state.hit = true;
	}
}

LevelEditor::LevelEditor(Game& game) {
	std::println("init level editor");

	view_selector.screen_height = &game.screen_height;

	init_selection_texture();
	select_shader.load_shader("assets/select.fs");
	reinit(game);
}

LevelEditor::~LevelEditor() {
	std::println("deinit level editor");
	selected_tex.unload_texture();
	select_shader.unload_shader();
	std::println("select shader w/ id {} loads/unloads {}", select_shader.id, select_shader.load_unloads);
	std::println("selected_tex w/ id {} loads/unloads {}", selected_tex.id, selected_tex.load_unloads);
}

void LevelEditor::reinit(Game& game) {
	branch_metadatas.clear();
	ui_elem_manager.init();
	config_to_managed_buttons.clear();
	mouse_tool_permits_selection_movement = false;
	// Why did I not clear all_config_info before???
	all_config_info.clear();

	// NOT PART OF all_config_infos, intentionally.
	{
		// A tree is guaranteed to already exist cause it was done in main.
		auto& back_tree = game.trees.back();
		mouse_tool_preview_config = std::make_unique<TendrilConfig>(TendrilConfig::Id(0), Rand(69), back_tree.get());
		BranchMetadata meta({}, 0);
		std::println("randomize the preview");
		randomize_tendrils(*mouse_tool_preview_config, meta);
	}

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

void LevelEditor::make_initialized_config(Tree& tree, const Rand& rand, const BranchMetadata& metadata) {
	TendrilConfig::Id config_index_and_id = (TendrilConfig::Id) all_config_info.size();
	const bool ids_available = !deleted_config_ids.empty();
	if (ids_available) {
		auto popped_id = deleted_config_ids[0];
		deleted_config_ids.erase(deleted_config_ids.begin());
		config_index_and_id = popped_id;
	}
	
	tree.tendril_configs.push_back(std::make_unique<TendrilConfig>(config_index_and_id, Rand(rand.seed), &tree));
	auto& config = tree.tendril_configs.back();
	all_config_info.push_back({ config.get(), all_config_info.size() });

	std::println("make config w/ id {}", (size_t) config->id);

	const auto& depth_rect = update_config_for_depth_ui(*config);
	if (ids_available)
		branch_metadatas[(size_t) config->id] = metadata;
	else
		branch_metadatas.push_back(metadata);

	randomize_tendrils((size_t) config_index_and_id);

	/*
	// MAKING AN INIT'D TREE INVOLVES THIS, RN WE CAN'T MAKE A TREE
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
	*/

	// Create the depth editing button
	auto depth_btn = make_depth_button(depth_rect, config->id);

	std::string name = ui_elem_manager.add(depth_btn, "depth_btn");
	config_to_managed_buttons[config->id] = name;
}
	
void LevelEditor::randomize_tendrils(TendrilConfig& config, const BranchMetadata& meta) {
	// Try using randomly generated tendrils too
	const Vector2 start_location { 100, 100 };

	auto structured_branches = config.gen_structured_branches(400, 20, 1.2, 0.1, start_location);
	config.branches = Tree::branches_from_structured_branches(structured_branches);
	config.structured_branches = structured_branches;

	config.on_updated_branch();
	branch_verts_from_metadata(config, meta);
	config.update_texture();
}

void LevelEditor::randomize_tendrils(size_t config_index) {
	auto& config = *all_config_info[config_index].ptr;
	const auto& meta = branch_metadatas[(size_t) config.id];

	randomize_tendrils(config, meta);
}

void LevelEditor::update_selected_verts() {
	for (const auto& [selected_index, _] : selections)
		branch_verts_from_metadata(selected_index);
}

// Selection is slightly larger than size of tree texture.
void LevelEditor::init_selection_texture() {
	std::println("init selection texture");

	auto blank = GenImageColor(10, 10, BLANK);
	selected_tex.load_texture_from_image(blank);
	UnloadImage(blank);
}

void LevelEditor::update(Game& game) {
	DeferThis defer;

	time += GetFrameTime();

	const auto mouse_pos = GetMousePosition();
	auto ui_elem_manager_view = ui_elem_manager.update(mouse_pos);

	const bool selecting = is_selecting();
	defer.and_this([&]() { last_selecting = selecting; });

	const bool last_ui_hovered = ui_hovered;
	ui_hovered = ui_elem_manager_view.any_hovered();	
	const bool using_ui = ui_elem_manager_view.any_in_use();

	// Let's test that it fails.

	// Right click to select, chooses closest tree
	if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
		if (IsKeyDown(KEY_LEFT_SHIFT))
			tree_multi_select(mouse_pos);
		else 
			tree_single_select(mouse_pos);
		extra_button_manager.set_active_extra_button_group(selections.size() > 1
			? OnMultipleSelected
			: None);
	}
	
	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
		debug_button->state.hit = true;

		for (const auto& btn : view_buttons)
			btn->state.hit = true;

		for (const auto& btn : mouse_tool_used.buttons)
			btn->state.hit = true;

		if (pivot_point_button->data)
			pivot_point_button->state.hit = true;

		extra_button_manager.set_hit_state_true();
	}

	// New addition, do something depending on what the MouseToolState::ToolUsed is.
	bool mouse_tool_can_place = !using_ui;
	const bool cursor_hovering_selection = is_cursor_hovering_selection(mouse_pos);
	if (cursor_hovering_selection)
		mouse_tool_can_place = false;

	switch (mouse_tool_used.type) {
		case MouseTool::PlaceTendrilConfig: {
			// If you left click and your mouse is inside of the bounds of a selected item, you can move as normal as you hold down left click.
			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
				mouse_tool_permits_selection_movement = cursor_hovering_selection;

			if (mouse_tool_can_place) {
				// Make a tree branch with default depth/rotation 
				// at the location of the cursor, with a biggest rand value + 1 I suppose?
				int seed = 69;
				for (auto& tree : game.trees) {
					for (auto& config : tree->tendril_configs) {
						seed = std::max(seed, config->rand.seed + 1);
					}
				}
				auto& tree = game.trees.back();
				const Vector2 default_offset { 100, 100 };
				const Vector2 offset_mouse_pos { mouse_pos - default_offset };

				if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {

					// I should probably get rid of this really weird offset issue
					// Location is kinda wonky (by a consistent amount, so whatevs)
					// Change selection to the recently placed thing.
					BranchMetadata meta(offset_mouse_pos, 0);
					invalidate_selections();
					selections.push_back({ all_config_info.size(), meta.offset });
					Rand rand(seed);
					make_initialized_config(*tree, rand, meta);
				}
				// TODO: Also if we are on this mode, preview our placement.
				else {
					// NOT PART OF all_config_infos, intentionally.
					auto& config = mouse_tool_preview_config;
					config->rand.set_seed(seed);
					config->tree_owner = tree.get();
					BranchMetadata meta(offset_mouse_pos, 0);
					randomize_tendrils(*mouse_tool_preview_config, meta);
				}
			}
		}
		break;
		case MouseTool::PlaceTreeTrunk: {
			std::println("TODO: Place tree trunk down");
			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouse_tool_can_place) {
				// Similar to PlaceTendrilConfig but place a tree trunk at the mouse location
			}
		}
		break;
		case MouseTool::SelectionCentric: {
			// This mode means left click does nothing at all.
			mouse_tool_permits_selection_movement = true;
			// Preview a tree's selections when you hover over it
		}
		break;
		case MouseTool::MOUSE_TOOL_SIZE: break;
	}

	// Yeah this looks weird but we use hit + hovered to do logic.
	if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
		if (selecting) {
			for (const auto& [selected_index, _] : selections) {
				auto& id = all_config_info[selected_index].ptr->id;
				auto depth_button = ui_elem_manager.get<Button<DepthState>>(config_to_managed_buttons[id]);
				depth_button->state.hit = true;
			}
		}
	}

	if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) {
		std::println("SAVE INTIATED, BUT IMPL DELAYED FOR NOW");
		/*
		std::println("Save trees to {}", Constants::test_level_path);

		auto repr = trees_to_chars(game_dot_trees);
		std::ofstream file;
		file.open(Constants::test_level_path);
		file << repr;
		file.close();
		*/
	}

	if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_Q)) {
		std::println("reinit");
		reinit(game);
	}

	if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)) {
		reinit(game);

		std::println("Load file {}", Constants::test_level2_path);
		game.load_trees(
			Constants::test_level2_path, 
			[&](BranchMetadata& meta, TendrilConfig* config, size_t tree_owner_index) {
				all_config_info.push_back({ config, tree_owner_index });
				auto depth_rect = update_config_for_depth_ui(*config);
				auto button = make_depth_button(depth_rect, config->id);
				const auto name = ui_elem_manager.add(button, "depth_btn");
				config_to_managed_buttons[config->id] = name;

				// Set the branches, which will be copied to the original_branches after this callback runs
				const auto& origin = config->branches[0].back();
				for (size_t i = 0; i < config->branches.size(); i++) {
					auto& verts = config->branches[i].verts;
					for (size_t j = 0; j < verts.size(); j++)
						verts[j] = rotate(origin, verts[j], meta.rotation) + meta.offset;
				}
			});

		// metadata is zero to start with?
		branch_metadatas = std::vector<BranchMetadata>(all_config_info.size());

		invalidate_selections();

		return;
	}

	if (selecting) {
		// Then we make sure to create a pivot point.
		if (!last_selecting) {
			// Clearly this involves some weird lifetimes causing a bug, ofc analyze this in wsl.
			auto pivot_point = get_or<Vector2>(
				pivot_point_button->data, 
				[&]() { 
					auto pivot_point = calc_default_pivot_point(game);
					pivot_point_button->data = pivot_point;
					return pivot_point;
				});

			// Idk do other stuff now.
			(void) pivot_point;
		}

		// Deletion, should be tough
		if (IsKeyPressed(KEY_BACKSPACE)) {
			for (auto& [selected_index, _] : selections) {
				if (all_config_info.size() == 1)
					break;

				delete_config(selected_index);
			}

			return;
		}

		for (auto& [selected_index, selection_offset] : selections) {
			// Duplicate. Means we copy over metadata
			if (IsKeyPressed(KEY_F)) {
				duplicate_selected_tendril();
				// Don't want to deal with selection having changed during this if statement affecting expectations for the rest of this function
				return;
			}

			// Selected tree is not a thing yet.
			auto& selected = all_config_info[selected_index].ptr;
			auto& meta = branch_metadatas[(size_t) selected->id];

			if ((!using_ui && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			 || (last_ui_hovered != ui_hovered)) {
				selection_offset = mouse_pos - meta.offset;
			}

			// rotation
			float rotation_input = 0;
			if (IsKeyDown(KEY_A))
				rotation_input = -0.05;
			if (IsKeyDown(KEY_D))
				rotation_input = 0.05; 

			meta.rotation += rotation_input;

			if (rotation_input != 0) {
				update_selected_verts();
				selected->update_texture();
			}

			// Adapted from main's while loop
			if (IsKeyPressed(KEY_R)) {
				const int new_seed = selected->rand.seed + (IsKeyDown(KEY_LEFT_SHIFT) ? -1 : 1);
				selected->rand.set_seed(new_seed);
				randomize_tendrils(selected_index);
			}

			// offset
			if (!using_ui && IsMouseButtonDown(MOUSE_LEFT_BUTTON) && mouse_tool_permits_selection_movement) {
				meta.offset = mouse_pos - selection_offset;

				std::println("Left mouse down and mouse tool permits");
				update_selected_verts();
				// just translate it instead of reloading shaders.
				Vector2 small, big;
				selected->bounding_box(small, big);
				selected->texture_pos = small;
				auto tree_tex_bounds = (Vector2I { selected->blank_tex.width, selected->blank_tex.height }).to_vec2();
				tree_tex_bounds += select_extra_bounds;
			}
		}
	}
	else {
		// If we stop selecting, destroy the pivot point data.
		pivot_point_button->data = std::nullopt;
	}

	// Use mouse scroll wheel to control depth
	adjust_cam_depth();
}

void LevelEditor::render(Game& game) const {
	// Sort tendril_configs by depth-based proximity
	const auto& depth_indices = calc_depth_indices();

	std::unordered_set<Tree*> selected_trees;
	for (const size_t i : depth_indices) {
		if (contains_selection(i)) {
			auto& tree = all_config_info[i].ptr->tree_owner;
			selected_trees.insert(tree);
		}
	}

	for (const size_t depth_i : depth_indices) {
		auto& config = all_config_info[depth_i].ptr;
		const float eff_depth = abs(config->depth - cam_depth);
		float depth_alpha = std::max(0.0, 1.0 - eff_depth / (depth_ui.MAX_DEPTH * 0.8));
		Vector4 rgb_tint { .x = 0, .y = 0, .z = 0, .w = 0 };

		// Ew this won't align with what we're doing.
		const bool tree_selected = selected_trees.contains(config->tree_owner);

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

		config->level_editor_render({
			.finishing_alpha = depth_alpha,
			.rgb_tint = rgb_tint
		});
	}

	if (is_selecting()) {
		set_shader_value(select_shader, "time", &time, SHADER_UNIFORM_FLOAT);

		for (const auto& [selected_index, _] : selections) {
			auto& config = all_config_info[selected_index].ptr;

			Vector2 pos { config->texture_pos - select_extra_bounds / 2 };
			Vector2 small, big;
			config->bounding_box(small, big);
			auto dims = big - small + select_extra_bounds;
			auto dims_i = Vector2I(dims);

			set_shader_value(select_shader, "dims", &dims_i, SHADER_UNIFORM_IVEC2);

			BeginShaderMode(select_shader);
			DrawTexturePro(
				selected_tex,
				// This should work
				full_texture(selected_tex),
				/*
				// source rect
				{
					.x = 0,
					.y = 0,
					.width = (float) selected_tex.width,
					.height = (float) selected_tex.height
				},
				*/
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

	// Preview shall have some fricken look.
	if (mouse_tool_used.type == MouseTool::PlaceTendrilConfig && 
		!is_cursor_hovering_selection(GetMousePosition())) {
		mouse_tool_preview_config->level_editor_render({
			.finishing_alpha = 0.2,
			.rgb_tint = to_vec4(ColorAlpha(RED, 0.2))
		});
	}

	// A little scuffed, but will be helpful for debugging later.
	{
		const float HEIGHT = 40;
		float y_pos = (float) game.screen_height / 2 - (float) game.trees.size() / 2 * HEIGHT;
		const int SHADOW_X = 1;
		const int SHADOW_Y = 1;
		for (const auto& tree : game.trees) {
			DrawText(std::format("tree tendrils: {}", tree->tendril_configs.size()).c_str(), 
				100 - SHADOW_X, (int) y_pos - SHADOW_Y, 24, WHITE);
			DrawText(std::format("tree tendrils: {}", tree->tendril_configs.size()).c_str(), 
				100 + SHADOW_X, (int) y_pos + SHADOW_Y, 24, WHITE);
			DrawText(std::format("tree tendrils: {}", tree->tendril_configs.size()).c_str(), 
				100, (int) y_pos, 24, BROWN);
			y_pos += HEIGHT;
		}
	}
}

void LevelEditor::invalidate_selections() {
	selections.clear();
	extra_button_manager.set_active_extra_button_group(None);
}

void LevelEditor::duplicate_selected_tendril() {
	// Make sure we do this first.
	// Depth is stored on tree, so it differs from treemetadata
	std::vector<Selection> new_selections;
	for (const auto& [selected_index, _] : selections) {
		const auto& selected = all_config_info[selected_index].ptr;
		const auto& meta = branch_metadatas[(size_t) selected->id];

		new_selections.push_back({ all_config_info.size(), meta.offset });

		// Use the tree that selected belongs to.
		make_initialized_config(*selected->tree_owner, selected->rand, meta);
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

void LevelEditor::adjust_cam_depth() {
	const float scroll = GetMouseWheelMove();

	cam_depth += scroll;

	// Bounds are the same bounds that depth_ui uses
	std::vector<float> depths(all_config_info.size());
	for (size_t i = 0; i < depths.size(); i++)
		depths[i] = all_config_info[i].ptr->depth;
	
	std::sort(depths.begin(), depths.end());

	min_cam_depth = std::min(0.0f, depths[0]);
	max_cam_depth = std::max(depth_ui.MAX_DEPTH, depths.back());

	cam_depth = std::min(std::max(min_cam_depth, cam_depth), max_cam_depth);
}

std::string LevelEditor::trees_to_chars(std::vector<std::unique_ptr<Tree>>& _) const {
	/*
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
	*/
	return "TODO IMPL of TREES_TO_CHARS";
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

Rectangle LevelEditor::update_config_for_depth_ui(const TendrilConfig& config) {
	std::vector<float> depths(all_config_info.size());
	for (size_t i = 0; i < depths.size(); i++)
		depths[i] = all_config_info[i].ptr->depth;
	
	std::sort(depths.begin(), depths.end());

	const float min_depth = std::min(0.0f, depths[0]);
	const float max_depth = std::max(depth_ui.MAX_DEPTH, depths.back());

	const int spacing = -5;
	const float height = (float) depth_ui.MARK_HEIGHT;

	const float percent = (config.depth - min_depth) / (max_depth - min_depth);
	const float y_pos = percent * depth_ui.height + depth_ui.SPACING - height / 2;

	return { 
		.x = depth_ui.top_left.x + spacing, 
		.y = y_pos, 
		.width = depth_ui.WIDTH - 2 * spacing, 
		.height = height 
	};
}

bool LevelEditor::is_cursor_hovering_selection(Vector2 cursor) const {
	for (const auto& selection : selections) {
		Vector2 small, big;
		all_config_info[selection.index].ptr->bounding_box(small, big);
		if (pt_in_rect(cursor, min_max_to_rect(small, big)))
			return true;
	}
	return false;
}

// Actually I think it makes more sense to choose a selection whose depth that is closer to the editor's 
bool LevelEditor::find_cursor_selection(Vector2 cursor, Selection* selection) {
	enum SelectionResolution {
		ClosestMidpoint,
		ClosestDepth,
	};
	switch (ClosestDepth) {
		case ClosestDepth: {
			float closest_depth_diff = INFINITY;

			for (size_t i = 0; i < all_config_info.size(); i++) {
				const auto& config = all_config_info[i].ptr;
				Vector2 small, big;
				config->bounding_box(small, big);
				if (!pt_in_rect(cursor, min_max_to_rect(small, big)))
					continue;

				const float depth_diff = abs(config->depth - cam_depth);

				if (depth_diff < closest_depth_diff) {
					selection->index = i;
					selection->offset = branch_metadatas[i].offset;
					closest_depth_diff = depth_diff;
				}
			}

			return closest_depth_diff != INFINITY;
		}
		break;
		case ClosestMidpoint: {
			float shortest = INFINITY;

			for (size_t i = 0; i < all_config_info.size(); i++) {
				const auto& config = all_config_info[i].ptr;
				Vector2 small, big;
				config->bounding_box(small, big);
				if (!pt_in_rect(cursor, min_max_to_rect(small, big)))
					continue;

				auto mid = (small + big) / 2;
				float dist = length(mid - cursor);
				if (dist < shortest) {
					selection->index = i;
					selection->offset = branch_metadatas[i].offset;
					shortest = dist;
				}
			}

			return shortest != INFINITY;
		}
		break;
	}
}

void LevelEditor::tree_single_select(Vector2 mouse_pos) {
	Selection selection;
	if (find_cursor_selection(mouse_pos, &selection))
		selections = { selection };
	else
		invalidate_selections();
}

void LevelEditor::tree_multi_select(Vector2 mouse_pos) {
	Selection selection;
	if (find_cursor_selection(mouse_pos, &selection)) {
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

void LevelEditor::branch_verts_from_metadata(TendrilConfig& config, const BranchMetadata& meta) {
	const float rotation = snap(meta.rotation, 2.0 * PI / 30);

	const auto& origin = config.original_branches[0].back();
	for (size_t i = 0; i < config.original_branches.size(); i++) {
		auto& sel_verts = config.branches[i].verts;
		const auto& verts = config.original_branches[i].verts;
		for (size_t j = 0; j < verts.size(); j++)
			sel_verts[j] = rotate(origin, verts[j], rotation) + meta.offset;
	}
}

void LevelEditor::branch_verts_from_metadata(size_t config_index) {
	auto& config = *all_config_info[config_index].ptr;
	auto& meta = branch_metadatas[(size_t) config.id];
	branch_verts_from_metadata(config, meta);
}

bool LevelEditor::contains_selection(size_t index) const { 
	for (const auto& selection : selections) {
		if (selection.index == index) {
			return true;
		}
	}
	return false;
}

int LevelEditor::from_selected_by_id(TendrilConfig::Id config_id) const {
	for (const auto& selection : selections) {
		const auto& id = all_config_info[selection.index].ptr->id;
		if (id == config_id)
			return selection.index;
	}
	return -1;
}

void LevelEditor::delete_config(size_t config_index) {
	auto& config = all_config_info[config_index].ptr;
	const auto config_id = config->id;
	auto name = config_to_managed_buttons[config_id];
	ui_elem_manager.remove(name);
	config_to_managed_buttons.erase(config_id);

	deleted_config_ids.push_back(config_id);
	all_config_info.erase(all_config_info.begin() + config_index);

	std::println("deleted {}", (size_t) config_id);
	invalidate_selections();
}

std::vector<size_t> LevelEditor::calc_depth_indices() const {
	std::vector<size_t> depth_indices(all_config_info.size());
	for (size_t i = 0; i < depth_indices.size(); i++) {
		auto& config = all_config_info[i].ptr;
		size_t j = i;
		for (; j > 0; j--) {
			size_t depth_index = depth_indices[j - 1];
			auto& existing_config = all_config_info[depth_index].ptr;
			if (config->depth <= existing_config->depth)
				break;

			// Shift it over. Insert after the loop.
			depth_indices[j] = depth_indices[j - 1];
		}
		depth_indices[j] = i;
	}

	return depth_indices;
}

Vector2 LevelEditor::calc_default_pivot_point(Game& game) {
	Vector2 screen_center { (float) game.screen_width / 2, (float) game.screen_height / 2 };
	return screen_center;
}
