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
enum IntersectionType {
	LINE,
	VERT
};

// Sometimes doesn't do LINE when it should.
static IntersectionType find_intersection(Vector2 a, Vector2 b, Vector2 circle_pos) {
	const Vector2 projection = project_pt(circle_pos, { a, b });

	// Ensure that the projection is inside of a and b
	if (!is_inside(projection, { a, b }, 0.001)) {
		return VERT;
	}
	return LINE;
}

static float dist(Vector2 a, Vector2 b, Vector2 circle_pos, IntersectionType intersection) {
	if (intersection == VERT)
		return length(b - circle_pos);

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

// Correct now?
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
	const auto intersection = find_intersection(a, b, circle.pos);
	const float ab_dist = dist(a, b, circle.pos, intersection);
	if (isnan(ab_dist)) {
		// std::println("ab_dist is nan, type was {}", intersection == VERT ? "VERT" : "LINE");
		// if (intersection == VERT)
		// 	std::println("for VERT, b {} circle.pos {}", to_str(b, 2), to_str(circle.pos, 2));
	}

	// Check if the closer dist is even intersecting at all, if not return, thereby doing nothing.

	if (ab_dist > circle.radius)
		return false;

	const float angle = intersection == VERT
		? vert_angle(a, b, circle)
		: line_angle(a, b, circle);
		
	// Rotate the closer branch to edge of the circle
	rotate_all(cur_i, angle, tree.branches[cur_i].back(), tree);
	*debug_angle = angle;
	return true;
}

