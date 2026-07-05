// You can package images in c files: https://github.com/raysan5/raylib/blob/master/examples/others/embedded_files_loading.c

// Try to check memleaks by using wsl and some compile command like https://github.com/raysan5/raylib/issues/3570 (or just use fsanitize=leak)

// Is probably a given.
#define PLATFORM_DESKTOP

#include <string>
#include <raylib.h>

#include "mylib.cpp"
#include "../gamestates/level_editor.cpp"
#include "../entities/tree.cpp"
#include "../scene_elements/button.cpp"
#include "game.cpp"
#include "../entities/petra.cpp"
#include "../gamestates/pause_menu.cpp"
#include "../gamestates/level.cpp"
#include "../scene_elements/camera.cpp"

#if defined(PLATFORM_DESKTOP)
	#define GLSL_VERSION 330
#else // PLATFORM_ANDROID, PLATFORM_WEB
	#define GLSL_VERSION 100
#endif

int main() {
	// testing
	SetTraceLogLevel(LOG_WARNING);

	int screen_width = 800;
	int screen_height = 600;
	const int fps = 60;
	InitWindow(screen_width, screen_height, "Petrichor");

	// To check that the scope of all variables is treated as expected
	{
		Game game(screen_width, screen_height, fps);
		game._game = &game;

		// Load tree tex once
		load_texture(Tree::branch_sampling_tex, "assets/tree_texture.png");

		LevelEditor level_editor(game);
		game.make_tree();
		auto& tree = game.trees.back();
		Rand rand(69);
		level_editor.make_initialized_config(*tree, rand, { {}, 0 });

		PauseMenu pause_menu(game);

		bool slow_down = false;

		while (!WindowShouldClose()) {
			// Debugging
			if (IsKeyPressed(KEY_L)) {
				slow_down = !slow_down;
				game.set_fps(slow_down ? 5 : fps);
			}

			pause_menu.update();

			if (!pause_menu.active) {
				switch (game.state) {
					case PlayLevel: {
						// Load in the trees
						// Eventually, do something close to this but with metadatas for the level editor so progress can be saved in editing levels.
						if (game.last_state == EditLevel) {
							// But refill game.trees with our edit level contents.
							for (auto& tree : game.trees)
								level_editor.saved_trees.push_back(std::move(tree));
							game.load_trees(
								Constants::test_level2_path,
								[](BranchMetadata& meta, TendrilConfig* config, size_t _) {
									const Vector2 origin = config->origin();
									for (size_t i = 0; i < config->branches.size(); i++) {
										auto& verts = config->branches[i].verts;
										for (size_t j = 0; j < verts.size(); j++)
											verts[j] = rotate(origin, verts[j], meta.rotation) + meta.offset;
									}
								});
						}

						game.level.update(game);
					} 
					break;
					case EditLevel: {
						if (game.last_state != EditLevel) {
							game.trees.clear();

							// But refill game.trees with our edit level contents.
							std::println("level_editor size {}", level_editor.saved_trees.size());
							for (auto& tree : level_editor.saved_trees)
								game.trees.emplace_back(std::move(tree));
							level_editor.saved_trees.clear();

							level_editor.reinit(game);
						}

						level_editor.update(game);
					}
					break;
					case Credits: break;
				}
			}

			BeginDrawing();
			// We need to do some kind of draw call apparently before textures work.
			DrawRectangle(0, 0, 1, 1, BLANK);
			ClearBackground({ 200, 200, 200, 255 });

			switch (game.state) {
				case PlayLevel: {
					game.level.render(game);
				}
				break;
				case EditLevel: {
					level_editor.render(game);
				}
				break;
				case Credits: break;
			}

			DrawText(game.level.petra.say_hello().c_str(), 200, 20, 20, GREEN);	
			DrawFPS(20, 20);
			pause_menu.render(game.screen_width, game.screen_height);
			EndDrawing();

			game.last_state = game.state;
		}	

		// do a bad, this is indeed caught by ubuntu -fsanitize=leak
		// void* volatile blah = malloc(1);
		// (void) blah;
		unload_texture(Tree::branch_sampling_tex);

		std::println("branch sampling tex w/ id {} loads/unloads {}", Tree::branch_sampling_tex.id, Tree::branch_sampling_tex.load_unloads);
	}

	CloseWindow();
	return 0;
}
