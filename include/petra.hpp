#ifndef PETRA_H
#define PETRA_H

#include <iostream>
#include <raylib.h>

struct Level;
struct Game;

struct Petra {
	Vector2 pos {};
	float depth = 0;
	std::string say_hello();

	Petra();

	void update(Level* level, Game& game);
};

#endif