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
	return length(verts[0] - verts[1]) / 2;
}

float Branch::back_thickness() const {
	return length(verts[2] - verts[3]) / 2;
}

Branch Branch::clone() const {
	std::vector<Vector2> vs;
	for (auto vert : verts)
		vs.push_back(vert);
	return { vs };
}

void Tree::init(std::vector<Branch> branches, ShaderWithCheck tendril_shader, ShaderWithCheck trunk_shader, Rand& rand) {
	this->branches = branches;
	this->tendril_shader = tendril_shader;
	this->trunk_shader = trunk_shader;
	this->rand = rand;

	this->tendrils = {};
	tree_tex = static_tree_tex;
	// This resolution actually matters. Lower looks ps1-like. It seems like 1000x1000 (1000, 1000) is practically perfect, but too slow to render more than like 5 branches at.
	target = LoadRenderTexture(400, 400);
	init_texture();
}

Tree::Tree(std::vector<Branch> branches, Rand& rand) : rand(rand) {
	id = 0;
	std::cout << "init tree\n";
	ShaderWithCheck tendril_shader;
	load_shader(tendril_shader, "assets/tree_tendril.fs");
	ShaderWithCheck trunk_shader;
	load_shader(trunk_shader, "assets/tree_trunk.fs");

	init(branches, tendril_shader, trunk_shader, rand);
}

Tree::~Tree() {
	std::cout << "deinit tree\n";
	unload_textures();
	std::cout << "unload tendril shader!\n";
	unload_shader(tendril_shader);
	std::cout << "unload trunk shader!\n";
	unload_shader(trunk_shader);
	std::cout << "tendril shader w/ id " << tendril_shader.id << " loads/unloads " << tendril_shader.load_unloads << "\n";
	std::cout << "trunk shader w/ id " << tendril_shader.id << " loads/unloads " << tendril_shader.load_unloads << "\n";

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

	int loc = GetShaderLocation(tendril_shader, "tex");
	SetShaderValueTexture(tendril_shader, loc, tree_tex);
}

// Needed to reposition texture.
void Tree::update_texture() {
	Vector2 _;
	bounding_box(texture_pos, _);
}

void Tree::send_vals_to_tendril_shader() {
	int color_loc = GetShaderLocation(tendril_shader, "finishing_color");
	Vector4 color { 1, 1, 1, 1 };
	// Create a color for this branch.
	SetShaderValue(tendril_shader, color_loc, &color, SHADER_UNIFORM_VEC4);

	int loc = GetShaderLocation(tendril_shader, "N");
	size_t size = branches.size();
	SetShaderValue(tendril_shader, loc, &size, SHADER_UNIFORM_INT);

	int dims_loc = GetShaderLocation(tendril_shader, "dims");
	Vector2I dims { tree_tex.width, tree_tex.height };
	SetShaderValue(tendril_shader, dims_loc, &dims, SHADER_UNIFORM_IVEC2);

	// TODO
	// HMM, BAD TO RECOMPUTE THESE ALL THE TIME IF WE USE IT FOR NORMALIZATION IN THE FOR LOOP.
	bounding_box(small, big);

	for (size_t n_i = 0; n_i < size; n_i++) {
		const auto& branch = branches[n_i];
		// Set the really big vertices array of the shader that doesn't exist yet
		for (size_t branch_i = 0; branch_i < 4; branch_i++) {
			auto pt = branch.verts[branch_i];
			// auto tex_size = Vector2I(big - small).to_vec2();

			// Vector2 norm = (pt - texture_pos.to_vec2()) / tex_size;
			// Vector2 clamped_pt { roundf(pt.x), roundf(pt.y) };
			// Vector2 norm = (clamped_pt - texture_pos.to_vec2()) / tex_size;

			// I'm sus of these options, but it looks like all of them are bad.
			Vector2 norm = (pt - small) / (big - small);
			compressed_branches[branch_i][n_i] = norm;
		}
	}

	int branch_i = 0;
	// Given: branches is the flattened version of tendrils.
	Rand render_rand(rand.seed);

	const float branch_width = fmodf((tendrils[0][0].back_thickness() * 2) / MAX_WIDTH, 1.0);
	// But the texture is at this width, so branch_width should be a multiple of that

	for (const auto& tendril : tendrils) {
		// Start from the bottom of the texture, work your way up
		// x_small and x_big chosen from start_thickness, perhaps
		const float left_bound = snap(render_rand.gen(0, 1.0 - branch_width), (float) tree_tex.width);
		float btm_height = 0;

		for (const auto& branch : tendril) {
			// smaller = less pixels
			// Force height to be such that we get a square
			const float height = fmodf(length(branch.forward()) / MAX_HEIGHT, 1.0);

			// Might be out of bounds of (1,1), in which case wrap it.
			btm_lefts[branch_i] = { left_bound, btm_height };
			btm_height = fmodf(btm_height + height, 1.0);
			top_rights[branch_i] = { left_bound + branch_width, btm_height };

			branch_i++;
		}
	}

	// texture regions
	int btm_left_locs = GetShaderLocation(tendril_shader, "btmLefts");
	int top_right_locs = GetShaderLocation(tendril_shader, "topRights");

	if (IsKeyPressed(KEY_U)) {
		std::cout << "btm lefts:\n";
		for (const auto& v : btm_lefts) {
			std::cout << to_str(v, 2) << " ";
		}
		std::cout << "\n";
		std::cout << "top rights:\n";
		for (const auto& v : top_rights) {
			std::cout << to_str(v, 2) << " ";
		}
		std::cout << "\n";
	}
	
	SetShaderValueV(tendril_shader, btm_left_locs, btm_lefts, SHADER_UNIFORM_VEC2, size);
	SetShaderValueV(tendril_shader, top_right_locs, top_rights, SHADER_UNIFORM_VEC2, size);

	SetShaderValueV(tendril_shader, GetShaderLocation(tendril_shader, "pt1s"), compressed_branches[0], SHADER_UNIFORM_VEC2, size);
	SetShaderValueV(tendril_shader, GetShaderLocation(tendril_shader, "pt2s"), compressed_branches[1], SHADER_UNIFORM_VEC2, size);
	SetShaderValueV(tendril_shader, GetShaderLocation(tendril_shader, "pt3s"), compressed_branches[2], SHADER_UNIFORM_VEC2, size);
	SetShaderValueV(tendril_shader, GetShaderLocation(tendril_shader, "pt4s"), compressed_branches[3], SHADER_UNIFORM_VEC2, size);
}

