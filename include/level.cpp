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

	const bool rhr_aligned = rhr_sign(a, b, circle.pos) == rhr_sign(a, solns[0], circle.pos);
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
	const float b_angle = angle_from(b - a, circle.pos - a);

	const float sign = -rhr_sign(a, b, circle.pos);
	const float ang = (bnew_angle - b_angle) * sign;
	return ang;
}

// THIS IS ANTI-MATHWISE I THINK?
void rotate_all(const size_t branch_i, const float amt, const Vector2 origin, Tree& tree) {
	if (isnan(amt))
		return;

	Branch& branch = tree.branches[branch_i];

	for (auto& vert : branch.verts) {
		const auto& rotated = rotate(origin, vert, amt);
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
	const float theta = intersection == VERT
		? vert_angle(a, b, circle)
		: line_angle(a, b, circle);

	// Rotate the closer branch to edge of the circle
	rotate_all(closer_branch_i, theta, tree.branches[closer_branch_i].back(), tree);

	// Seems to rotate in the wrong direction sometimes

	// Rotate the further branch a smaller amount, but still ensuring its in front
	// const float bacd_theta = angle_from(d - a, b - a) * -rhr_sign(b, a, d);
	// const float sigmoidal_fraction = 2 * (1 - 0.2) * (1 / (1 + exp(fabs(theta) * 0.4)) - 0.5) + 1;
	// const float new_theta = bacd_theta * sigmoidal_fraction;
	// const float ba_angle = angle(b - a);
	// const float new_dc_angle = ba_angle + new_theta;
	// const float dc_angle = angle(d - c);
	// // Idk why we add a negative sign.
	// const float dc_theta = -(new_dc_angle - dc_angle);
	// // std::println("rhr_sign {}, theta {}, dc_theta {}", rhr_sign(b, a, d), theta, dc_theta);
	// rotate_all(further_branch_i, dc_theta, tree.branches[closer_branch_i].back(), tree);

	return true;
}

void Dome::flatten_tree(Tree& tree, const Circle& circle, std::map<std::string, std::string>& debug) const {
	// Debugging

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

		if (debug["rotate_state"] != "None") {
			if (branch.nexts.size() == 2) {
				// Let's isolate the problem
				if (flatten_fork(tree, walk_i, circle, interp_radius)) {
					// std::println("circle radius {} depth {}", circle.radius, branch_depth);
				}
			}
			else if (branch.nexts.size() == 1) {
				const size_t next_i = branch.nexts[0];
				auto [a, b] = to_wireframe(tree, walk_i, next_i);
				float debug_angle;
				if (flatten(tree, walk_i, a, b, circle, &debug_angle)) {
					debug["should_flatten"] = "true";
				}
			}
			else if (branch.nexts.size() == 0) {
				auto [a, b] = to_wireframe(tree, walk_i, 0);
				float blah;
				flatten(tree, walk_i, a, b, circle, &blah);
			}
			else
				std::cout << "Unexpectedly we have this many branches: " << branch.nexts.size() << "\n";
		}

		circle.radius = original_radius;
		for (const auto& next : branch.nexts)
			walk(next, branch_depth + 1, circle);
	};
	walk(0, 0, circle);

	tree.update_texture();
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
		.max_radius = (float) (std::sqrt(collision_dist) * 12),
		.depth_to_radius_fn = [](float depth) {
			return std::sqrt(depth) * 20;
		}
	};

	debug["rotate_state"] = "None";
	debug["spring"] = "false";
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

