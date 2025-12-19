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

    // Not preferred due to lack of clarity, but for testing.
    static Game* _game;
    static Game* get();

    void make_tree();
};

Game* Game::_game = nullptr;

#endif