// You can package images in c files: https://github.com/raysan5/raylib/blob/master/examples/others/embedded_files_loading.c

// Try to check memleaks by using wsl and some compile command like https://github.com/raysan5/raylib/issues/3570 (or just use fsanitize=leak)

// Is probably a given.
#define PLATFORM_DESKTOP

#include <sstream>
#include <string>
#include <memory>

#include "main.hpp"
#include "mylib.cpp"
#include "tree.cpp"
#include "petra.cpp"
#include "level_editor.cpp"
#include "button.cpp"
#include "game.cpp"

#include "raylib.h"

// TODO: Reuse the same buffer of textures and just use DrawTextureEx with the scale option. Of course, I hope that works alongside the tree shader.
// Or the dumb solution of making textures the size of the screen and just specifying a boundary as uniform
	// This dumb solution could involve some weird strategies of juggling textures of closer sizes to give to other trees, but ehhhh
// Also why is in the examples is RenderTexture? see raylib [shaders] example - mandelbrot set. This seems to be most promising.

#if defined(PLATFORM_DESKTOP)
	#define GLSL_VERSION 330
#else // PLATFORM_ANDROID, PLATFORM_WEB
	#define GLSL_VERSION 100
#endif

int main() {
	// SetTraceLogLevel(LOG_WARNING);
	const int screen_width = 800;
	const int screen_height = 600;
	const int fps = 60;
	InitWindow(screen_width, screen_height, "Petrichor");

	// To check that the scope of all variables is treated as expected
	{
		Game game(screen_width, screen_height, fps);
		game._game = &game;

		LevelEditor level_editor;
		level_editor.initialize_ui();
		auto metadata_zero = TreeMetadata::zero();
		level_editor.make_initialized_tree([&game]() { game.make_tree(); }, game, metadata_zero);
		auto& m = level_editor.tree_metadatas.back();
		std::cout << "calced, meta rect " << to_str({ m.mark.x, m.mark.y }, 2) << "\n";

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

		// do a bad, this is indeed caught by ubuntu -fsanitize=leak
		// void* volatile blah = malloc(1);
		// (void) blah;
	}

	CloseWindow();
	return 0;
}