#include <iostream>
#include <sstream>
#include <assert.h>
#include "mylib.hpp"
#include "level_editor.hpp"
#include "constants.cpp"

LevelEditor::LevelEditor(Game& game) {
    std::cout << "init level editor\n";
    initialize_ui();
    time = 0;
    selected_index = 0;
    // Just to make sure last_selected_index is different from selected_index
    last_selected_index = selected_index + 1;

	Shader select_shader = LoadShader(0, TextFormat("assets/select.fs", Constants::glsl_version));
    this->select_shader = select_shader;

    selected_tex = load_dummy_tex();
}

LevelEditor::~LevelEditor() {
    std::cout << "deinit level editor\n";
    UnloadTexture(selected_tex);
    UnloadShader(select_shader);
}

void LevelEditor::update_rotation(Game& game) {
    auto& selected = game.trees[0];
    auto& meta = tree_metadatas[selected->id];
    const auto& branches = meta.branches;

    if (branches.size() != selected->branches.size())
        std::cerr << "metadata branches size " << branches.size() << " selected branches size " << selected->branches.size() << "\n";

    Vector2 origin = selected->branches[0].back();

    float rotation = floor(meta.rotation / (2.0 * PI / 30)) * (2.0 * PI / 30);

    for (size_t i = 0; i < branches.size(); i++) {
        auto& sel_verts = selected->branches[i].verts;
        const auto& verts = branches[i].verts;
        for (size_t j = 0; j < verts.size(); j++) {
            Vector2 rotated = verts[j];
            sel_verts[j] = my_rotate(origin, rotated, rotation);
        }
    }

    selected->init_texture();
    load_selection_shader(game);
}

void LevelEditor::update(Game& game) {
    time += GetFrameTime();
    // Selected tree is not a thing yet.
    auto& selected = game.trees[0];
    float rotation_input = 0;
    if (IsKeyDown(KEY_A))
        rotation_input = 0.05;
    if (IsKeyDown(KEY_D))
        rotation_input = -0.05; 
    auto& meta = tree_metadatas[selected->id];
    meta.rotation = meta.rotation + rotation_input;
    if (rotation_input != 0) {
        // Selection shader has to resize.
        update_rotation(game);
    }

    if (last_selected_index != selected_index) {
        load_selection_shader(game);
        last_selected_index = selected_index;
    }
}

// Selection is slightly larger than size of tree texture.
void LevelEditor::load_selection_shader(Game& game) {
    std::cout << "load selection shader\n";
    // selected_tex should be set to something at first.
    UnloadTexture(selected_tex);

    auto& tree = game.trees[selected_index];
    auto tree_tex_bounds = (Vector2I { tree->blank_tex.width, tree->blank_tex.height }).to_vec2();
    tree_tex_bounds += select_extra_bounds;
    auto blank = GenImageColor(tree_tex_bounds.x, tree_tex_bounds.y, BLANK);
    selected_tex = LoadTextureFromImage(blank);
    UnloadImage(blank);
}

void LevelEditor::render(Game& game) const {
    int loc = GetShaderLocation(select_shader, "time");
    SetShaderValue(select_shader, loc, &time, SHADER_UNIFORM_FLOAT);

    BeginShaderMode(select_shader);
    auto& tree = game.trees[selected_index];
    Vector2I pos = Vector2I((tree->texture_pos).to_vec2() + select_extra_bounds / 2);
    DrawTexture(selected_tex, pos.x, pos.y, WHITE);
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
            // auto level_editor = dynamic_cast<LevelEditor*>(b.owner);
            std::stringstream ss; ss
            << "F = duplicate\n"
            << "A = rotate counterclockwise\n"
            << "D = rotate clockwise\n"
            << "G = guidelines (editor add ons that are saved separate from level data)";
            b.text = ss.str();
        });
    buttons.push_back(debug_btn);
}