#ifndef LEVEL_EDITOR_H
#define LEVEL_EDITOR_H

#include "game.hpp"
#include "button.hpp"
#include <vector>

struct TreeMetadata {
    // Rotation affects all tendrils
    std::vector<Branch> branches;
    // I don't trust the zero-initialization
    Vector2 offset = {};
    float rotation = 0;

    static TreeMetadata zero() {
        return {};
    }
    TreeMetadata(float rotation, Vector2 offset, Tree& tree) {
        std::cout << "init tree metadata\n";
        this->rotation = rotation;
        this->offset = offset;

        branches.reserve(tree.branches.size());
        for (auto& branch : tree.branches)
            branches.push_back(branch);
    }
    ~TreeMetadata() {
        std::cout << "deinit tree metadata\n";
    }
    private:
    TreeMetadata() {}
};

struct LevelEditor : public Button::Owner {
    std::vector<Button> buttons;
    Button* debug_button;
    std::vector<TreeMetadata> tree_metadatas;
    size_t selected_index, last_selected_index;
    Texture2D selected_tex;
    Vector2 selection_offset;
    float time;
    bool using_ui;

    LevelEditor();
    ~LevelEditor();

    void make_initialized_tree(Game& game, const TreeMetadata& metadata);
    void initialize_ui();
    void randomize_tendrils(Game& game);
    void update_selected_verts(Game& game);
    void load_selection_shader(Game& game);
    void update(Game& game);
    void render(Game& game) const;

    private:
    bool show_instructions;
    Vector2 select_extra_bounds { 10, 10 };
    Shader select_shader;
};

#endif