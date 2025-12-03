#ifndef MAIN_H
#define MAIN_H

#include <vector>
#include "include/tree.hpp"

class Main {
    public:
    static int clicks;
    static int screen_width;
    static int screen_height;
    std::vector<Tree> trees;
};

int Main::screen_width = 800;
int Main::screen_height = 600;
int Main::clicks = 0;

#endif