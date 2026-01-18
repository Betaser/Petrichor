#include <iostream>
#include <cmath>
#include <random>

#include "tree.hpp"
#include "../main.hpp"

Branch::Branch(std::vector<Vector2> verts) {
	this->verts = verts;
}

Vector2 Branch::front() const {
	return (verts[0] + verts[1]) / 2;
}

Vector2 Branch::back() const {
	return (verts[2] + verts[3]) / 2;
}

Vector2 Branch::forward() const { 
	return front() - back();
}

float Branch::front_thickness() const {
	return my_length(verts[0] - verts[1]) / 2;
}

float Branch::back_thickness() const {
	return my_length(verts[2] - verts[3]) / 2;
}

Branch Branch::clone() const {
	std::vector<Vector2> vs;
	for (auto vert : verts)
		vs.push_back(vert);
	return { vs };
}

void Tree::init(std::vector<Branch> branches, ShaderWithCheck shader, Rand& rand) {
	this->branches = branches;
	this->shader = shader;
	this->rand = rand;

	this->tendrils = {};
	tree_tex = static_tree_tex;
	// This resolution actually matters. It seems like 1000x1000 is practically perfect
	target = LoadRenderTexture(1000, 1000);
	init_texture();
}

Tree::Tree(std::vector<Branch> branches, ShaderWithCheck shader, Rand& rand) : rand(rand) {
	id = 0;
	std::cout << "init tree w/ args\n";
	init(branches, shader, rand);
}

Tree::~Tree() {
	std::cout << "deinit tree\n";
	unload_textures();
	std::cout << "unload shader!\n";
	unload_shader(shader);
	std::cout << "tree shader w/ id " << shader.id << " loads/unloads " << shader.load_unloads << "\n";
	std::cout << "tree blank tex w/ id " << blank_tex.id << " loads/unloads " << blank_tex.load_unloads << "\n";
	UnloadRenderTexture(target);
}

void Tree::unload_textures() {
	std::cout << "unload texs\n";
	unload_texture(blank_tex);
}

void Tree::bounding_box(Vector2& small, Vector2& big) {
	small.x = small.y = 9999;
	big.x = big.y = -9999;
	for (const auto& branch : branches) {
		for (size_t i = 0; i < 4; i++) {
			small.x = fmin(small.x, branch.verts[i].x);
			small.y = fmin(small.y, branch.verts[i].y);
			big.x = fmax(big.x, branch.verts[i].x);
			big.y = fmax(big.y, branch.verts[i].y);
		}
	}
}

// Will be out of date if branch verts are changed.
void Tree::init_texture() {
	std::cout << "init tree texture\n";
	// unload_textures();

	// Bounding box it
	update_texture();

	auto blank = GenImageColor(blank_tex_dims.x, blank_tex_dims.y, BLANK);
	load_texture_from_image(blank_tex, blank);
	UnloadImage(blank);

	int loc = GetShaderLocation(shader, "tex");
	SetShaderValueTexture(shader, loc, tree_tex);
}

std::vector<Branch> Tree::branches_from_tendrils(Tendrils tendrils) {
	std::vector<Branch> branches;

	for (const auto& tendril : tendrils) {
		for (const auto& subtendril : tendril) {
			for (const auto& branch : subtendril) {
				branches.push_back(branch);
			}
		}
	}

	return branches;
}

// Needed to reposition texture.
void Tree::update_texture() {
	Vector2 pos, _;
	bounding_box(pos, _);
	texture_pos = Vector2I(pos);
}

