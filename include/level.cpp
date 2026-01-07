#include "level.hpp"
#include "mylib.hpp"

void Level::update(Game& game) {
	/*
	std::vector<Vector2> vs { { 0, 1 }, { 1, 0 }, { 2, 2 } };
	std::vector<Vector2*> v_refs(vs.size());
	for (size_t i = 0; i < vs.size(); i++)
		v_refs[i] = &vs[i];

	camera.transform(v_refs);
	for (auto& v : vs)
		std::cout << to_str(v, 2) << "\n";
	*/

	/*
	Vector2 dims { 500, 300 };
	Rectangle clip {
		.x = ((float) game.screen_width - dims.x) / 2,
		.y = ((float) game.screen_height - dims.y) / 2,
		.width = dims.x,
		.height = dims.y
	};
	*/
	Rectangle clip {
		.x = 300,
		.y = 200,
		.width = 100,
		.height = 100
	};
	// Step 0: Overlay a gray color over everything to separate previous tree renders from current.
	DrawRectangle(0, 0, game.screen_width, game.screen_height, { 0, 0, 0, 200 });
	// Step 1: Render out some transparent overlay to show where the clip is
	DrawRectangleRec(clip, { 255, 0, 0, 100 });

	for (const auto& tree_ptr : game.trees) {
		auto& tree = *tree_ptr;
		tree.render(camera, clip);
		/*
		auto tex = tree.blank_tex;
		Rectangle src {
			.x = 0,
			.y = 0,
			.width = (float) tex.width,
			.height = (float) tex.height
		};
		Vector2 small, big;
		tree.bounding_box(small, big);
		Rectangle dest {
			.x = (float) tree.texture_pos.x,
			.y = (float) tree.texture_pos.y,
			.width = (big - small).x,
			.height = (big - small).y
		}; 
		camera.draw_texture(tree.shader, game.screen_width, game.screen_height, tex, src, dest);
		*/
	}
}