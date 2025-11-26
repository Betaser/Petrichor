#include <string>
#include "mylib.hpp"
#include <functional>
#include "raylib.h"

#ifndef BUTTON_H
#define BUTTON_H

// Add shader support later.
class Button;

class Button {
    public:

    Vector2 pos;
    Vector2 dim;
    bool hovered;
    Color background_color;
    Color text_color;
    std::string text;

    Button* idle_state;

    std::function<void(Button&)> on_hover;

    Button(Vector2 pos, Vector2 dim, std::string text, std::function<void(Button&)> on_hover);
    ~Button();
    void take_input(Vector2 cursor);
    void render() const;
};

#endif