// ONLY FOR LEVEL EDITOR
void Tree::render(LevelEditor* level_editor) {
	send_vals_to_tendril_shader();
	int finishing_alpha_loc = GetShaderLocation(tendril_shader, "finishing_alpha");
	// TODO: make tendrils that are far away clearer, which will require an ability to scroll the depth we are viewing at.
	float finishing_alpha = std::max(0.0, 1.0 - depth / (level_editor->depth_ui.MAX_DEPTH * 0.8));
	SetShaderValue(tendril_shader, finishing_alpha_loc, &finishing_alpha, SHADER_UNIFORM_FLOAT);

	// Later TODO: Make a custom shader for level editor (which is here) cause showing a high level repr of each segment is very different.

	BeginShaderMode(tendril_shader);
	DrawTexturePro(
		blank_tex,
		// source rect
		full_texture(blank_tex),
		// dest rect, I think its the whole screen
		{
			.x = texture_pos.x,
			.y = texture_pos.y,
			.width = (big - small).x,
			.height = (big - small).y,
		},
		{},
		0,
		WHITE);
	EndShaderMode();

	// Suppose we only render the top
	if (trunk_segments.size() > 0) {
		auto trunk_layer = trunk_segments[0].top;
		BeginShaderMode(trunk_shader);
		DrawTexturePro(
			blank_tex,
			full_texture(blank_tex),
			{
				.x = trunk_layer.position.x,
				.y = trunk_layer.position.y,
				.width = trunk_layer.radius * 2,
				.height = trunk_layer.radius * 2
			},
			{},
			0,
			WHITE);
		EndShaderMode();
	}
}

