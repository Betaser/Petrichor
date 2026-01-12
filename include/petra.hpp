#ifndef PETRA_H
#define PETRA_H

#include <iostream>
#include <raylib.h>

struct Level;
struct Game;

#include "game.hpp"

struct Petra {
	bool we_are_debugging = true;
	float depth = 0;
	float hitbox_radius = 0;
	Vector2 pos {};

	Petra();

	Circle get_hitbox() const;
	std::string say_hello();
	void update();
	void debug_update_movement();
	void update_movement();
	void render(Game& game, Level& level);
};

#endif
