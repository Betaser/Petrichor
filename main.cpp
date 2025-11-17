#include "tree.cpp"
#include "petra.cpp"
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
	int seed = 69;
	Tendrils tendrils = { Tree::random_tendril_config(seed++, 300, 50, 1.2, -9999) };
	Tree tree(Tree::branches_from_tendrils(tendrils), tree_shader);
	tree.tendrils = tendrils;
	tree.init_texture();

	while (!WindowShouldClose()) {
		if (IsKeyPressed(KEY_F)) {
			Tendrils tendrils = { Tree::random_tendril_config(seed++, 300, 50, 1.2, -9999) };
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