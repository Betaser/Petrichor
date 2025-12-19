#include <iostream>
#include <sstream>
#include "level_editor.hpp"

LevelEditor::LevelEditor() {
    initialize_ui();
}

LevelEditor::~LevelEditor() {
    std::cout << "deinit level editor\n";
}

void LevelEditor::update_rotation(Game& game) {
    auto& selected = game.trees[0];
    auto& meta = tree_metadatas[selected->id];
    const auto& branches = meta.branches;

    if (branches.size() != selected->branches.size())
        std::cerr << "metadata branches size " << branches.size() << " selected branches size " << selected->branches.size() << "\n";

    Vector2 origin = selected->branches[0].back();

    float rotation = floor(meta.rotation / (2.0 * PI / 30)) * (2.0 * PI / 30);

    for (int i = 0; i < branches.size(); i++) {
        auto& sel_verts = selected->branches[i].verts;
        const auto& verts = branches[i].verts;
        for (int j = 0; j < verts.size(); j++) {
            Vector2 rotated = verts[j];
            sel_verts[j] = my_rotate(origin, rotated, rotation);
        }
    }

    selected->init_texture();
}

void LevelEditor::update(Game& game) {
    // Selected tree is not a thing yet.
    auto& selected = game.trees[0];
    float rotation_input = 0;
    if (IsKeyDown(KEY_A))
        rotation_input = -0.05;
    if (IsKeyDown(KEY_D))
        rotation_input = 0.05; 
    auto& meta = tree_metadatas[selected->id];
    meta.rotation = meta.rotation + rotation_input;
    if (rotation_input != 0)
        update_rotation(game);
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
            auto level_editor = static_cast<LevelEditor*>(b.owner);
            std::stringstream ss; ss
            << "Right click = toggle branch placement mode\n"
            << "G = guidelines (editor add ons that are saved separate from level data)\n"
            << "test " << level_editor->blah << "\n"
            << "A = rotate counterclockwise\n"
            << "D = rotate clockwise";
            b.text = ss.str();
        });
    buttons.push_back(debug_btn);
}