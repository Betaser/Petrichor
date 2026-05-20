#include <algorithm>
#include <ranges>
#include <map>

#include "level.hpp"
#include "mylib.hpp"

static inline void walk_fn(const size_t branch_i, size_t& walk_i, std::vector<Vector4>& branch_tints, Tree& tree) {
	Vector3 start_tint { 1, 0, 0 };
	Vector3 end_tint { 0, 1, 0 };
	const float amt = ((float) walk_i) / (float) branch_tints.size();

	Vector3 interp_color = (end_tint - start_tint) * amt + start_tint;
	branch_tints[branch_i] = {
		.x = interp_color.x,
		.y = interp_color.y,
		.z = interp_color.z,
		.w = amt * 0.2f + 0.5f,
	};
	walk_i++;

	Branch& branch = tree.branches[branch_i];
	for (size_t i = 0; i < 4; i++)
		branch.verts[i] += Vector2(0.2, 0);

	for (const size_t next : branch.nexts)
		walk_fn(next, walk_i, branch_tints, tree);
}

// The stuff for flatten_tree's final form
static float dist(Vector2 a, Vector2 b, Vector2 circle_pos) {
	const Vector2 projection = project_pt(circle_pos, { a, b });
	const float projection_dist_sum = length(projection - a) + length(projection - b);

	if (projection_dist_sum / length(a - b) > 1.0005) {
		return length(b - circle_pos);
		// const float pt_dist_a = length(a - circle_pos);
		// const float pt_dist_b = length(b - circle_pos);

		// return fmin(pt_dist_b, pt_dist_a);
	}

	const float line_dist = dist_pt_from_line(circle_pos, { a, b });
	return line_dist;
}

static std::tuple<Vector2, Vector2> to_wireframe(const Tree& tree, const size_t cur_i, const size_t next_i) {
	const Branch& branch = tree.branches[cur_i];
	if (branch.nexts.size() == 0) {
		// End branch
		return { branch.back(), branch.front() };
	}
	return { branch.back(), tree.branches[next_i].back() };
}

// TODO: Unfortunately this is wrong.
static float vert_angle(const Vector2& a, const Vector2& b, const Circle& circle) {
	// For 2 circles intersecting, one from a to b and the other c (for circle),
	// b' and b' in the wrong theta direction can be found. Finding the correct of those 2 
	// aforementioned solutions requires comparing the rhr-ness of a-b-c.pos vs a-b'-c.pos, 
	// which should match

	const Vector2 delta = a - circle.pos;
	const float d = length(delta);
	const float r = length(a - b);

	const float a_to_midpt = (pow(r, 2) - pow(circle.radius, 2) + pow(d, 2)) / (2 * d);
	const Vector2 midpt = a + (circle.pos - a) * (a_to_midpt / d);
	const float perp_length = sqrt(pow(r, 2) - pow(a_to_midpt, 2));
	const Vector2 perp_out = perp_rhr(delta) * perp_length / d;
	const Vector2 solns[2] = { midpt + perp_out, midpt - perp_out };

	// Works nicely for some situations, but maybe one of the solns is being picked when the other should be chosen?

	const bool rhr_aligned = (rhr_sign(a, b, circle.pos) < 0) == (rhr_sign(a, solns[0], circle.pos) < 0);
	const Vector2 b_new = rhr_aligned
		? solns[0]
		: solns[1];
	
	const float theta = angle_from(b_new - a, b - a) * 
		(rhr_aligned ? -1 : 1);

	return theta;
}

// Not correct yet
static float line_angle(const Vector2& a, const Vector2& b, const Circle& circle) {
	const float opp_over_hyp = circle.radius / length(circle.pos - a);
	const float bnew_angle = asin(opp_over_hyp);
	const float b_angle = angle_from(b - a, circle.pos - a) * (rhr_sign(a, b, circle.pos) > 0 ? -1 : 1);

	return bnew_angle - b_angle;
}

void rotate_all(const size_t branch_i, const float amt, const Vector2 origin, Tree& tree) {
	Branch& branch = tree.branches[branch_i];

	for (auto& vert : branch.verts) {
		auto rotated = rotate(origin, vert, amt);
		vert.x = rotated.x;
		vert.y = rotated.y;
	}

	for (const size_t next : branch.nexts)
		rotate_all(next, amt, origin, tree);
};


