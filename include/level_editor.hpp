#ifndef LEVEL_EDITOR_H
#define LEVEL_EDITOR_H

#include "game.hpp"
#include "button.hpp"
#include <vector>

struct TreeMetadata {
    // Rotation affects all tendrils
    std::vector<Branch> branches;
    float rotation;

    TreeMetadata(float rotation, Tree& tree) {
        std::cout << "init tree metadata\n";
        this->rotation = rotation;

        branches.reserve(tree.branches.size());
        for (auto& branch : tree.branches)
            branches.push_back(branch);
    }
    ~TreeMetadata() {
        std::cout << "deinit tree metadata\n";
    }
};

class LevelEditor : public Button::Owner {
    public:
    std::vector<Button> buttons;

    LevelEditor(Game& game);
    ~LevelEditor();

    void initialize_ui();
    void update_rotation(Game& game);
    void update(Game& game);
    void render(Game& game) const;

    std::vector<TreeMetadata> tree_metadatas;
    size_t selected_index, last_selected_index;
    Texture2D selected_tex;
    float time;

    private:
    Vector2 select_extra_bounds { 10, 10 };
    Shader select_shader;
    void load_selection_shader(Game& game);
};

#endif