void Tree::send_vals_to_shader() {
	int color_loc = GetShaderLocation(shader, "color");
	Vector4 white { 1.0, 1.0, 1.0, 1.0 };
	// Create a color for this branch.
	SetShaderValue(shader, color_loc, &white, SHADER_UNIFORM_VEC4);

	int loc = GetShaderLocation(shader, "N");
	size_t size = branches.size();
	SetShaderValue(shader, loc, &size, SHADER_UNIFORM_INT);

	int dims_loc = GetShaderLocation(shader, "dims");
	Vector2I dims { tree_tex.width, tree_tex.height };
	SetShaderValue(shader, dims_loc, &dims, SHADER_UNIFORM_IVEC2);

	bounding_box(small, big);

	for (size_t n_i = 0; n_i < size; n_i++) {
		const auto& branch = branches[n_i];
		// Set the really big vertices array of the shader that doesn't exist yet
		for (size_t branch_i = 0; branch_i < 4; branch_i++) {
			auto pt = branch.verts[branch_i];
			auto tex_size = Vector2I(big - small).to_vec2();
			Vector2 norm = (pt - texture_pos.to_vec2()) / tex_size;
			compressed_branches[branch_i][n_i] = norm;
			// std::cout << "pt " << to_str(pt, 4) << " norm " << to_str(norm, 4) << "\n";
		}
	}

	int branch_i = 0;
	// Given: branches is the flattened version of tendrils.
	Rand render_rand(rand.seed);

	for (const auto& tendril : tendrils) {
		const float branch_width = fmodf((tendril[0][0].back_thickness() * 2) / MAX_WIDTH, 1.0);
		// But the texture is at this width, so branch_width should be a multiple of that

		for (const auto& subtendril : tendril) {
			// Start from the bottom of the texture, work your way up
			// x_small and x_big chosen from start_thickness, perhaps
			const float left_bound = snap(render_rand.gen(0, 1.0 - branch_width), (float) tree_tex.width);
			float btm_height = 0;

			for (const auto& branch : subtendril) {
				// smaller = less pixels
				// Force height to be such that we get a square
				const float height = fmodf(my_length(branch.forward()) / MAX_HEIGHT, 1.0);

				// Might be out of bounds of (1,1), in which case wrap it.
				btm_lefts[branch_i] = { left_bound, btm_height };
				btm_height = fmodf(btm_height + height, 1.0);
				top_rights[branch_i] = { left_bound + branch_width, btm_height };

				branch_i++;
			}
		}
	}

	// texture regions
	int btm_left_locs = GetShaderLocation(shader, "btmLefts");
	int top_right_locs = GetShaderLocation(shader, "topRights");
	
	SetShaderValueV(shader, btm_left_locs, btm_lefts, SHADER_UNIFORM_VEC2, size);
	SetShaderValueV(shader, top_right_locs, top_rights, SHADER_UNIFORM_VEC2, size);

	SetShaderValueV(shader, GetShaderLocation(shader, "pt1s"), compressed_branches[0], SHADER_UNIFORM_VEC2, size);
	SetShaderValueV(shader, GetShaderLocation(shader, "pt2s"), compressed_branches[1], SHADER_UNIFORM_VEC2, size);
	SetShaderValueV(shader, GetShaderLocation(shader, "pt3s"), compressed_branches[2], SHADER_UNIFORM_VEC2, size);
	SetShaderValueV(shader, GetShaderLocation(shader, "pt4s"), compressed_branches[3], SHADER_UNIFORM_VEC2, size);
}

void Tree::render() {
	send_vals_to_shader();
	BeginShaderMode(shader);
	// DrawTexture(blank_tex, texture_pos.x, texture_pos.y, WHITE);
	DrawTexturePro(
		blank_tex,
		// source rect
		{ 
			.x = 0, 
			.y = 0,
			.width = (float) blank_tex.width,
			.height = (float) blank_tex.height,
		},
		// dest rect, I think its the whole screen
		{
			.x = (float) texture_pos.x,
			.y = (float) texture_pos.y,
			.width = (big - small).x,
			.height = (big - small).y,
		},
		{},
		0,
		WHITE);
	EndShaderMode();
}

void Tree::render_to_target() {
	BeginTextureMode(target);
	ClearBackground(BLANK);

	send_vals_to_shader();
	auto src = full_texture(blank_tex);
	auto dest = full_texture(target.texture);
	src.height *= -1;
	BeginShaderMode(shader);
	DrawTexturePro(
		blank_tex,
		src,
		dest,
		{},
		0,
		WHITE);
	EndShaderMode();
	EndTextureMode();
}

