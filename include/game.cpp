#include <iostream>

#include "constants.cpp"
#include "game.hpp"

Game::~Game() {}

Game::Game(const int screen_width, const int screen_height, const int fps) {
	this->screen_width = screen_width;
	this->screen_height = screen_height;	

	level.init(screen_width, screen_height);

	set_fps(fps);
}

void Game::set_fps(int fps) {
	this->fps = fps;
	SetTargetFPS(fps);
}

Game* Game::get() {
	std::cout << "using game.get\n";
	return Game::_game;
}

void Game::make_tree() {
	Rand rand(69);
	ShaderWithCheck shader;
	load_shader(shader, "assets/tree.fs");

	// Black magic that is required to ensure trees are not created and copied, even though that would be fine.
	auto t = std::unique_ptr<Tree>(new Tree({}, shader, rand));
	t->id = trees.size();
	trees.emplace_back(std::move(t));
}