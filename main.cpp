#include "tree.cpp"
#include "petra.cpp"
#include "main.hpp"
#include "raylib.h"
#include <string>

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

	Petra petra;	

	// shader setup
	Shader tree_shader = LoadShader(0, TextFormat("include/tree_shader.fs", GLSL_VERSION));

	// Try using randomly generated tendrils too
	Vector2 start_location { 100, 100 };

	Rand rand(69);
	Tree tree({}, tree_shader, rand);
	const auto gen_tendrils = [&tree, &start_location]() {
		return tree.random_tendril_config(400, 20, 1.2, 0.1, start_location);
	};

	Tendrils tendrils = { gen_tendrils() };
	tree.branches = Tree::branches_from_tendrils(tendrils);
	tree.tendrils = tendrils;
	tree.init_texture();

	while (!WindowShouldClose()) {
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			tree.rand.set_seed(++tree.rand.seed);
			Main::clicks++;
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

		EndDrawing();
	}	

	UnloadShader(tree_shader);	
	CloseWindow();
	return 0;
}