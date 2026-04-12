// You can package images in c files: https://github.com/raysan5/raylib/blob/master/examples/others/embedded_files_loading.c

// Try to check memleaks by using wsl and some compile command like https://github.com/raysan5/raylib/issues/3570 (or just use fsanitize=leak)

// Is probably a given.
#define PLATFORM_DESKTOP

#include <sstream>
#include <fstream>
#include <string>
#include <memory>

#include "mylib.cpp"
#include "main.hpp"
#include "level_editor.cpp"
#include "tree.cpp"
#include "button.cpp"
#include "game.cpp"
#include "petra.cpp"
#include "pause_menu.cpp"
#include "level.cpp"
#include "camera.cpp"

#include <raylib.h>

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
	SetTraceLogLevel(LOG_WARNING);

	const int screen_width = 800;
	const int screen_height = 600;
	const int fps = 60;
	InitWindow(screen_width, screen_height, "Petrichor");

	// To check that the scope of all variables is treated as expected
	{
		auto img = GenImageColor(1, 1, BLANK);
		load_texture_from_image(Main::dummy_tex, img);
		UnloadImage(img);

		Game game(screen_width, screen_height, fps);
		game._game = &game;

		// PauseMenu pause_menu(game);

		// Load tree tex once
		load_texture(Tree::static_tree_tex, "assets/tree_texture.png");

		LevelEditor level_editor;
		auto metadata_zero = TreeMetadata::zero();
		level_editor.make_initialized_tree([&game]() { game.make_tree(); }, game, metadata_zero);

		PauseMenu pause_menu(game, &level_editor);

		while (!WindowShouldClose()) {
			pause_menu.update();

			switch (game.state) {
				case PlayLevel: {
					// Load in the trees
					// Eventually, do something close to this but with metadatas for the level editor so progress can be saved in editing levels.
					if (game.last_state == EditLevel) {
						// But refill game.trees with our edit level contents.
						for (auto& tree : game.trees)
							level_editor.saved_trees.emplace_back(std::move(tree));
						game.load_trees(
							Constants::test_level_path,
							[](TreeMetadata& meta, Tree& tree) {
								const Vector2 origin = tree.origin();
								for (size_t i = 0; i < tree.branches.size(); i++) {
									auto& verts = tree.branches[i].verts;
									for (size_t j = 0; j < verts.size(); j++)
										verts[j] = rotate(origin, verts[j], meta.rotation) + meta.offset;
								}
							});
					}

					if (game.last_state == EditLevel && false) {
						// But refill game.trees with our edit level contents.
						for (auto& tree : game.trees)
							level_editor.saved_trees.emplace_back(std::move(tree));

						game.trees.clear();
						std::cout << "load in the trees\n";
						std::string line;
						std::ifstream file;
						file.open(Constants::test_level_path);

						float rotation;
						Vector2 offset;
						int seed;
						float depth;

						std::string name;
						while (!file.eof()) {
							std::getline(file, line);
							const size_t separator_loc = line.find(":");

							if (separator_loc == std::string_view::npos)
								break;

							name = line.substr(0, separator_loc);
							const auto value = line.substr(separator_loc + 1);

							if (name == "rotation") {
								rotation = std::stof(value);
							} 
							else if (name == "offset") {
								const size_t xy_sep = value.find(" ");
								float x = std::stof(value.substr(0, xy_sep));
								float y = std::stof(value.substr(xy_sep + 1));
								offset = { x, y };
							} 
							else if (name == "seed") {
								seed = std::stoi(value);
							} 
							else if (name == "depth") {
								depth = std::stof(value);

								game.make_tree();
								auto& tree = game.trees.back();
								tree->depth = depth;
								tree->rand = Rand(seed);
								tree->id = game.trees.size();
								const Vector2 start_location { 100, 100 };
								std::vector<std::vector<Branch>> tendrils = tree->random_tendril_config(400, 20, 1.2, 0.1, start_location);
								tree->branches = Tree::branches_from_tendrils(tendrils);
								tree->tendrils = tendrils;
								const Vector2 origin = tree->origin();
								for (size_t i = 0; i < tree->branches.size(); i++) {
									auto& verts = tree->branches[i].verts;
									for (size_t j = 0; j < verts.size(); j++) {
										verts[j] = rotate(origin, verts[j], rotation) + offset;
									}
								}

								tree->update_texture();
							}
						}
						file.close();
					}

					game.level.update(game);
				} 
				break;
				case EditLevel: {
					if (game.last_state != EditLevel) {
						game.trees.clear();

						// But refill game.trees with our edit level contents.
						std::cout << "level_editor size " << level_editor.saved_trees.size() << "\n"; 
						for (auto& tree : level_editor.saved_trees)
							game.trees.emplace_back(std::move(tree));
						level_editor.saved_trees.clear();

						level_editor.time = 0;
						/*
						level_editor.deleted_tree_ids.clear();
						level_editor.tree_metadatas.clear();
						level_editor.using_depth_ui = false;
						level_editor.make_initialized_tree([&game]() { game.make_tree(); }, game, metadata_zero);
						*/
						level_editor.invalidate_selected_index(game);
					}
					level_editor.update(game);

					const Vector2 mouse = GetMousePosition();
					for (auto& button : level_editor.buttons) 
						button.take_input(mouse);
				}
				break;
				case Credits: break;
			}

			BeginDrawing();
			// We need to do some kind of draw call apparently before textures work.
			DrawRectangle(0, 0, 1, 1, BLANK);
			ClearBackground({ 200, 200, 200, 255 });
			DrawText(game.level.petra.say_hello().c_str(), 200, 20, 20, GREEN);	

			switch (game.state) {
				case PlayLevel: {
					game.level.render(game);
				}
				break;
				case EditLevel: {
					for (const auto& tree : game.trees)
						tree->render(&level_editor);

					level_editor.render(game);

					for (const auto& button : level_editor.buttons)
						button.render();
				}
				break;
				case Credits: break;
			}
			pause_menu.render(game.screen_width, game.screen_height);
			EndDrawing();

			game.last_state = game.state;
		}	

		// do a bad, this is indeed caught by ubuntu -fsanitize=leak
		// void* volatile blah = malloc(1);
		// (void) blah;
		unload_texture(Tree::static_tree_tex);
		unload_texture(Main::dummy_tex);

		std::cout << "static tree tex w/ id " << Tree::static_tree_tex.id << " loads/unloads " << Tree::static_tree_tex.load_unloads << "\n";
		std::cout << "dummy tex w/ id " << Main::dummy_tex.id << " loads/unloads " << Main::dummy_tex.load_unloads << "\n";
	}

	CloseWindow();
	return 0;
}
