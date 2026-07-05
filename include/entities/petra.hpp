#ifndef PETRA_H
#define PETRA_H

#include <raylib.h>
#include <cstdlib>
#include <memory>

struct Level;
struct Game;

#include "../globals/mylib.hpp"

struct Collision {
	size_t tree_index;
	size_t tendril_config_index;

	Collision(size_t tree_index, size_t tendril_config_index) {
		this->tree_index = tree_index;
		this->tendril_config_index = tendril_config_index;
	}
	~Collision() {}
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
	void update(Level& level, Game& game);
	void collision_detection(Level& level, Game& game);
	void debug_update_movement();
	void update_movement();
	void render(Game& game, Level& level);
};

#endif
