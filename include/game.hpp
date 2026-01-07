#ifndef GAME_H
#define GAME_H

#include <memory>
#include <vector>

#include "petra.hpp"
#include "button.hpp"
#include "tree.hpp"
#include "states.cpp"
#include "level.hpp"

struct Game;

struct Game : Button::Owner {
	std::vector<std::unique_ptr<Tree>> trees;
	int screen_width = 0;
	int screen_height = 0;
	int fps = 0;
	State state = EditLevel;
	State last_state = EditLevel;
	Level level;

	Game(const int screen_width, const int screen_height, const int fps);
	~Game();

	// Not preferred due to lack of clarity, but for testing.
	static Game* _game;
	static Game* get();

	void make_tree();
	void set_fps(int fps);
};

Game* Game::_game = nullptr;

#endif