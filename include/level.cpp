#include <algorithm>

#include "level.hpp"
#include "mylib.hpp"

Level::Level() {
	camera.lens_mult = 0.01;

	petra.pos = { 350, 300 };
	petra.depth = -100;

	auto img = GenImageColor(10, 10, BLANK);
	load_texture_from_image(fog_texture, img);
	UnloadImage(img);
	load_shader(fog_shader, "assets/ambient_fog.fs");
	load_shader(tree_foggy_blur_shader, "assets/tree_foggy_blur.fs");
}

void Level::init(int screen_width, int screen_height) {
	(void) screen_width;
	(void) screen_height;
}

Level::~Level() {
	unload_texture(fog_texture);
	std::cout << "fog texture loads/unloads " << fog_texture.load_unloads << "\n";
	unload_shader(fog_shader);
	std::cout << "ambient fog shader loads/unloads " << fog_shader.load_unloads << "\n";
	unload_shader(tree_foggy_blur_shader);
	std::cout << "tree foggy blur shader loads/unloads " << tree_foggy_blur_shader.load_unloads << "\n";
}

void Level::update(Game& game) {
	petra.update();

	render_trees_to_target(game);
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

	// Indicate the center of where zooming happens
	DrawRectangleV(camera.screen_offset, { 10, 10 }, { 45, 20, 45, 255 });

	// Render in reverse depth order
	std::vector<Tree*> trees(game.trees.size());
	for (size_t i = 0; i < game.trees.size(); i++)
		trees[i] = game.trees[i].get();
	std::sort(trees.begin(), trees.end(), 
		[](Tree* t1, Tree* t2) { return t1->depth > t2->depth; });

	for (const auto& tree_ptr : trees) {
		auto& tree = *tree_ptr;

		// We are past it then.
		const float epsilon = 0;
		if (petra.depth > tree.depth + epsilon)
			continue;

		const float dist = dist_from_cam(tree);
		
		const int dfc_loc = GetShaderLocation(tree_foggy_blur_shader, "distFromCam");
		SetShaderValue(tree_foggy_blur_shader, dfc_loc, &dist, SHADER_UNIFORM_FLOAT);

		const int cd_loc = GetShaderLocation(tree_foggy_blur_shader, "collisionDist");
		SetShaderValue(tree_foggy_blur_shader, cd_loc, &collision_dist, SHADER_UNIFORM_FLOAT);

		Cam depth_cam = camera.clone();
		depth_cam.scale = 1.0 / (dist * depth_cam.lens_mult);
		// Change depth_cam if the tree is past the collision point
		// You know what? This basic linear transition is not so bad
		if (dist < collision_dist) {
			Vector2 outwards = my_normalize(camera.pos - tree.branches[0].back());
			float norm = (collision_dist - dist) / collision_dist;
			Vector2 cam_offset = outwards * norm * std::max((float) game.screen_width, (float) game.screen_height);
			depth_cam.pos += cam_offset;
		}

		Rectangle dest {
			.x = (float) tree.texture_pos.x,
			.y = (float) tree.texture_pos.y,
			.width = (tree.big - tree.small).x,
			.height = (tree.big - tree.small).y
		};
		BeginShaderMode(tree_foggy_blur_shader);
		depth_cam.draw_texture(clip, tree.target.texture, full_texture(tree.target.texture), dest);
		EndShaderMode();
	}

	render_fog(game);

	petra.render(game, *this);
}

void Level::render_trees_to_target(Game& game) {
	camera.pos = petra.pos;
	camera.screen_offset = { (float) game.screen_width / 2, (float) game.screen_height / 2 };

	// BeginTextureMode(trees_target);
	// ClearBackground(BLANK);
	for (const auto& tree_ptr : game.trees) {
		auto& tree = *tree_ptr;
		// We are past it then.
		const float epsilon = 0;
		if (petra.depth > tree.depth + epsilon)
			continue;

		// Cam depth_cam = camera.clone();
		// depth_cam.scale = 1.0 / (dist_from_cam(tree) * depth_cam.lens_mult);

		tree.render_to_target();
	}
}

void Level::render_fog(Game& game) {
	Vector2 dims { (float) game.screen_width, (float) game.screen_height };
	int dims_loc = GetShaderLocation(fog_shader, "dims");
	SetShaderValue(fog_shader, dims_loc, &dims, SHADER_UNIFORM_VEC2);

	Rectangle screen_rect { 
		.x = 0, 
		.y = 0, 
		.width = dims.x,
		.height = dims.y
	};
	Cam static_camera = camera.clone();
	static_camera.pos = dims / 2;
	static_camera.draw_texture_begin_end(
		fog_shader,
		screen_rect,
		fog_texture,
		full_texture(fog_texture),
		screen_rect);
}

constexpr float Level::dist_from_cam(Tree& tree) {
	return tree.depth - petra.depth;
}
