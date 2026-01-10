#include "petra.hpp"
#include <string>

Petra::Petra() {
	std::cout << "initialized Petra\n";
}

void Petra::update(Level* level, Game& game) {
	(void) level;
	(void) game;
	if (IsKeyDown(KEY_A)) {
		pos.x -= 1;
	}
	if (IsKeyDown(KEY_D)) {
		pos.x += 1;
	}
	if (IsKeyDown(KEY_W)) {
		pos.y -= 1;
	}
	if (IsKeyDown(KEY_S)) {
		pos.y += 1;
	}
	float scroll = GetMouseWheelMove();
	if (scroll != 0) {
		depth += scroll;
	}
	else {
		if (IsKeyDown(KEY_UP))
			depth -= 1;
		if (IsKeyDown(KEY_DOWN))
			depth += 1;
	}
}

std::string Petra::say_hello() {
	return "hii";
}