static float interpolate_rotation2(const float twist, const Vector2& original_rel_dir, const Vector2& cur_rel_dir, const size_t cur, const Branch& child, Tree& tree) {
	(void) original_rel_dir;
	(void) cur_rel_dir;
	(void) twist;
	(void) child;
	(void) tree;
	(void) cur;
	// const float raw_ang = angle_from(original_rel_dir, cur_rel_dir);

	// const float rollover_twist = fmodf(abs(twist), 2 * PI);

	// if (child.nexts.size() > 0 && child.nexts[0] == 1)
	// 	std::println("rollover twist {}", rollover_twist);

	// const float ang = rollover_twist > PI
	// 	? 2 * PI - raw_ang
	// 	: raw_ang;
	// const float theta = fmin(0.03, 0.1 * ang)
	// 	* rhr_sign(original_rel_dir, { 0, 0 }, cur_rel_dir)
	// 	* (rollover_twist > PI ? -1 : 1);
	const float theta = fmin(0.04, 0.05 * abs(twist)) * (twist < 0 ? -1 : 1);
	// if (abs(theta) > 0.001)
	// 	std::println("theta {}", theta);

	// ROTATE ALL LOOKS ANTI-MATHWISE
	if (abs(theta) > 0.005)
		rotate_all(cur, theta, child.back(), tree);

	return theta;
}

void interpolate_rotation(const bool flip_cur_to_original, const Vector2& original_rel_dir, const Vector2& cur_rel_dir, const size_t cur, const Branch& child, Tree& tree) {
	const float raw_ang = angle_from(original_rel_dir, cur_rel_dir);
	(void) flip_cur_to_original;
	// const float ang = flip_cur_to_original ? 2 * PI - raw_ang : raw_ang;
	// const float theta = fmin(0.015, 0.1 * ang) * 
	// 	rhr_sign(original_rel_dir, { 0, 0 }, cur_rel_dir) *
	// 	(flip_cur_to_original ? -1 : 1);

	const float theta = fmin(0.015, 0.1 * raw_ang) * 
		rhr_sign(original_rel_dir, { 0, 0 }, cur_rel_dir);

	if (abs(theta) > 0.005)
		rotate_all(cur, theta, child.back(), tree);
}

struct BranchOffsetBundle {
	float& twist;
	const Vector2 original_rel_dir;
	const Vector2 cur_rel_dir;
	const size_t cur;
	const Branch& child;
	Tree& tree;
};
static void walk_branch_offsets_helper(std::function<void(BranchOffsetBundle&&)> bundle_fn, Tree& tree, const size_t i) {
	const std::vector<unsigned int>& nexts = tree.branches[i].nexts;

	for (const auto& next : nexts) {
		// The first iterated branch will not rotate, fix that outside of this fn.
		const size_t parent_i = i;
		const size_t child_i = next;
		const auto& parent = tree.branches[parent_i];
		const auto& child = tree.branches[child_i];

		const auto& original_parent = tree.original_branches[parent_i];
		const auto& original_child = tree.original_branches[child_i];

		const auto& parent_to_child = child.back() - parent.back();
		const auto& cur_forward = child.forward();
		const auto& cur_rel_dir = rel_dir(cur_forward, parent_to_child);

		const auto& original_parent_to_child = original_parent.forward(); 
		const auto& original_cur_forward = original_child.forward();
		const auto& original_rel_dir = rel_dir(original_cur_forward, original_parent_to_child);

		// Read from the current twist and edit it
		bundle_fn({
			tree.branch_twists[child_i],
			original_rel_dir,
			cur_rel_dir,
			child_i,
			child,
			tree
		});

		walk_branch_offsets_helper(bundle_fn, tree, next);
	}
}

static void walk_branch_offsets(std::function<void(BranchOffsetBundle&&)> bundle_fn, Tree& tree, const size_t i) {
	if (i == 0) {
		// Do I want to do the 0th iteration here too?
		const auto& first = tree.branches[0];
		bundle_fn({
			tree.branch_twists[0],
			rel_dir(tree.original_branches[0].forward(), { 1, 0 }),
			rel_dir(first.forward(), { 1, 0 }),
			0,
			first,
			tree});
	}
	
	walk_branch_offsets_helper(bundle_fn, tree, i);
}

