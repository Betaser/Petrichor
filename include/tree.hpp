#include "mylib.cpp"
#include <vector>
#include "raylib.h"

class Branch {
    public:
    std::vector<Vector2> verts;

    Branch(std::vector<Vector2> verts);

    Vector2 forward() const;
    Vector2 front() const;
};

class Tree {
    private:
    Vector2I texture_pos;
    Texture2D tex;
    Shader shader;
    Vector2 compressed_branches[4][100];

    public:

    // Contains same branches as in tendrils
    std::vector<Branch> branches;
    std::vector<std::vector<std::vector<Branch>>> tendrils;

    Tree(std::vector<Branch> branches, Shader shader);

    void init_texture();
    void make_branches_tendrils();
    void render();

    // Does not figure out how we want to render it.
    static std::vector<std::vector<Branch>> random_tendril_config(int seed, float total_length, float start_thickness, float start_rotation, float split_chance);
};