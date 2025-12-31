#include <iostream>
#include <sstream>
#include <assert.h>
#include "mylib.hpp"
#include "level_editor.hpp"
#include "constants.cpp"

LevelEditor::LevelEditor() {
    debug_button = nullptr;
    show_instructions = false;
    std::cout << "init level editor\n";
    initialize_ui();
    time = 0;
    selected_index = 0;
	using_ui = false;
    // Just to make sure last_selected_index is different from selected_index
    last_selected_index = selected_index + 1;
    // This zeroes our struct.
    selection_offset = {};

	Shader select_shader = LoadShader(0, TextFormat("assets/select.fs", Constants::glsl_version));
    this->select_shader = select_shader;

    selected_tex = load_dummy_tex();
}

LevelEditor::~LevelEditor() {
    std::cout << "deinit level editor\n";
    UnloadTexture(selected_tex);
    UnloadShader(select_shader);
}

void LevelEditor::make_initialized_tree(Game& game, const TreeMetadata& metadata) {
	game.make_tree();
	auto& tree = *game.trees.back();
	tree_metadatas.push_back(TreeMetadata(metadata.rotation, metadata.offset, tree));
	randomize_tendrils(game);
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
    buttons.push_back(debug_btn);
    debug_button = &buttons[0];
}

void LevelEditor::randomize_tendrils(Game& game) {
	// Try using randomly generated tendrils too
	Vector2 start_location { 100, 100 };

	auto& tree = game.trees[selected_index];
	Tendrils tendrils = { tree->random_tendril_config(400, 20, 1.2, 0.1, start_location, 5) };
	tree->branches = Tree::branches_from_tendrils(tendrils);
	tree->tendrils = tendrils;

	auto& meta = tree_metadatas[tree->id];
	tree_metadatas[tree->id] = TreeMetadata(meta.rotation, meta.offset, *tree);
	update_selected_verts(game);

	tree->init_texture();
	load_selection_shader(game);
}

void LevelEditor::update_selected_verts(Game& game) {
    auto& selected = game.trees[selected_index];
    auto& meta = tree_metadatas[selected->id];
	const auto& branches = meta.branches;
	if (branches.size() != selected->branches.size())
		std::cerr << "metadata branches size " << branches.size() << " selected branches size " << selected->branches.size() << "\n";

	const Vector2 origin = branches[0].back();
	const float rotation = floor(meta.rotation / (2.0 * PI / 30)) * (2.0 * PI / 30);

	for (size_t i = 0; i < branches.size(); i++) {
		auto& sel_verts = selected->branches[i].verts;
		const auto& verts = branches[i].verts;
		for (size_t j = 0; j < verts.size(); j++) {
			Vector2 rotated = verts[j];
			sel_verts[j] = my_rotate(origin, rotated, rotation) + meta.offset;
		}
	}
}

// Selection is slightly larger than size of tree texture.
void LevelEditor::load_selection_shader(Game& game) {
    std::cout << "\nload selection shader\n";
    UnloadTexture(selected_tex);

    auto& tree = game.trees[selected_index];
    auto tree_tex_bounds = (Vector2I { tree->blank_tex.width, tree->blank_tex.height }).to_vec2();
    tree_tex_bounds += select_extra_bounds;
    auto blank = GenImageColor(tree_tex_bounds.x, tree_tex_bounds.y, BLANK);
    selected_tex = LoadTextureFromImage(blank);
    UnloadImage(blank);
}

void LevelEditor::update(Game& game) {
    time += GetFrameTime();

    // Selected tree is not a thing yet.
    auto& selected = game.trees[selected_index];
    auto& meta = tree_metadatas[selected->id];

	// Right click to select, chooses closest tree
	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
		auto mouse_pos = GetMousePosition();
		float shortest = INFINITY;
		for (size_t i = 0; i < game.trees.size(); i++) {
			const auto& tree = game.trees[i];
			Vector2 small, big;
			tree->bounding_box(small, big);
			auto mid = (small + big) / 2;
			float dist = my_length(mid - mouse_pos);
			if (dist < shortest) {
				selected_index = i;
				shortest = dist;
			}
		}
	}

	// Duplicate. Means we copy over metadata
	if (IsKeyPressed(KEY_F)) {
		// Make sure we do this first.
		// std::cout << "dup " << selected->id << "\n";
		const auto& meta = tree_metadatas[selected->id];
		selected_index = game.trees.size();
		make_initialized_tree(game, meta);
		// std::cout << "newest " << game.trees.back()->id << "\n";

		// Don't want to deal with selection having changed during this if statement affecting expectations for the rest of this function
		return;
	}

    // rotation
    float rotation_input = 0;
    if (IsKeyDown(KEY_A))
        rotation_input = 0.05;
    if (IsKeyDown(KEY_D))
        rotation_input = -0.05; 

    meta.rotation = meta.rotation + rotation_input;
    if (rotation_input != 0) {
		update_selected_verts(game);

		selected->init_texture();
		load_selection_shader(game);
    }

    // Yes, let's eventually move this button checking bounds to a designated class
	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
		using_ui = pt_in_rect(GetMousePosition(), debug_button->state.pos, debug_button->state.dim);
		if (using_ui) {
			debug_button->state.hit = true;
		} 
		else { 
			selection_offset = GetMousePosition() - meta.offset;
		}
    }

	// Adapted from main's while loop
	if (IsKeyPressed(KEY_R)) {
		selected->rand.set_seed(++selected->rand.seed);
		randomize_tendrils(game);

		std::cout << to_str(tree_metadatas[selected->id].offset, 3) << "\n";
	}

    // offset
    if (!using_ui && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        meta.offset = GetMousePosition() - selection_offset;

		update_selected_verts(game);
		// just translate it instead of reloading shaders.
		Vector2 small, big;
		selected->bounding_box(small, big);
		selected->texture_pos = Vector2I(small);
        // selected->init_texture();
		auto tree_tex_bounds = (Vector2I { selected->blank_tex.width, selected->blank_tex.height }).to_vec2();
		tree_tex_bounds += select_extra_bounds;
    }

    if (last_selected_index != selected_index) {
        load_selection_shader(game);
        last_selected_index = selected_index;
    }
}

void LevelEditor::render(Game& game) const {
    int dims_locs = GetShaderLocation(select_shader, "dims");
    Vector2I dims { selected_tex.width, selected_tex.height };
    SetShaderValue(select_shader, dims_locs, &dims, SHADER_UNIFORM_IVEC2);

    int loc = GetShaderLocation(select_shader, "time");
    SetShaderValue(select_shader, loc, &time, SHADER_UNIFORM_FLOAT);

    BeginShaderMode(select_shader);
    auto& tree = game.trees[selected_index];
    Vector2I pos = Vector2I(tree->texture_pos.to_vec2() + select_extra_bounds / 2);
    DrawTexture(selected_tex, pos.x, pos.y, WHITE);
    EndShaderMode();

    if (show_instructions) {
        std::stringstream ss; ss
        << "F = duplicate\n"
		<< "Right click to select\n"
		<< "R = randomize seed\n"
        << "A = rotate counterclockwise\n"
        << "D = rotate clockwise\n"
        << "Mouse scroll = change depth\n"
        << "G = guidelines (editor add ons.\n"
        << "which are saved separate from level data)";
        unsigned char opacity = 255 * (0.3 * (0.5 * sin(time * 3.0) + 0.5) + 0.7);
		const int font_size = 30;
        int text_size = MeasureText(ss.str().c_str(), font_size);
        DrawText(ss.str().c_str(), 400 - text_size / 2, 80, font_size, { 255, 70, 70, opacity });
    }
}