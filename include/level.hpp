#ifndef LEVEL_H
#define LEVEL_H

#include "petra.hpp"
#include "camera.hpp"
#include "game.hpp"

struct Level {
	Petra petra;
    Cam camera;

    void update(Game& game);
};

#endif