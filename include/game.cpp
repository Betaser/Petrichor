#include "constants.cpp"
#include "game.hpp"

void Game::make_tree() {
	Rand rand(69);
	Shader tree_shader = LoadShader(0, TextFormat("include/tree_shader.fs", Constants::glsl_version));

	// Black magic that is required to ensure trees are not created and copied, even though that would be fine.
	auto t = std::unique_ptr<Tree>(new Tree({}, tree_shader, rand));
	t->id = trees.size();
	trees.push_back(std::move(t));
}