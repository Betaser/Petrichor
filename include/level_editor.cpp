#include "raylib.h"
#include "level_editor.hpp"
#include <iostream>

LevelEditor::LevelEditor() {
    treeIndexesAndAddOns = {};
}

void LevelEditor::initialize_ui() {
    buttons.clear();
	std::vector<Button*> default_buttons = {
        new Button(
            this,
            { (float) Main::screen_width - 490, 110 }, 
            { 80, 80 }, 
            "Show debug keybinds",
			[](Button& b) {
                // auto level_editor = dynamic_cast<LevelEditor*>(b.owner);
				std::stringstream ss; ss
				<< "Right click = cycle mouse mode\n"
				// << "Mouse mode: " << level_editor.from_mode().name
                << "type id of owner: " << typeid(b.owner).name() << "\n"
                // << "type id of leveleditor: " << typeid(this).name() << "\n"
				<< "A = rotate counterclockwise\n"
				<< "D = rotate clockwise";
				b.text = ss.str();
			})
    };
    buttons.insert(buttons.end(), default_buttons.begin(), default_buttons.end());
}

const LevelEditor::MoreMouseMode LevelEditor::from_mode() const {
    for (const auto& more : more_mouse_modes) {
        if (more.mode == mouse_mode) {
            return more;
        }
    }
    return {};
}

void LevelEditor::update(const Main& main) {
    // Cycle mouse mode
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        int index = (from_mode().index + 1) % more_mouse_modes.size();
        mouse_mode = more_mouse_modes[index].mode;
    }

    // Select a tree 
    /*
    int rotation_sign = 0;
    if (IsKeyDown(KEY_A)) {
        rotation_sign = -1;
    }
    if (IsKeyDown(KEY_D)) {
        rotation_sign = 1;
    }
    */
}
