#include "mylib.cpp"
#include <vector>
#include "raylib.h"

class Branch;
using Tendrils = std::vector<std::vector<std::vector<Branch>>>;

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
    // Hold onto tree_tex just to unload it.
    Texture2D blank_tex, tree_tex;
    Shader shader;
    Vector2 compressed_branches[4][100];

    public:

    // Contains same branches as in tendrils
    std::vector<Branch> branches;
    Tendrils tendrils;

    Tree(std::vector<Branch> branches, Shader shader);
    ~Tree();

    void unload_textures();
    void init_texture();
    void render();

    static std::vector<Branch> branches_from_tendrils(Tendrils tendrils);

    // Does not figure out how we want to render it.
    static std::vector<std::vector<Branch>> random_tendril_config(int seed, float total_length, float start_thickness, float start_rotation, float split_chance);
};