static void blah(Tree& tree) {
	// std::function<void(size_t)> walk = [&walk, &tree](const size_t i) {
	// 	const std::vector<unsigned int>& nexts = tree.branches[i].nexts;

	// 	for (const auto& next : nexts) {
	// 		// The first iterated branch will not rotate, fix that outside of this fn.
	// 		const size_t parent_i = i;
	// 		const size_t child_i = next;
	// 		const auto& parent = tree.branches[parent_i];
	// 		const auto& child = tree.branches[child_i];

	// 		const auto& original_parent = tree.original_branches[parent_i];
	// 		const auto& original_child = tree.original_branches[child_i];

	// 		const auto& parent_to_child = child.back() - parent.back();
	// 		const auto& cur_forward = child.forward();
	// 		const auto& cur_rel_dir = rel_dir(cur_forward, parent_to_child);

	// 		const auto& original_parent_to_child = original_parent.forward(); 
	// 		const auto& original_cur_forward = original_child.forward();
	// 		const auto& original_rel_dir = rel_dir(original_cur_forward, original_parent_to_child);

	// 		// Read from the current twist and edit it
	// 		interpolate_rotation2(
	// 			tree.branch_twists[child_i], 
	// 			original_rel_dir,
	// 			cur_rel_dir,
	// 			next,
	// 			child,
	// 			tree);

	// 		walk(next);
	// 	}
	// };
	// walk(0);

	// (void) walk;
	auto interpolate_rotation_bundled = [](BranchOffsetBundle&& b) {
		// if (b.cur == 19 || b.cur == 0)
		// 	std::println("cur {} receiving b.twist {}", b.cur, b.twist);
		const float theta = interpolate_rotation2(
			b.twist, 
			b.original_rel_dir,
			b.cur_rel_dir,
			b.cur,
			b.child,
			b.tree);
		if (b.cur == 19 || b.cur == 0) {
			std::println("cur {} b.twist {} theta {}", b.cur, b.twist, theta);

			// const float theta = fmin(0.04, 0.05 * abs(b.twist)) * (b.twist < 0 ? -1 : 1);
			// if (abs(theta) > 0.001)
			// 	std::println("theta {}", theta);
		}
	};
	walk_branch_offsets(interpolate_rotation_bundled, tree, 0);
}

