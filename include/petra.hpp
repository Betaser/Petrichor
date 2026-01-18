#ifndef PETRA_H
#define PETRA_H

#include <iostream>
#include <raylib.h>

struct Level;
struct Game;

#include "game.hpp"

struct Collision {
	size_t tree_index = 0;
	size_t tendril_index = 0;
	Collision(size_t tree_index, size_t tendril_index) {
		this->tree_index = tree_index;
		this->tendril_index = tendril_index;
	}
	~Collision() {
		std::cout << "destroyed collision\n";
	}
};

struct Petra {
	bool we_are_debugging = true;
	std::unique_ptr<Collision> collision = nullptr;
	float depth = 0;
	float hitbox_radius = 0;
	Vector2 pos {};

	Petra();

	Circle get_hitbox() const;
	std::string say_hello();
	void update(Level& level, std::vector<std::unique_ptr<Tree>>& trees);
	void collision_detection(Level& level, std::vector<std::unique_ptr<Tree>>& trees);
	void debug_update_movement();
	void update_movement();
	void render(Game& game, Level& level);
};

#endif