void Tree::render_to_target() {
	BeginTextureMode(target);
	ClearBackground(BLANK);

	send_vals_to_tendril_shader();
	auto src = full_texture(blank_tex);
	auto dest = full_texture(target.texture);
	src.height *= -1;

	BeginShaderMode(tendril_shader);
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

constexpr Vector2 Tree::origin() const {
	return branches[0].back();
}

std::vector<Branch> Tree::branches_from_tendrils(std::vector<std::vector<Branch>> tendrils) {
	std::vector<Branch> branches;

	for (const auto& tendril : tendrils) {
		for (const auto& branch : tendril) {
			branches.push_back(branch);
		}
	}

	return branches;
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
			if (angle_from(tendril_direction, aim_v) > 1.0) {
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
		float last_thickness = length(last_branch.verts[0] - last_branch.verts[1]) / 2;
		return end_norm * rand.gen(0.5 * last_thickness, 1.2 * last_thickness);
	};

	const auto& make_branch = [](Vector2 start, float rotation, float length, float front_thickness, float back_thickness) -> Branch {
		const Vector2 mid_front = start + unit_vector(rotation) * length;
		const Vector2 perp_rotation = perp_rhr(unit_vector(rotation));
		Vector2 p1 = mid_front - perp_rotation * front_thickness;
		Vector2 p2 = mid_front + perp_rotation * front_thickness;
		Vector2 p3 = start + perp_rotation * back_thickness;
		Vector2 p4 = start - perp_rotation * back_thickness;
		return {{ p1, p2, p3, p4 }};
	};

	const float SPAWN_END_RATIO = 0.9;

	const auto& make_branch_from = [&make_branch, &length_calc, &angle_calc, &thickness_calc, &SPAWN_END_RATIO](std::vector<Branch> tendril, Branch branch) -> Branch {
		const Vector2 forward = branch.forward();
		const float forward_angle = angle(forward);

		const float length = length_calc(tendril);
		const float new_angle = angle_calc(angle(tendril[0].forward()), tendril) + forward_angle;
		const float new_thickness = thickness_calc(tendril);

		const Vector2 back = branch.back();
		const Vector2 start = forward * SPAWN_END_RATIO + back;
		const float thickness = branch.front_thickness();

		return make_branch(start, new_angle, length, new_thickness, thickness);
	};

	size_t next_index = 1;

	auto start_branch = make_branch(start_location, start_rotation, total_length * 0.2, start_thickness * 0.8, start_thickness);

	std::vector<std::vector<Branch>> tendrils;
	// In a for loop, allocate tendril vectors
	std::vector<Branch> curr_tendril { start_branch, start_branch };

	// std::vector<Branch> splittable_branches;
	struct SplitIdx {
		int i, j;
	};
	std::vector<SplitIdx> splittable_indices;
	SplitIdx indices;

	while ((int) tendrils.size() < MAX_TENDRILS) {
		length_used = 0;
		
		// Below is the process of building out a tendril.
		while (length_used / total_length < 0.99) {
			auto& branch = curr_tendril.back();
			auto new_branch = make_branch_from(curr_tendril, branch);
			const auto new_length = length(new_branch.front() - new_branch.back());
			length_used += new_length;

			const float branch_thickness = new_branch.front_thickness();

			// Let's adjust nexts.
			if (curr_tendril.size() == 1)
				tendrils[indices.i][indices.j].nexts.push_back(next_index++);
			else
				branch.nexts.push_back(next_index++);

			// Stops the tendril if its too thin.
			if (branch_thickness / start_thickness < thickness_cutoff) {
				// Then treat it like an ending branch, forcing the thickness to be small.
				const Vector2 forward = branch.forward();
				const float forward_angle = angle(forward);

				length_used -= new_length;
				const float length = length_calc(curr_tendril);
				length_used += length;

				const float new_angle = angle_calc(angle(curr_tendril[0].forward()), curr_tendril) + forward_angle;

				const Vector2 back = branch.back();
				const Vector2 start = forward * SPAWN_END_RATIO + back;
				const float thickness = branch.front_thickness();
				const float new_thickness = 0.1 * thickness;

				auto end_branch = make_branch(start, new_angle, length, new_thickness, thickness);
				curr_tendril.push_back(end_branch);

				length_used = total_length;
			}
			else
				curr_tendril.push_back(new_branch);
		}
		// Base building a tendril off of a branch as above, but don't repeat branches across tendrils.
		curr_tendril.erase(curr_tendril.begin());

		tendrils.push_back(curr_tendril);

		for (int i = 0; i < (int) curr_tendril.size() - 2; i++)
			splittable_indices.push_back({ (int) tendrils.size() - 1, i });
		if (splittable_indices.size() == 0)
			break;

		const int random_index = (int) rand.gen(0, (float) splittable_indices.size());

		indices = splittable_indices[random_index];
		const auto& random_branch = tendrils[indices.i][indices.j];

		// We want to capture this data conceptually, though this index value is kind of weird.
		// Specifically, we care about which branch this random_branch "split off from". In actuality, it is that a tendril is shared between two branches.
		// We want to be able to ask for every pair of "split" split branches. Then, we also want to describe each of these pairs by their index from the start. However, branches randomly select where they branch off from, meaning the "earlyness" of branches is completely unguaranteed here. Therefore, we need to sort them in some way.
		// Also, looking into the future, when we want to rotate all branches after a particular branch, we need to easily grab all branchs after an index. Remember this.

		// Where does this leave us?
		// We want construct a collection of branch indexes/pointers with more structure than an array, 
		// because we need to determine the "earlyness" of branches. This algorithm has to be at least in part constructed in this algorithm, and it turns out that branches that are "shared" by splittable branches are actually duplicated.

		// Ideas:
		// Suppose we return an dictionary of {split_branch_id: {branch_ids_2[...], branch_ids_2[...]}, ...}
		// This lets us enumerate the different split branches, and go down the two branches that correspond to that branch.
		// However, we need to figure out the split "earlyness", and I am going to sketch that out to figure something out.

		curr_tendril = { random_branch };
		splittable_indices.erase(splittable_indices.begin() + random_index);
	}

	return tendrils;
}

bool Tree::past_me(const Petra& petra, const float epsilon) const {
	return petra.depth <= depth + epsilon;
}