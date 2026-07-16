#ifndef GAME_H
#define GAME_H

#include <memory>
#include <vector>

#include "../scene_elements/button.hpp"
#include "../entities/tree.hpp"
#include "../gamestates/states.cpp"
#include "../gamestates/level.hpp"

struct Game;
struct BranchMetadata;

struct Game {
	std::vector<std::unique_ptr<Tree>> trees;
	int screen_width = 0;
	int screen_height = 0;
	int fps = 0;
	State state = EditLevel;
	State last_state = EditLevel;
	float overall_time;
	Level level;

	Game(const int screen_width, const int screen_height, const int fps);
	~Game();

	// Not preferred due to lack of clarity, but for testing.
	static Game* _game;
	static Game* get();

	void load_trees(const char* filepath, std::function<void(BranchMetadata&, TendrilConfig*, size_t)> accept_metadata);
	void make_tree();
	void set_fps(int fps);
};

#endif
