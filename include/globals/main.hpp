#ifndef MAIN_H
#define MAIN_H

#include <raylib.h>

#include "mylib.hpp"

struct Main {
	public:
	static int clicks;
	static TextureWithCheck dummy_tex;
};

// ???
int Main::clicks = 0;
TextureWithCheck Main::dummy_tex;

#endif
