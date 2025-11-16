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
	Shader tree_shader = LoadShader(0, TextFormat("tree_shader.fs", GLSL_VERSION));

	Tree tree({
		// Remember this will get flipped during rendering.
		// tilted to the left trapezoid.
		Branch(std::vector<Vector2>{
			{-2.66, 1.98},
			{-1, 2.85},
			{2, 0},
			{-2.03, -2.04}
		}),
		// rightside up trapezoid
		Branch(std::vector<Vector2>{
			{-1, 1.5},
			{1, 1.5},
			{2, -2},
			{-2, -2}
		}),
	}, tree_shader);
	tree.init_texture();

	while (!WindowShouldClose()) {
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