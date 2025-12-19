#include <iostream>
#include <sstream>
#include "level_editor.hpp"

LevelEditor::LevelEditor() {
    initialize_ui();
}

LevelEditor::~LevelEditor() {
    std::cout << "deinit level editor\n";
}

void LevelEditor::update() {
    if (IsKeyDown(KEY_A)) {

    }
    if (IsKeyDown(KEY_D)) {
        
    }
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