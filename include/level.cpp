#include <algorithm>
#include <ranges>

#include "level.hpp"
#include "mylib.hpp"

void Dome::flatten_tree(Tree& tree) const {
	// Debugging
	
	// The default order of the flat vector tree.branches is not exactly what we need, because we need to choose the nearer branch when branches happen.

	// Below is flat vector traversal, which is close to what we want
	if (false) {
		int loc = GetShaderLocation(tree.tendril_shader, "debugBranchTints");
		std::vector<Vector4> branch_tints(tree.branches.size());
		// Debugging it:
		Vector3 start_tint { 1, 0, 0 };
		Vector3 end_tint { 0, 1, 0 };
		for (size_t i = 0; i < branch_tints.size(); i++) {
			const float amt = ((float) i) / (float) branch_tints.size();
			Vector3 interp_color = (end_tint - start_tint) * amt + start_tint;
			branch_tints[i] = {
				.x = interp_color.x,
				.y = interp_color.y,
				.z = interp_color.z,
				.w = amt * 0.2f + 0.5f,
			};
		}
		SetShaderValueV(tree.tendril_shader, loc, branch_tints.data(), SHADER_UNIFORM_VEC4, branch_tints.size());
	}
	std::cout << "todo: flatten tree\n";
}

Level::Level() {
	camera.lens_mult = 0.01;

	petra.pos = { 350, 300 };
	petra.depth = -100;

	auto img = GenImageColor(10, 10, BLANK);
	load_texture_from_image(fog_texture, img);
	UnloadImage(img);
	load_shader(fog_shader, "assets/ambient_fog.fs");
	load_shader(tree_foggy_blur_shader, "assets/tree_foggy_blur.fs");

	dome = {
		.pos = {},
		.max_radius = std::sqrt(collision_dist) * 20,
		.depth_to_radius_fn = [](float depth) {
			return std::sqrt(depth) * 20;
		}
	};
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
	petra.update(*this, game.trees);
	dome.pos = petra.pos;
}

void Level::render(Game& game) {
	for (const auto& tree_ptr : game.trees) {
		auto& tree = *tree_ptr;
		if (!tree.past_me(petra))
			continue;

		const auto& dist = dist_from_cam(tree);
		if (dist >= collision_dist)
			continue;

		const float radius = std::min(dome.max_radius, dome.depth_to_radius_fn(dist));
		const float dist_tree_dome = length(dome.pos - tree.origin());
		std::cout << "radius " << radius << " dist tree dome " << dist_tree_dome << "\n";
		if (dist_tree_dome < radius) {
			dome.flatten_tree(tree);
			std::cout << "flatten tree " << tree.id << "\n";
		}
	}

	render_trees_to_target(game);

	Vector2 dims { 700, 500 };
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
		const auto& tree = *tree_ptr;

		// We are past it then.
		if (!tree.past_me(petra))
			continue;

		const float dist = dist_from_cam(tree);
		
		const int dfc_loc = GetShaderLocation(tree_foggy_blur_shader, "distFromCam");
		SetShaderValue(tree_foggy_blur_shader, dfc_loc, &dist, SHADER_UNIFORM_FLOAT);

		const int cd_loc = GetShaderLocation(tree_foggy_blur_shader, "collisionDist");
		SetShaderValue(tree_foggy_blur_shader, cd_loc, &collision_dist, SHADER_UNIFORM_FLOAT);

		Cam depth_cam = calc_depth_cam(dist);
		// Change depth_cam if the tree is past the collision point
		// You know what? This basic linear transition is not so bad
		/*
		if (false && dist < collision_dist) {
			Vector2 outwards = my_normalize(camera.pos - tree.origin());
			float norm = (collision_dist - dist) / collision_dist;
			Vector2 cam_offset = outwards * norm * std::max((float) game.screen_width, (float) game.screen_height);
			depth_cam.pos += cam_offset;
		}
		*/

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
		if (!tree.past_me(petra))
			continue;

		tree.render_to_target();
	}
}

void Level::render_fog(Game& game) {
	Vector2 dims { (float) game.screen_width, (float) game.screen_height };
	int dims_loc = GetShaderLocation(fog_shader, "dims");
	SetShaderValue(fog_shader, dims_loc, &dims, SHADER_UNIFORM_VEC2);

	debug_render_dome_radii(game);

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

constexpr float Level::dist_from_cam(const Tree& tree) const {
	return tree.depth - petra.depth;
}

std::vector<std::tuple<size_t, float>> Level::calc_dome_radii(const Dome& dome, std::vector<std::unique_ptr<Tree>>& trees) const {
	// Hey, don't we have to scale the dome radius too, when drawing it?
	std::vector<std::tuple<size_t, float>> dome_radii;

	for (const auto& tree : trees) {
		if (!tree->past_me(petra))
			continue;	

		const auto& dist = dist_from_cam(*tree);

		if (dist >= collision_dist)
			continue;

		const Cam depth_cam = calc_depth_cam(dist);
		const float radius = std::min(dome.max_radius, dome.depth_to_radius_fn(dist));
		dome_radii.push_back({ tree->depth, radius * depth_cam.scale });
	}

	return dome_radii;
}

void Level::debug_render_dome_radii(Game& game) const {
	const auto& mapped_dome_radii = calc_dome_radii(dome, game.trees);
	static float radii_array[100];
	for (size_t i = 0; i < mapped_dome_radii.size(); i++)
		radii_array[i] = std::get<1>(mapped_dome_radii[i]);

	int radii_N_loc = GetShaderLocation(fog_shader, "domeRadiiN");
	const int N = mapped_dome_radii.size();
	SetShaderValue(fog_shader, radii_N_loc, &N, SHADER_UNIFORM_INT);
	int radii_array_loc = GetShaderLocation(fog_shader, "domeRadii");
	SetShaderValueV(fog_shader, radii_array_loc, &radii_array, SHADER_UNIFORM_FLOAT, N);
}

Cam Level::calc_depth_cam(float dist) const {
	Cam cam = camera.clone();
	cam.scale = 1.0 / (dist * cam.lens_mult);
	return cam;
}