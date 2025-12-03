#include "level_editor.cpp"
#include "tree.cpp"
#include "petra.cpp"
#include "main.hpp"
#include "button.cpp"
#include "raylib.h"
#include <sstream>
#include <string>

#if defined(PLATFORM_DESKTOP)
	#define GLSL_VERSION 330
#else // PLATFORM_ANDROID, PLATFORM_WEB
	#define GLSL_VERSION 100
#endif

int main() {
	InitWindow(Main::screen_width, Main::screen_height, "Raylib basic window");
	SetTargetFPS(60);	

	Main main;
	Petra petra;	

	// shader setup
	Shader tree_shader = LoadShader(0, TextFormat("include/tree_shader.fs", GLSL_VERSION));

	// Try using randomly generated tendrils too
	Vector2 start_location { 100, 100 };

	Rand rand(69);
	Tree tree({}, tree_shader, rand);
	main.trees.push_back(tree);

	const auto gen_tendrils = [&tree, &start_location]() {
		return tree.random_tendril_config(400, 20, 1.2, 0.1, start_location);
	};

	Tendrils tendrils = { gen_tendrils() };
	tree.branches = Tree::branches_from_tendrils(tendrils);
	tree.tendrils = tendrils;
	tree.init_texture();

	// LevelEditor level_editor;
	// level_editor.initialize_ui();

	// for (size_t i = 0; i < main.trees.size(); i++)
	// 	level_editor.treeIndexesAndAddOns[i] = {};

	while (!WindowShouldClose()) {
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			tree.rand.set_seed(++tree.rand.seed);
			Tendrils tendrils = { gen_tendrils() };
			tree.branches = Tree::branches_from_tendrils(tendrils);
			tree.tendrils = tendrils;
			tree.init_texture();
		}
		if (IsKeyPressed(KEY_F)) {
			Tendrils tendrils = { gen_tendrils() };
			tree.branches = Tree::branches_from_tendrils(tendrils);
			tree.tendrils = tendrils;
			tree.init_texture();
		}
		BeginDrawing();
		ClearBackground(RAYWHITE);
		DrawText(petra.say_hello().c_str(), 200, 20, 20, GREEN);	

		tree.render();
		// level_editor.update(main);

		const Vector2 mouse = GetMousePosition();
		/*
		for (auto& button : level_editor.buttons) {
			button->take_input(mouse);
			button.render();
		}
		*/

		EndDrawing();
	}	

	UnloadShader(tree_shader);	
	CloseWindow();
	return 0;
}