#include <algorithm>

#include "level.hpp"
#include "mylib.hpp"

Level::Level() {
	camera.lens_mult = 0.01;
	petra.pos = { 350, 300 };
	petra.depth = -100;
}

void Level::update(Game& game) {
	petra.update(this, game);
}

void Level::render(Game& game) {
	Vector2 dims { 500, 300 };
	Rectangle clip {
		.x = ((float) game.screen_width - dims.x) / 2,
		.y = ((float) game.screen_height - dims.y) / 2,
		.width = dims.x,
		.height = dims.y
	};
	// Step 0: Overlay a gray color over everything to separate previous tree renders from current.
	DrawRectangle(0, 0, game.screen_width, game.screen_height, { 0, 0, 0, 200 });
	// Step 1: Render out some transparent overlay to show where the clip is
	DrawRectangleRec(clip, { 255, 0, 0, 100 });

	camera.pos = petra.pos;
	camera.screen_offset = { (float) game.screen_width / 2, (float) game.screen_height / 2 };

	// Render in reverse depth order
	std::vector<Tree*> trees(game.trees.size());
	for (size_t i = 0; i < game.trees.size(); i++)
		trees[i] = game.trees[i].get();
	std::sort(trees.begin(), trees.end(), 
		[](Tree* t1, Tree* t2) { return t1->depth > t2->depth; });

	for (const auto& tree_ptr : trees) {
		auto& tree = *tree_ptr;
		Cam depth_cam = camera.clone();
		depth_cam.scale = 1.0 / ((tree.depth - petra.depth) * depth_cam.lens_mult);

		// We are past it then.
		const float epsilon = 0;
		if (petra.depth > tree.depth + epsilon) {
			continue;
		}
		tree.render(depth_cam, clip);
	}

	DrawRectangleV(camera.screen_offset, { 10, 10 }, { 45, 20, 45, 255 });
}