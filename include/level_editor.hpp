#include "button.hpp"
#include <vector>

struct TreeMetadata {
    float rotation;
    size_t id;

    TreeMetadata(float rotation, size_t id) {
        this->rotation = rotation;
        this->id = id;
    }
};

class LevelEditor : public Button::Owner {
    public:
    std::vector<Button> buttons;

    std::string blah = "blahhh";

    LevelEditor();
    ~LevelEditor();

    void initialize_ui();
    void update();

    private:
    std::vector<TreeMetadata> tree_metadatas;
};