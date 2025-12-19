#ifndef BUTTON_H
#define BUTTON_H

#include <string>
#include "mylib.hpp"
#include <functional>
#include "raylib.h"
#include "level_editor.hpp"

// Add shader support later.
class Button;
class LevelEditor;
struct Game;

struct ButtonOwner {
    union {
        LevelEditor* level_editor;
        Game* game;
    } data;
    enum Types {
        LEVEL_EDITOR,
        GAME
    } type;
};

class Button {
    public:

    // Destructor exists so we have polymorphic type metadata at runtime.
    struct Owner {
        virtual ~Owner() = default;
    };

    Owner* owner;
    ButtonOwner btn_owner;
    Vector2 pos;
    Vector2 dim;
    bool hovered;
    Color background_color;
    Color text_color;
    std::string text;

    Button* idle_state;

    std::function<void(Button&)> on_hover;

    Button(Owner* owner, Vector2 pos, Vector2 dim, std::string text, std::function<void(Button&)> on_hover);
    ~Button();
    void take_input(Vector2 cursor);
    void render() const;
};

#endif