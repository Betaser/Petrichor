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

    std::string blah = "blahhh";

    LevelEditor();
    ~LevelEditor();

    void initialize_ui();
    void update_rotation(Game& game);
    void update(Game& game);

    std::vector<TreeMetadata> tree_metadatas;
    private:
};