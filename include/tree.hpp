#include "mylib.cpp"
#include <array>
#include <vector>
#include "raylib.h"
#include <random>

class Branch;
using Tendrils = std::vector<std::vector<std::vector<Branch>>>;

class Branch {
    public:
    std::vector<Vector2> verts;

    Branch(std::vector<Vector2> verts);

    const Vector2 forward() const;
    const Vector2 front() const;
    const Vector2 back() const;
    const float front_thickness() const;
    const float back_thickness() const;
    const Branch clone() const;
};

// Might have to ifndef this
class Tree {
    private:
    static const int MAX = 100;
    Vector2I texture_pos;
    // Hold onto tree_tex just to unload it.
    Texture2D blank_tex, tree_tex;
    Shader shader;
    // std::array<std::array<Vector2, 4>, MAX> compressed_branches;
    Vector2 compressed_branches[4][MAX];
    Vector2 btm_lefts[MAX];
    Vector2 top_rights[MAX];


    public:
    Rand rand;
    // Contains same branches as in tendrils
    std::vector<Branch> branches;
    Tendrils tendrils;

    Tree(std::vector<Branch> branches, Shader shader, Rand& rand);
    ~Tree();

    void unload_textures();
    void init_texture();
    void render();

    static std::vector<Branch> branches_from_tendrils(Tendrils tendrils);

    // Does not figure out how we want to render it.
    std::vector<std::vector<Branch>> random_tendril_config(float total_length, float start_thickness, float start_rotation, float thickness_cutoff, Vector2 start_location, int MAX_TENDRILS);
};