void Level::update(Game& game) {
	petra.update(*this, game.trees);
	dome.pos = petra.pos;

	debug["should_flatten"] = "false";

	if (IsKeyPressed(KEY_R)) {
		debug_apply_rotation = !debug_apply_rotation;

		std::map<std::string, std::string> switch_map {
			// { "Rotate fork", "Rotate single branch" },
			// { "Rotate single branch", "None" },
			{ "Rotation enabled ", "None"},
			{ "None", "Rotate enabled " },
		};

		if (switch_map.contains(debug["rotate_state"]))
			debug["rotate_state"] = switch_map[debug["rotate_state"]];
		else
			std::cout << "What is this rotation state " << debug["rotate_state"] << "\n";
	}

	if (IsKeyPressed(KEY_P)) {
		if (debug["spring"] == "true")
			debug["spring"] = "false";
		else
			debug["spring"] = "true";
	}

	for (const auto& tree_ptr : game.trees) {
		auto& tree = *tree_ptr;
		if (!tree.past_me(petra))
			continue;

		const auto& dist = dist_from_cam(tree);
		if (dist >= collision_dist)
			continue;

		const float radius = std::min(dome.max_radius, dome.depth_to_radius_fn(collision_dist - dist));

		const float dist_tree_dome = length(dome.pos - tree.origin());

		// But we also want to move trees out of the way of the dome as necessary.
		const bool tree_too_close = dist_tree_dome < radius * 1.3;
		if (tree_too_close && debug["spring"] == "true") {
			// TODO: Then we move the tree. 

		}
		else {
			// TODO: Then we move the tree back to the original position.
		}

		if (tree_too_close)
			debug["too_close"] = "true";
		else {
			debug["too_close"] = "";
			// Then we may be too far, it depends on what flatten_tree determines.
			const Circle dome_circle {
				.pos = dome.pos,
				.radius = radius
			};

			if (debug["spring"] == "true") {
				// tree_interp_rigid(tree, dome_circle);

				blah(tree);
				// tree.update_texture();

				dome.flatten_tree(tree, dome_circle, debug);

				std::vector<Vector2> cur_rel_dirs;
				auto walk_fn = [&cur_rel_dirs](BranchOffsetBundle&& b) {
					cur_rel_dirs.push_back(b.cur_rel_dir);
				};
				walk_branch_offsets(walk_fn, tree, 0);
				// I DON'T TRUST THE WALK_BRANCH ABOVE.

				// For twisting purposes, capture diff in twist caused by flatten_tree
				// std::println("size of pre_rel_dirs {} size of tree branches {}", pre_rel_dirs.size(), tree.branches.size());

				// 19 15
				// for (size_t i = 0; i < tree.branches.size(); i++) {
				// 	const Vector2& prev_rel_dir = tree.prev_rel_dirs[i];
				// 	const Vector2& cur_rel_dir = cur_rel_dirs[i];

				// 	// const float ang = angle_from(prev_rel_dir, cur_rel_dir);
				// 	// const float theta = ang * rhr_sign(prev_rel_dir, { 0, 0 }, cur_rel_dir);
				// 	const float theta = signed_angle_from(prev_rel_dir, cur_rel_dir);
				// 	if (i > 9 || i == 0) {
				// 		// std::println("{}'s theta {} twist {}", i, theta, tree.branch_twists[i]);
				// 		std::println("{}'s twist {} cur_rel_dir {}", i, tree.branch_twists[i], to_str(cur_rel_dir, 2));
				// 		// std::println("nexts #: {}", tree.branches[i].nexts.size());
				// 	}

				// 	tree.branch_twists[i] += theta;
				// 	// tree.branch_twists[child_i],
				// }
				size_t nxt = 0;
				auto fn = [&tree, &cur_rel_dirs, &nxt](BranchOffsetBundle&& b) {
					const Vector2& prev_rel_dir = tree.prev_rel_dirs[nxt];
					const Vector2& cur_rel_dir = cur_rel_dirs[nxt++];

					const float theta = signed_angle_from(prev_rel_dir, cur_rel_dir);
					b.twist += theta;
				};
				walk_branch_offsets(fn, tree, 0);

				tree.prev_rel_dirs.clear();
				for (const auto& dir : cur_rel_dirs)
					tree.prev_rel_dirs.push_back(dir);
			}
			else {
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
}

void Level::tree_interp_rigid(Tree& tree, const Circle& circle) {
	std::function<void(size_t)> walk = [&walk, &tree, &circle](const size_t i) {
		(void) circle;
		const std::vector<unsigned int>& nexts = tree.branches[i].nexts;

		for (const auto& next : nexts) {
			// The first iterated branch will not rotate, fix that outside of this fn.
			const size_t parent_i = i;
			const size_t child_i = next;
			const auto& parent = tree.branches[parent_i];
			const auto& child = tree.branches[child_i];

			const auto& original_parent = tree.original_branches[parent_i];
			const auto& original_child = tree.original_branches[child_i];

			const auto& parent_to_child = child.back() - parent.back();
			const auto& cur_forward = child.forward();
			const auto& cur_rel_dir = rel_dir(cur_forward, parent_to_child);

			const auto& original_parent_to_child = original_parent.forward(); 
			const auto& original_cur_forward = original_child.forward();
			const auto& original_rel_dir = rel_dir(original_cur_forward, original_parent_to_child);

			interpolate_rotation(false, original_rel_dir, cur_rel_dir, next, child, tree);

			walk(next);
		}
	};

	walk(0);

	// Okay but now we have to rotate the 0th iteration
	const auto& first = tree.branches[0];
	interpolate_rotation(
		false,
		tree.original_branches[0].forward(),
		first.forward(),
		0,
		first,
		tree);
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
	DrawText(debug["rotate_state"].c_str(), 30, 125, 45, with_alpha(WHITE, 0.2));
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
		const float radius = std::min(dome.max_radius, dome.depth_to_radius_fn(collision_dist - dist));
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