// This assume cur_i has 2 nexts, which both need to be rotated.
bool flatten_fork(Tree& tree, const size_t cur_i, Circle& circle, const float interp_radius) {
	const Branch& branch = tree.branches[cur_i];
	const auto& nexts = branch.nexts;

	// First rotate our branch!
	{
		const auto either_back = nexts[0];
		auto [a, b] = to_wireframe(tree, cur_i, either_back);
		float blah;
		flatten(tree, cur_i, a, b, circle, &blah);
		if (isnan(blah)) {
			// std::println("nan angle for flatten_fork branch {}", cur_i);
		}					
	}

	circle.radius = interp_radius;

	const size_t ab_next_next_i = tree.branches[nexts[0]].nexts.size() > 0 
		? tree.branches[nexts[0]].nexts[0]
		: 0;
	const size_t cd_next_next_i = tree.branches[nexts[1]].nexts.size() > 0 
		? tree.branches[nexts[1]].nexts[0]
		: 0;

	auto [a, b] = to_wireframe(tree, nexts[0], ab_next_next_i);
	auto [c, d] = to_wireframe(tree, nexts[1], cd_next_next_i);

	size_t closer_branch_i = nexts[0];
	size_t further_branch_i = nexts[1];
	const auto ab_intersection = find_intersection(a, b, circle.pos);
	const auto cd_intersection = find_intersection(c, d, circle.pos);
	IntersectionType intersection = ab_intersection;
	const float ab_dist = dist(a, b, circle.pos, ab_intersection);
	const float cd_dist = dist(c, d, circle.pos, cd_intersection);
	if (isnan(ab_dist)) {
		// std::println("in flatten fork, ab_dist is nan, type was {}", ab_intersection == VERT ? "VERT" : "LINE");
	}
	if (ab_dist > cd_dist) {
		std::swap(closer_branch_i, further_branch_i);
		std::swap(a, c);
		std::swap(b, d);
		intersection = cd_intersection;
	}

	// Check if the closer dist is even intersecting at all, if not return, thereby doing nothing.
	if (fmin(ab_dist, cd_dist) > circle.radius)
		return false;

	// std::println("intersection type was {} ab and cd were {} {}", intersection == VERT ? "VERT" : "LINE", ab_intersection == VERT ? "VERT" : "LINE", cd_intersection == VERT ? "VERT" : "LINE");
	const float angle = intersection == VERT
		? vert_angle(a, b, circle)
		: line_angle(a, b, circle);

	// Rotate the closer branch to edge of the circle
	rotate_all(closer_branch_i, angle, tree.branches[closer_branch_i].back(), tree);

	// Seems to rotate in the wrong direction sometimes

	// Rotate the further branch a smaller amount, but still ensuring its in front
	const float closer_to_further_angle = angle_from(d - c, b - a);
	const float angle_from_closer = angle + closer_to_further_angle * 0.8;
	const float further_angle = angle_from_closer - closer_to_further_angle;
	rotate_all(further_branch_i, further_angle, tree.branches[closer_branch_i].back(), tree);
	return true;
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
			const float original_radius = circle.radius;
			circle.radius += radius_buffer;

			const Branch& branch = tree.branches[walk_i];

			// Before we do the next iter, shrink circle radius a little.
			float radius_buffer2 = circle.radius * 0.001;
			for (size_t i = 0; i < branch_depth + 1; i++)
				radius_buffer2 *= 0.5;
			const float interp_radius = original_radius + (radius_buffer2 + radius_buffer) / 2;

			if (branch.nexts.size() == 2) {
				// Let's isolate the problem
				if (debug["rotate_state"] != "None")
					if (flatten_fork(tree, walk_i, circle, interp_radius)) {
						// std::println("circle radius {} depth {}", circle.radius, branch_depth);
					}
			}
			else if (branch.nexts.size() == 1) {
				if (debug["rotate_state"] == "Rotate single branch") {
					const size_t next_i = branch.nexts[0];
					auto [a, b] = to_wireframe(tree, walk_i, next_i);
					float debug_angle;
					if (flatten(tree, walk_i, a, b, circle, &debug_angle)) {
						debug["should_flatten"] = "true";
						// std::cout << "branches to flatten " << walk_i << "\n";
						// std::cout << "branches angle " << debug_angle << "\n";
						if (walk_i == 0)
							std::println("0th iter branch angle is {}", debug_angle);
					}

					circle.radius = interp_radius;
					// This should do the job of flattening the child nodes as well.
					const Branch& next_branch = tree.branches[next_i];
					const size_t next_next_i = next_branch.nexts.size() > 0 
						? next_branch.nexts[0] 
						: 0;
					auto [next_a, next_b] = to_wireframe(tree, next_i, next_next_i);
					if (flatten(tree, next_i, next_a, next_b, circle, &debug_angle)) {
						debug["should_flatten"] = "true";
						if (walk_i == 0)
							std::println("0th iter tip angle is {}", debug_angle);
						// std::cout << "tip angle " << debug_angle << "\n";
						// std::println("circle radius {} depth {}", circle.radius, branch_depth);
					}
				}
			}
			else if (branch.nexts.size() == 0) {
			}
			else
				std::cout << "Unexpectedly we have this many branches: " << branch.nexts.size() << "\n";

			circle.radius = original_radius;
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
		.max_radius = std::sqrt(collision_dist) * 20,
		// .max_radius = (float) (std::sqrt(collision_dist) * 2.5),
		.depth_to_radius_fn = [](float depth) {
			return std::sqrt(depth) * 20;
		}
	};

	debug["rotate_state"] = "None";
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

	const auto debug_rotate_state = debug["rotate_state"];
	debug.clear();
	debug["rotate_state"] = debug_rotate_state;

	if (IsKeyPressed(KEY_R)) {
		debug_apply_rotation = !debug_apply_rotation;

		std::map<std::string, std::string> switch_map {
			{ "Rotate fork", "Rotate single branch" },
			{ "Rotate single branch", "None" },
			{ "None", "Rotate fork" },
		};

		if (switch_map.contains(debug["rotate_state"]))
			debug["rotate_state"] = switch_map[debug["rotate_state"]];
		else
			std::cout << "What is this rotation state " << debug["rotate_state"] << "\n";
	}

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
	DrawText(debug["rotate_state"].c_str(), 30, 125, 45, WHITE);
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