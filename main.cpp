// You can package images in c files: https://github.com/raysan5/raylib/blob/master/examples/others/embedded_files_loading.c

// Is probably a given.
#define PLATFORM_DESKTOP

#include "main.hpp"
#include "mylib.cpp"
#include "tree.cpp"
#include "petra.cpp"
#include "level_editor.cpp"
#include "button.cpp"
#include "game.cpp"

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
	// SetTraceLogLevel(LOG_WARNING);
	const int screenWidth = 800;
	const int screenHeight = 600;
	InitWindow(screenWidth, screenHeight, "Raylib basic window");
	SetTargetFPS(60);	

	Game game;
	game._game = &game;

	LevelEditor level_editor;
	level_editor.initialize_ui();
	auto metadata_zero = TreeMetadata::zero();
	level_editor.make_initialized_tree(game, metadata_zero);

	while (!WindowShouldClose()) {
		BeginDrawing();
		// We need to do some kind of draw call apparently before textures work.
		DrawRectangle(0, 0, 1, 1, BLANK);

		ClearBackground({ 200, 200, 200, 255 });
		DrawText(game.petra.say_hello().c_str(), 200, 20, 20, GREEN);	

		level_editor.update(game);

		const Vector2 mouse = GetMousePosition();
		for (auto& button : level_editor.buttons) 
			button.take_input(mouse);

		for (auto& tree : game.trees)
			tree->render();

		level_editor.render(game);

		for (const auto& button : level_editor.buttons)
			button.render();

		EndDrawing();
	}	

	CloseWindow();
	return 0;
}