#ifndef GAME_H
#define GAME_H

#include "petra.hpp"
#include "button.hpp"
#include "tree.hpp"
#include <memory>
#include <vector>

struct Game;

struct Game : Button::Owner {
	Petra petra;
	std::vector<std::unique_ptr<Tree>> trees;
	int screen_width = 0;
	int screen_height = 0;
	int fps = 0;

	Game(const int screen_width, const int screen_height, const int fps);

	// Not preferred due to lack of clarity, but for testing.
	static Game* _game;
	static Game* get();

	void make_tree();
	void set_fps(int fps);
};

Game* Game::_game = nullptr;

#endif