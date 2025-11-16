#include "mylib.cpp"
#include <vector>
#include "raylib.h"

class Branch {
    public:
    std::vector<Vector2> verts;

    Branch(std::vector<Vector2> verts);
};

class Tree {
    private:
    Vector2I texture_pos;
    Texture2D tex;
    Shader shader;
    Vector2 compressed_branches[4][100];

    public:
    std::vector<Branch> branches;

    Tree(std::vector<Branch> branches, Shader shader);

    void init_texture();

    void render();
};