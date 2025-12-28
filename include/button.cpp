#include <iostream>
#include "button.hpp"

Button::Button(Button::Owner* owner, Vector2 pos, Vector2 dim, std::string text, 
    std::function<void(Button&)> on_hover,
    std::function<void(Button&)> on_hit) {
    this->hovered = false;
    this->last_hit = false;
    this->hit = false;
    this->owner = owner;
    idle_state = nullptr;
    
    std::cout << "created button\n";
    this->pos = pos;
    this->dim = dim;
    this->text = text;
    this->on_hover = on_hover;
    this->on_hit = on_hit;

    // Default values
    background_color = YELLOW;
    background_color.a = 100;
    text_color = BLACK;
}

Button::~Button() {
    std::cout << "deinit button\n";
    delete this->idle_state;
}

void Button::take_input(Vector2 cursor) {
    bool horz = pos.x < cursor.x && cursor.x < pos.x + dim.x;
    bool vert = pos.y < cursor.y && cursor.y < pos.y + dim.y;
    bool new_hovered = horz && vert;

    if (new_hovered && !hovered) {
        // Save idle state, aka this state
        std::cout << "free button\n";
        free(idle_state);
        idle_state = new Button(owner, pos, dim, text, on_hover, on_hit);
        on_hover(*this);
    }
    if (hit && !last_hit) {
        on_hit(*this);
        hit = false;
    }
    if (!new_hovered && hovered) {
        if (idle_state != nullptr) {
            // Restore idle state
            std::swap(pos, idle_state->pos);
            std::swap(dim, idle_state->dim);
            std::swap(text, idle_state->text);
            std::swap(background_color, idle_state->background_color);
            std::swap(text_color, idle_state->text_color);
            std::swap(on_hover, idle_state->on_hover);
            std::swap(on_hit, idle_state->on_hit);
        }
    }

    hovered = new_hovered;
    last_hit = hit;
}

void Button::render() const {
    auto color = background_color;
    if (hovered) {
        color = lerp(background_color, BLACK, 0.4);
    }
    DrawRectangle((int) pos.x, (int) pos.y, (int) dim.x, (int) dim.y, color);

    DrawText(text.c_str(), (int) pos.x, (int) pos.y, 20, text_color);
}