std::vector<std::vector<Branch>> Tree::random_tendril_config(float total_length, float start_thickness, float start_rotation, float thickness_cutoff, Vector2 start_location, int MAX_TENDRILS) {
	start_thickness = snap(start_thickness, (float) tree_tex.width / MAX_WIDTH);
	std::uniform_real_distribution<> uniform_gen(0.0, 1.0);
	float length_used = 0;

	const auto& length_calc = [this, &total_length, &length_used](std::vector<Branch> subtendril) -> float {
		(void) subtendril;
		// for now just go with it being independent of tendril.
		float rand_length = rand.gen(total_length * 0.04, total_length * 0.13);
		float ret = fmin(total_length - length_used, rand_length);
		return snap(ret, (float) tree_tex.width / MAX_WIDTH);
	};

	// Redo so that we follow a straight line given by another parameter; which will be determined by analyizing all tendrils and pathing towards a location that spreads out best.
	const auto& angle_calc = [this, &length_used, &total_length](float aim, std::vector<Branch> subtendril) -> float {
		// We gotta make sure the tree angles its branches kinda in a straight line.
		if (subtendril.size() > 1) {
			float sign = rand.gen(0.0, 1.0) < 0.5 ? -1 : 1;
			float end_norm = 1.0 - length_used / total_length * 0.3;
			float radian_offset = end_norm * rand.gen(0.2, 0.6) * sign;
			
			Vector2 tendril_direction = subtendril.back().front() - subtendril[0].front();
			Vector2 aim_v = unit_vector(aim);
			if (my_angle_from(tendril_direction, aim_v) > 1.0) {
				// Then make sure the next radian_offset is in the right direction
				float direction_sign = direction_to_rotate(aim_v, tendril_direction);
				radian_offset = rand.gen(0.50, 0.9) * direction_sign;
			}
		
			return radian_offset;
		} 
		// First branch case:
		return rand.gen(0.5, 0.9) * rand.gen(0.0, 1.0) < 0.5 ? 1 : -1;
	};

	const auto& thickness_calc = [this, &length_used, &total_length](std::vector<Branch> subtendril) -> float {
		// for now just randomize it but taper to MIN based on length_used
		float end_norm = 0.95 - length_used / total_length;
		const auto& last_branch = subtendril.back();
		float last_thickness = my_length(last_branch.verts[0] - last_branch.verts[1]) / 2;
		return end_norm * rand.gen(0.5 * last_thickness, 1.2 * last_thickness);
	};

	const auto& make_branch = [](Vector2 start, float rotation, float length, float front_thickness, float back_thickness) -> Branch {
		const Vector2 mid_front = start + unit_vector(rotation) * length;
		const Vector2 perp_rotation = perp_rhr(unit_vector(rotation));
		// Trying to swap to ccw orientation.
		/*
		Vector2 p1 = mid_front + perp_rotation * front_thickness;
		Vector2 p2 = mid_front - perp_rotation * front_thickness;
		Vector2 p3 = start - perp_rotation * back_thickness;
		Vector2 p4 = start + perp_rotation * back_thickness;
		*/
		Vector2 p1 = mid_front - perp_rotation * front_thickness;
		Vector2 p2 = mid_front + perp_rotation * front_thickness;
		Vector2 p3 = start + perp_rotation * back_thickness;
		Vector2 p4 = start - perp_rotation * back_thickness;
		return {{ p1, p2, p3, p4 }};
	};

	const auto& make_branch_from = [&make_branch, &length_calc, &angle_calc, &thickness_calc](std::vector<Branch> tendril, Branch branch) -> Branch {
		const Vector2 forward = branch.forward();
		const float angle = my_angle(forward);

		const float length = length_calc(tendril);
		const float new_angle = angle_calc(my_angle(tendril[0].forward()), tendril) + angle;
		const float new_thickness = thickness_calc(tendril);

		const Vector2 back = branch.back();
		const Vector2 start = forward * 0.9 + back;
		const float thickness = branch.front_thickness();

		// Not you
		return make_branch(start, new_angle, length, new_thickness, thickness);
	};

	// Starting length = ???
	auto start_branch = make_branch(start_location, start_rotation, total_length * 0.2, start_thickness * 0.8, start_thickness);

	std::vector<std::vector<Branch>> tendrils;
	// In a for loop, allocate tendril vectors
	std::vector<Branch> curr_tendril { start_branch, start_branch };

	std::vector<Branch> splittable_branches;

	while ((int) tendrils.size() < MAX_TENDRILS) {
		length_used = 0;
		
		// Below is the process of building out a tendril.
		while (length_used / total_length < 0.99) {
			const auto branch = curr_tendril.back();
			auto new_branch = make_branch_from(curr_tendril, branch);
			const auto new_length = my_length(new_branch.front() - new_branch.back());
			length_used += new_length;

			const float branch_thickness = new_branch.front_thickness();

			// Stops the tendril if its too thin.
			if (branch_thickness / start_thickness < thickness_cutoff) {
				// Then treat it like an ending branch, forcing the thickness to be small.
				const Vector2 forward = branch.forward();
				const float angle = my_angle(forward);

				length_used -= new_length;
				const float length = length_calc(curr_tendril);
				length_used += length;

				const float new_angle = angle_calc(my_angle(curr_tendril[0].forward()), curr_tendril) + angle;

				const Vector2 back = branch.back();
				const Vector2 start = forward * 0.9 + back;
				const float thickness = branch.front_thickness();
				const float new_thickness = 0.1 * thickness;

				auto end_branch = make_branch(start, new_angle, length, new_thickness, thickness);
				curr_tendril.push_back(end_branch);
				length_used = total_length;
			}
			else {
				curr_tendril.push_back(new_branch);
			}
		}
		// Base a building tendril off of a branch as above, but don't repeat branches across tendrils.
		curr_tendril.erase(curr_tendril.begin());

		tendrils.push_back(curr_tendril);

		for (int i = 0; i < (int) curr_tendril.size() - 2; i++) {
			const auto& branch = curr_tendril[i];
			splittable_branches.push_back(branch);
		}
		if (splittable_branches.size() == 0)
			break;

		const int random_index = (int) rand.gen(0, (float) splittable_branches.size());

		const auto& random_branch = splittable_branches[random_index];

		curr_tendril = { random_branch };
		splittable_branches.erase(splittable_branches.begin() + random_index);
	}

	return tendrils;
}