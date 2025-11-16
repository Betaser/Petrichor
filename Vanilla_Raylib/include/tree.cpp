#include <iostream>
#include <cmath>
#include "tree.hpp"

Tree::Tree(std::vector<Branch> branches, Shader shader) {
    this->branches = branches;
    for (int j = 0; j < (int) branches.size(); j++) {
        for (int i = 0; i < 4; i++) {
            this->branches[j].verts[i] = branches[j].verts[i] * 30 + Vector2{400, 300};
        }
    }
    this->shader = shader;
}

void Tree::init_texture() {
    // Bounding box it
    Vector2 small {9999, 9999};
    Vector2 big {-9999, -9999};
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

void Tree::render() {
    // We'll replace this.
    int color_loc = GetShaderLocation(shader, "color");
    Vector4 white {1.0, 1.0, 1.0, 1.0};
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