static bool flatten(Tree& tree, const size_t cur_i, const Vector2 a, const Vector2 b, const Circle& circle, float* debug_angle) {
	const float ab_dist = dist(a, b, circle.pos);

	// Check if the closer dist is even intersecting at all, if not return, thereby doing nothing.
	// std::cout << "ab_dist " << ab_dist << "\n";

	if (ab_dist > circle.radius) {
		// std::cout << "return false!\n";
		return false;
	}

	// And just do nudge stuff here instead
	const Vector2 perp_pt = project_pt(circle.pos, { a, b });

	float angle = line_angle(a, b, circle);
	if (isnan(angle))
		angle = vert_angle(a, b, circle);
	else
		angle = fmax(angle, vert_angle(a, b, circle));
	(void) perp_pt;
		
	// if (length(perp_pt - a) < length(b - a)) {
	// 	if (circle.radius < length(circle.pos - a))
	// 		angle = line_angle(a, b, circle);
	// 	else
	// 		angle = vert_angle(a, b, circle);
	// } else
	// 	angle = vert_angle(a, b, circle);
	// const float angle = 
	// 	length(perp_pt - a) < length(b - a) ?
	// 	line_angle(a, b, circle) :
	// 	vert_angle(a, b, circle);

	// if (isnan(angle)) {
	// 	const float opp_over_hyp = circle.radius / length(circle.pos - a);
	// 	if (abs(opp_over_hyp) > 1.0) {
	// 		std::cout << "opp_over_hyp" << opp_over_hyp << "\n";
	// 	}
	// 	std::cout << (length(perp_pt - a) < length(b - a) ? "line angle is nan" : "vert angle is nan") << "\n";
	// }

	// const float angle = vert_angle(a, b, circle);
		
	// Rotate the closer branch to edge of the circle
	if (Level::debug_apply_rotation)
		rotate_all(cur_i, angle, tree.branches[cur_i].back(), tree);
	*debug_angle = angle;
	return true;
}

// This assume cur_i has 2 nexts, which both need to be rotated.
void flatten_fork(Tree& tree, const size_t cur_i, const Circle& circle) {
	const Branch& branch = tree.branches[cur_i];
	const auto& nexts = branch.nexts;
	if (nexts.size() != 2)
		std::cout << "\n\n\n\n\nERROR: flatten fork called on this many branches " << nexts.size() << "\n";	

	auto [a, b] = to_wireframe(tree, cur_i, nexts[0]);
	auto [c, d] = to_wireframe(tree, cur_i, nexts[1]);

	size_t closer_branch_i = nexts[0];
	size_t further_branch_i = nexts[1];
	const float ab_dist = dist(a, b, circle.pos);
	const float cd_dist = dist(c, d, circle.pos);
	if (ab_dist > cd_dist) {
		std::swap(closer_branch_i, further_branch_i);
		std::swap(a, c);
		std::swap(b, d);
	}

	// Check if the closer dist is even intersecting at all, if not return, thereby doing nothing.
	if (fmin(ab_dist, cd_dist) > circle.radius)
		return;

	// And just do nudge stuff here instead
	const Vector2 perp_pt = project_pt(circle.pos, { a, b });
	const float angle = 
		length(perp_pt - a) < length(b - a) ?
		line_angle(a, b, circle) :
		vert_angle(a, b, circle);

	// Rotate the closer branch to edge of the circle
	rotate_all(closer_branch_i, angle, tree.branches[closer_branch_i].back(), tree);

	// Rotate the further branch a smaller amount, but still ensuring its in front
	const float closer_to_further_angle = angle_from(d - c, b - a);
	const float angle_from_closer = angle + closer_to_further_angle * 0.8;
	const float further_angle = angle_from_closer - closer_to_further_angle;
	rotate_all(further_branch_i, further_angle, tree.branches[closer_branch_i].back(), tree);
}

