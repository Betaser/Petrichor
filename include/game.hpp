#ifndef GAME_H
#define GAME_H

#include "petra.hpp"
#include "tree.hpp"
#include <memory>
#include <vector>

struct Game {
    Petra petra;
	std::vector<std::unique_ptr<Tree>> trees;

    void make_tree();
};

#endif