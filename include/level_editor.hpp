#include <sstream>
#include "button.hpp"
#include "../main.hpp"
#include "tree.hpp"
#include <map>

struct TreeAddOns {
    float rotation = 0;
};

class LevelEditor : ButtonOwner {
    public:
    enum class MouseMode {
        BRUSH_PLACEMENT,
        SELECT,
        NONE
    };
    struct MoreMouseMode {
        const MouseMode mode;
        const std::string name;
        const unsigned int index;
    };
    const std::vector<MoreMouseMode> more_mouse_modes = {
        { MouseMode::BRUSH_PLACEMENT, "BRUSH_PLACEMENT", 0 },
        { MouseMode::SELECT, "SELECT", 1 },
        { MouseMode::NONE, "NONE", 2 },
    };

    MouseMode mouse_mode = MouseMode::NONE;

    std::map<int, TreeAddOns> treeIndexesAndAddOns;
    std::vector<Button*> buttons;

    LevelEditor();

    const MoreMouseMode from_mode() const;
    void initialize_ui();
    void update(const Main& main);
};