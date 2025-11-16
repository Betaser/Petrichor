#include <iostream>
#include <cmath>
#include <random>
#include "tree.hpp"

Tree::Tree(std::vector<Branch> branches, Shader shader) {
    this->branches = branches;
    for (int j = 0; j < (int) branches.size(); j++) {
        for (int i = 0; i < 4; i++) {
            this->branches[j].verts[i] = branches[j].verts[i] * 30 + Vector2{400, 300};
        }
    }
    this->shader = shader;

    this->tendrils = {};
}

// Will be out of date if branch verts are changed.
void Tree::init_texture() {
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

    Image blank = GenImageColor(int(big.x - small.x), int(big.y - small.y), BLANK);
    tex = LoadTextureFromImage(blank);
    texture_pos = Vector2I(small);
}

void Tree::make_branches_tendrils() {
    branches.clear();
    for (const auto& tendril : tendrils) {
        for (const auto& subtendril : tendril) {
            for (const auto& branch : subtendril) {
                branches.push_back(branch);
            }
        }
    }
}

void Tree::render() {
    // We'll change the shader later.
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
            auto tex_size = Vector2I(tex.width, tex.height).to_vec2();
            Vector2 norm = (pt - texture_pos.to_vec2()) / tex_size;
            compressed_branches[branch_i][n_i] = norm;
            // std::cout << "pt " << to_str(pt, 4) << " norm " << to_str(norm, 4) << "\n";
        }
    }

    SetShaderValueV(shader, GetShaderLocation(shader, "pt1s"), compressed_branches[0], SHADER_UNIFORM_VEC2, size);

    SetShaderValueV(shader, GetShaderLocation(shader, "pt2s"), compressed_branches[1], SHADER_UNIFORM_VEC2, size);
    
    SetShaderValueV(shader, GetShaderLocation(shader, "pt3s"), compressed_branches[2], SHADER_UNIFORM_VEC2, size);

    SetShaderValueV(shader, GetShaderLocation(shader, "pt4s"), compressed_branches[3], SHADER_UNIFORM_VEC2, size);

    BeginShaderMode(shader);
    DrawTexture(tex, texture_pos.x, texture_pos.y, WHITE);
    EndShaderMode();
}

Branch::Branch(std::vector<Vector2> verts) {
    this->verts = verts;
}

std::vector<std::vector<Branch>> Tree::random_tendril_config(int seed, float total_length, float start_thickness, float start_rotation, float split_chance) {
    std::mt19937 int_gen(seed);
    std::uniform_real_distribution<> uniform_gen(0.0, 1.0);
    const auto& seed_rand = [&int_gen, &uniform_gen](float a, float b) -> float {
        float norm = uniform_gen(int_gen);
        return norm * (b - a) + a;
    };

    float length_used = 0;

    const auto& length_calc = [&total_length, &length_used, &seed_rand](std::vector<Branch> subtendril) -> float {
        // for now just go with it being independent of tendril.
        float rand_length = seed_rand(total_length * 0.2, total_length * 0.3);
        return fmin(total_length - length_used, rand_length);
    };

    const auto& angle_calc = [&length_used, &total_length, &seed_rand](std::vector<Branch> subtendril) -> float {
        // for now just vary it with less and less based on length_used
        float end_norm = 1.0 - length_used / total_length;
        float radian_offset = end_norm * (seed_rand(1.0, 3.0) - 2.0);
        return radian_offset;
    };

    const auto& thickness_calc = [&length_used, &total_length, &seed_rand](std::vector<Branch> subtendril) -> float {
        // for now just randomize it but taper to MIN based on length_used
        float end_norm = 1.0 - length_used / total_length;
        const auto& last_branch = subtendril.back();
        float last_thickness = v_length(last_branch.verts[0] - last_branch.verts[1]);
        return end_norm * seed_rand(0.5 * last_thickness, 2.5 * last_thickness);
    };

    const auto& make_branch = [&start_thickness](Vector2 start, float rotation, float length, float front_thickness, float back_thickness) -> Branch {
        const Vector2 mid_front = start + unit_vector(rotation) * length;
        const Vector2 perp_rotation = perp_rhr(unit_vector(rotation));
        Vector2 p1 = mid_front + perp_rotation * (front_thickness / 2.0);
        Vector2 p2 = mid_front - perp_rotation * (front_thickness / 2.0);
        Vector2 p3 = start - perp_rotation * (back_thickness / 2.0);
        Vector2 p4 = start + perp_rotation * (back_thickness / 2.0);
        return Branch({ p1, p2, p3, p4 });
    };

    // First we just make one branch with the correct orientation

    // start in middlish of screen
    Vector2 start { 300, 200 };
    auto start_branch = make_branch(start, start_rotation, total_length / 2, start_thickness * 0.8, start_thickness);

    // For now, no splitting.
    std::vector<Branch> tendril { start_branch };

    while (length_used / total_length < 0.99) {
        float length = length_calc(tendril);
        float angle = angle_calc(tendril);
        float thickness = thickness_calc(tendril);

        length_used += length;

        const auto& branch = tendril.back();
        Vector2 front = (branch.verts[0] + branch.verts[1]) / 2;
        Vector2 back = (branch.verts[2] + branch.verts[3]) / 2;
        Vector2 start = (front - back) * 0.95 + back;
        float branch_thickness = v_length(branch.verts[0] - branch.verts[1]);
        auto new_branch = make_branch(start, angle, length, thickness, branch_thickness);

        tendril.push_back(new_branch);
    }

    return { tendril };
}