void Dome::flatten_tree(Tree& tree, const Circle& circle, std::map<std::string, std::string>& debug) const {
	// Debugging

	if (true) {
		// We have to like have a buffer that gets smaller the more we "walk"
		const std::function<void(size_t, size_t, Circle)> walk = [&walk, &tree, &debug](const size_t walk_i, const size_t branch_depth, Circle circle) {
			// Circle radius buffer
			float radius_buffer = circle.radius * 0.001;
			for (size_t i = 0; i < branch_depth; i++)
				radius_buffer *= 0.5;
			circle.radius += radius_buffer;

			const Branch& branch = tree.branches[walk_i];

			if (branch.nexts.size() == 2) {
				// Let's isolate the problem
				// flatten_fork(tree, walk_i, circle);
			}
			else if (branch.nexts.size() == 1) {
				const size_t next_i = branch.nexts[0];
				auto [a, b] = to_wireframe(tree, walk_i, next_i);
				float debug_angle;
				if (flatten(tree, walk_i, a, b, circle, &debug_angle)) {
					debug["should_flatten"] = "true";
					// std::cout << "branches to flatten " << walk_i << "\n";
					// std::cout << "branches angle " << debug_angle << "\n";
				}
				// This should do the job of flattening the child nodes as well.
				const Branch& next_branch = tree.branches[next_i];
				const size_t next_next_i = next_branch.nexts.size() > 0 
					? next_branch.nexts[0] 
					: 0;
				auto [next_a, next_b] = to_wireframe(tree, next_i, next_next_i);
				if (flatten(tree, next_i, next_a, next_b, circle, &debug_angle)) {
					debug["should_flatten"] = "true";
					std::cout << "tip angle " << debug_angle << "\n";
				}
			}
			else if (branch.nexts.size() == 0) {
				// const Vector2 a = branch.back();
				// const Vector2 b = branch.front();
				// if (flatten(tree, walk_i, a, b, circle)) {
				// 	debug["should_flatten"] = "true";
				// 	// std::cout << "tips to flatten " << walk_i << "\n";
				// }
				// std::cout << " last branch " << walk_i;
			}
			else
				std::cout << "Unexpectedly we have this many branches: " << branch.nexts.size() << "\n";

			for (const auto& next : branch.nexts)
				walk(next, branch_depth + 1, circle);
		};
		walk(0, 0, circle);
	
		tree.update_texture();
	}
	
	if (false) {
		// Let's try rotating multiple parts instead of just the index 2.
		// Must wrap this into a loop then.
		const float rotate_amt = 0.002;

		const std::function<void(size_t, float, Vector2)> rotate_all = [&rotate_all, &tree](const size_t branch_i, const float amt, const Vector2 origin) {
			Branch& branch = tree.branches[branch_i];

			for (auto& vert : branch.verts) {
				auto rotated = rotate(origin, vert, amt);
				vert.x = rotated.x;
				vert.y = rotated.y;
			}

			for (const size_t next : branch.nexts)
				rotate_all(next, amt, origin);
		};

		const std::function<void(size_t)> walk = [&walk, &tree, &rotate_amt, &rotate_all](const size_t walk_i) {
			const Branch& branch = tree.branches[walk_i];

			// Simulate some selectivity
			if (walk_i >= 2) {
				rotate_all(walk_i, rotate_amt, branch.back());
			}

			for (const auto& next : branch.nexts)
				walk(next);
		};
		walk(0);
		
		tree.update_texture();
	}

	if (false) {
		int loc = GetShaderLocation(tree.tendril_shader, "debugBranchTints");
		std::vector<Vector4> branch_tints(tree.branches.size());

		size_t walk_i = 2;
		// walk_fn(2, walk_i, branch_tints, tree);

		// static std::map<size_t, std::vector<Branch>> orig_branches;
		// static std::map<size_t, float> frames;

		// if (!frames.contains(tree.id))
		// 	frames[tree.id] = 0;
		// else
		// 	frames[tree.id]++;

		// if (!orig_branches.contains(tree.id)) {
		// 	orig_branches[tree.id] = {};
		// 	for (auto branch : tree.branches)
		// 		orig_branches[tree.id].push_back(branch);
		// }

		const std::function<void(size_t)> walk = [&walk, &walk_i, &branch_tints, &tree](const size_t branch_i) {
			Vector3 start_tint { 1, 0, 0 };
			Vector3 end_tint { 0, 1, 0 };
			const float amt = ((float) walk_i) / (float) branch_tints.size();

			Vector3 interp_color = (end_tint - start_tint) * amt + start_tint;
			branch_tints[branch_i] = {
				.x = interp_color.x,
				.y = interp_color.y,
				.z = interp_color.z,
				.w = amt * 0.2f + 0.5f,
			};
			walk_i++;

			Branch& branch = tree.branches[branch_i];
			for (size_t i = 0; i < 4; i++) {
				branch.verts[i] += Vector2(0.05, 0);
				// const auto offset = Vector2(0, 0.25) * frames[tree.id];
				// branch.verts[i] = orig_branches[tree.id][branch_i].verts[i] + offset;
			}

			for (const size_t next : branch.nexts)
				walk(next);
		};
		walk(2);

		tree.update_texture();
		SetShaderValueV(tree.tendril_shader, loc, branch_tints.data(), SHADER_UNIFORM_VEC4, branch_tints.size());
	}

	// The default order of the flat vector tree.branches is not exactly what we need, because we need to choose the nearer branch when branches happen.
	if (false) {
		int loc = GetShaderLocation(tree.tendril_shader, "debugBranchTints");
		std::vector<Vector4> branch_tints(tree.branches.size());
		// Debugging it:
		Vector3 start_tint { 1, 0, 0 };
		Vector3 end_tint { 0, 1, 0 };
		for (size_t i = 0; i < branch_tints.size(); i++) {
			std::cout << tree.branches[i].nexts.size() << "\n";
			const float amt = ((float) tree.branches[i].nexts.size()) / 2.0;
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
	// std::cout << "todo: flatten tree\n";
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
		// .max_radius = std::sqrt(collision_dist) * 20,
		.max_radius = std::sqrt(collision_dist) * 4,
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

	debug.clear();

	if (IsKeyPressed(KEY_R))
		debug_apply_rotation = !debug_apply_rotation;

	for (const auto& tree_ptr : game.trees) {
		auto& tree = *tree_ptr;
		if (!tree.past_me(petra))
			continue;

		const auto& dist = dist_from_cam(tree);
		if (dist >= collision_dist)
			continue;

		const float radius = std::min(dome.max_radius, dome.depth_to_radius_fn(dist));
		const float dist_tree_dome = length(dome.pos - tree.origin());
		// std::cout << "radius " << radius << " dist tree dome " << dist_tree_dome << "\n";

		if (dist_tree_dome < radius)
			debug["too_close"] = "true";
		else {
			// Then we may be too far, it depends on what flatten_tree determines.
			const Circle dome_circle {
				.pos = dome.pos,
				.radius = radius
			};
			// std::cout << "dome circle pos " << to_str(dome_circle.pos, 2) << " radius " << dome_circle.radius << "\n";

			// Before we flatten, use the branch structure of the original tree!
			std::vector<Branch> branches;
			for (const auto& branch : tree.original_branches) {
				Branch b(branch.verts);
				b.nexts = branch.nexts;
				branches.push_back(b);
			}
			tree.branches = branches;

			dome.flatten_tree(tree, dome_circle, debug);
		}
	}
}

void Level::render(Game& game) {
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

	render_trees_to_target(game);

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
		if (tree.id == 0) {
			// printf("collision dist %f dist %f\n", collision_dist, dist);
		}
		// Change depth_cam if the tree is past the collision point
		// You know what? This basic linear transition is not so bad
		
		if (false && dist < collision_dist) {
			Vector2 outwards = normalize(camera.pos - tree.origin());
			float norm = (collision_dist - dist) / collision_dist;
			Vector2 cam_offset = outwards * norm * 0.05 * std::max((float) game.screen_width, (float) game.screen_height);
			depth_cam.pos += cam_offset;
		}
		
		// This dest perfectly matches the bounding_box call that yields a diff result every time verts is adjusted
		// But the jitter is fairly hard to tell now.
		Rectangle dest {
			.x = tree.texture_pos.x,
			.y = tree.texture_pos.y,
			.width = (tree.big - tree.small).x,
			.height = (tree.big - tree.small).y
		};
		BeginShaderMode(tree_foggy_blur_shader);
		// Render tree.target.texture to the screen
		depth_cam.draw_texture(clip, tree.target.texture, full_texture(tree.target.texture), dest);
		EndShaderMode();
	}

	render_fog(game);

	petra.render(game, *this);

	// Debugging
	if (debug["too_close"] == "true")
		DrawCircle(100, 100, 10, RED);
	if (debug["should_flatten"] == "true")
		DrawCircle(150, 100, 15, GREEN);
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