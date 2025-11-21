#include <iostream>
#include <cmath>
#include <random>
#include "tree.hpp"
#include "../main.hpp"

Branch::Branch(std::vector<Vector2> verts) {
    this->verts = verts;
}

const Vector2 Branch::front() const {
    return (verts[0] + verts[1]) / 2;
}

const Vector2 Branch::back() const {
    return (verts[2] + verts[3]) / 2;
}

const Vector2 Branch::forward() const { 
    return front() - back();
}

const float Branch::front_thickness() const {
    return my_length(verts[0] - verts[1]) / 2;
}

const float Branch::back_thickness() const {
    return my_length(verts[2] - verts[3]) / 2;
}

const Branch Branch::clone() const {
    std::vector<Vector2> vs {};
    for (auto vert : verts) {
        vs.push_back(vert);
    }
    return Branch(vs);
}

Tree::Tree(std::vector<Branch> branches, Shader shader, Rand& rand) : rand(rand) {
    this->branches = branches;
    this->shader = shader;

    this->tendrils = {};
	tree_tex = LoadTexture("include/assets/tree_texture.png");
}

Tree::~Tree() {
    unload_textures();
}

void Tree::unload_textures() {
    UnloadTexture(blank_tex);
    UnloadTexture(tree_tex);
}

// Will be out of date if branch verts are changed.
void Tree::init_texture() {
    unload_textures();

    // Bounding box it
    Vector2 small { 9999, 9999 };
    Vector2 big { -9999, -9999 };
    for (const auto& branch : branches) {
        for (int i = 0; i < 4; i++) {
            small.x = fmin(small.x, branch.verts[i].x);
            small.y = fmin(small.y, branch.verts[i].y);
            big.x = fmax(big.x, branch.verts[i].x);
            big.y = fmax(big.y, branch.verts[i].y);
        }
    }
    std::cout << "small " << small.x << ", " << small.y << "\n";
    std::cout << "big " << big.x << ", " << big.y << "\n";

    auto blank = GenImageColor(int(big.x - small.x), int(big.y - small.y), BLANK);
    blank_tex = LoadTextureFromImage(blank);
    texture_pos = Vector2I(small);

	tree_tex = LoadTexture("include/assets/tree_texture.png");
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

void Tree::render() {
    int color_loc = GetShaderLocation(shader, "color");
    Vector4 white { 1.0, 1.0, 1.0, 1.0 };

    // Create a texture for this branch.
    SetShaderValue(shader, color_loc, &white, SHADER_UNIFORM_VEC4);

    int n_loc = GetShaderLocation(shader, "N");
    int size = branches.size();

    SetShaderValue(shader, n_loc, &size, SHADER_UNIFORM_INT);

    // std::cout << "bounds " << to_str(Vector2I(tex.width, tex.height).to_vec2(), 2) << "\n";
    // std::cout << "tex_pos " << to_str(texture_pos.to_vec2(), 4) << "\n";
    for (int n_i = 0; n_i < size; n_i++) {
        const auto& branch = branches[n_i];
        // Set the really big vertices array of the shader that doesn't exist yet
        for (int branch_i = 0; branch_i < 4; branch_i++) {
            auto pt = branch.verts[branch_i];
            auto tex_size = Vector2I(blank_tex.width, blank_tex.height).to_vec2();
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
            const float left_bound = snap(render_rand.gen(0, 1.0 - branch_width), tree_tex.width);
            float btm_height = 0;

            for (const auto& branch : subtendril) {
                // smaller = less pixels
                // Force height to be such that we get a square
                const float height = fmodf(my_length(branch.forward()) / MAX_HEIGHT, 1.0);

                // Might be out of bounds of (1,1), in which case wrap it.
                btm_lefts[branch_i] = Vector2 { left_bound, btm_height };
                btm_height = fmodf(btm_height + height, 1.0);
                top_rights[branch_i] = Vector2 { left_bound + branch_width, btm_height };

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

    BeginShaderMode(shader);
    DrawTexture(blank_tex, texture_pos.x, texture_pos.y, WHITE);
    EndShaderMode();
}

std::vector<std::vector<Branch>> Tree::random_tendril_config(float total_length, float start_thickness, float start_rotation, float thickness_cutoff, Vector2 start_location, int MAX_TENDRILS = 5) {
    start_thickness = snap(start_thickness, tree_tex.width / MAX_WIDTH);
    std::uniform_real_distribution<> uniform_gen(0.0, 1.0);
    float length_used = 0;

    const auto& length_calc = [this, &total_length, &length_used](std::vector<Branch> subtendril) -> float {
        // for now just go with it being independent of tendril.
        float rand_length = rand.gen(total_length * 0.04, total_length * 0.13);
        float ret = fmin(total_length - length_used, rand_length);
        return snap(ret, tree_tex.width / MAX_WIDTH);
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

    const auto& make_branch = [&start_thickness](Vector2 start, float rotation, float length, float front_thickness, float back_thickness) -> Branch {
        const Vector2 mid_front = start + unit_vector(rotation) * length;
        const Vector2 perp_rotation = perp_rhr(unit_vector(rotation));
        Vector2 p1 = mid_front + perp_rotation * front_thickness;
        Vector2 p2 = mid_front - perp_rotation * front_thickness;
        Vector2 p3 = start - perp_rotation * back_thickness;
        Vector2 p4 = start + perp_rotation * back_thickness;
        return Branch({ p1, p2, p3, p4 });
    };

    const auto& make_branch_from = [&make_branch, &length_used, &length_calc, &angle_calc, &thickness_calc](std::vector<Branch> tendril, Branch branch) -> Branch {
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
            } else {
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

        const int random_index = int(rand.gen(0, splittable_branches.size()));

        const auto& random_branch = splittable_branches[random_index];

        curr_tendril = { random_branch };
        splittable_branches.erase(splittable_branches.begin() + random_index);
    }

    return tendrils;
}