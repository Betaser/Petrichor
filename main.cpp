#include "main.hpp"
#include "mylib.cpp"
#include "tree.cpp"
#include "petra.cpp"
#include "level_editor.cpp"
#include "game.cpp"
#include "button.cpp"
#include "raylib.h"
#include <sstream>
#include <string>
#include <memory>

#if defined(PLATFORM_DESKTOP)
	#define GLSL_VERSION 330
#else // PLATFORM_ANDROID, PLATFORM_WEB
	#define GLSL_VERSION 100
#endif

int main() {
	const int screenWidth = 800;
	const int screenHeight = 600;
	InitWindow(screenWidth, screenHeight, "Raylib basic window");
	SetTargetFPS(60);	

	Game game;
	game.make_tree();

	LevelEditor level_editor;
	level_editor.initialize_ui();

	// Try using randomly generated tendrils too
	Vector2 start_location { 100, 100 };

	const auto gen_tendrils = [&start_location](Tree& tree) {
		return tree.random_tendril_config(400, 20, 1.2, 0.1, start_location);
	};

	const auto set_tendrils = [&game, &level_editor, &gen_tendrils](Tree& tree) {
		Tendrils tendrils = { gen_tendrils(tree) };
		tree.branches = Tree::branches_from_tendrils(tendrils);
		tree.tendrils = tendrils;
		tree.init_texture();

		float rotation = level_editor.tree_metadatas[tree.id].rotation;
		level_editor.tree_metadatas[tree.id] = TreeMetadata(rotation, tree);
		level_editor.update_rotation(game);
	};

	Tree& tree = *game.trees[0];
	level_editor.tree_metadatas.push_back(TreeMetadata(0, tree));
	set_tendrils(tree);

	while (!WindowShouldClose()) {
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			tree.rand.set_seed(++tree.rand.seed);
			set_tendrils(tree);
		}
		if (IsKeyPressed(KEY_F)) {
			set_tendrils(tree);
		}
		BeginDrawing();
		ClearBackground(RAYWHITE);
		DrawText(game.petra.say_hello().c_str(), 200, 20, 20, GREEN);	

		level_editor.update(game);

		tree.render();

		const Vector2 mouse = GetMousePosition();
		for (auto& button : level_editor.buttons) {
			button.take_input(mouse);
			button.render();
		}

		EndDrawing();
	}	

	// UnloadShader(tree.shader);
	CloseWindow();
	return 0;
}