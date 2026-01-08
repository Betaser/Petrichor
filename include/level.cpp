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
	trees_target = LoadRenderTexture(screen_width, screen_height);
}

Level::~Level() {
	unload_texture(fog_texture);
	std::cout << "fog texture loads/unloads " << fog_texture.load_unloads << "\n";
	unload_shader(fog_shader);
	std::cout << "ambient fog shader loads/unloads " << fog_shader.load_unloads << "\n";
	unload_shader(tree_foggy_blur_shader);
	std::cout << "tree foggy blur shader loads/unloads " << tree_foggy_blur_shader.load_unloads << "\n";
	UnloadRenderTexture(trees_target);
}

void Level::update(Game& game) {
	petra.update(this, game);

	render_tree_to_target(game);
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

	DrawTexture(trees_target.texture, 0, 0, WHITE);

	render_fog(game);
}

void Level::render_tree_to_target(Game& game) {
	Vector2 dims { 500, 300 };
	Rectangle clip {
		.x = ((float) game.screen_width - dims.x) / 2,
		.y = ((float) game.screen_height - dims.y) / 2,
		.width = dims.x,
		.height = dims.y
	};

	camera.pos = petra.pos;
	camera.screen_offset = { (float) game.screen_width / 2, (float) game.screen_height / 2 };

	// Render in reverse depth order
	std::vector<Tree*> trees(game.trees.size());
	for (size_t i = 0; i < game.trees.size(); i++)
		trees[i] = game.trees[i].get();
	std::sort(trees.begin(), trees.end(), 
		[](Tree* t1, Tree* t2) { return t1->depth > t2->depth; });

	BeginTextureMode(trees_target);
	ClearBackground(BLANK);
	for (const auto& tree_ptr : trees) {
		auto& tree = *tree_ptr;
		Cam depth_cam = camera.clone();
		depth_cam.scale = 1.0 / ((tree.depth - petra.depth) * depth_cam.lens_mult);

		// We are past it then.
		const float epsilon = 0;
		if (petra.depth > tree.depth + epsilon)
			continue;

		// tree.render_with_cam(depth_cam, clip);
		
		tree.render_with_cam_begin_end(depth_cam, clip);
		
		Rectangle dest {
			.x = (float) tree.texture_pos.x,
			.y = (float) tree.texture_pos.y,
			.width = (tree.big - tree.small).x,
			.height = (tree.big - tree.small).y
		};
		depth_cam.draw_texture(clip, tree.blank_tex, full_texture(tree.blank_tex), dest);

		/*
		// Apply tree_foggy_blur
		Rectangle dest {
			.x = (float) tree.texture_pos.x,
			.y = (float) tree.texture_pos.y,
			.width = (tree.big - tree.small).x,
			.height = (tree.big - tree.small).y
		};
		int tex_loc = GetShaderLocation(tree_foggy_blur_shader, "tex");
		SetShaderValueTexture(tree_foggy_blur_shader, tex_loc, tree.blank_tex);
		depth_cam.draw_texture_begin_end(tree_foggy_blur_shader, clip, tree.blank_tex, full_texture(tree.blank_tex), dest);
		*/
	}
	EndTextureMode();
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