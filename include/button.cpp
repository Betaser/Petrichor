#include <iostream>
#include "button.hpp"

Button::Button(Button::Owner* owner, Vector2 pos, Vector2 dim, std::string text, 
	std::function<void(Button&)> on_hover,
	std::function<void(Button&)> on_hit,
	Color background_color,
	Color text_color) {
	state.hovered = false;
	state.last_hit = false;
	state.hit = false;
	state.owner = owner;
	
	std::cout << "created button\n";
	state.pos = pos;
	state.dim = dim;
	state.text = text;
	this->on_hover = on_hover;
	this->on_hit = on_hit;

	// Default values
	state.background_color = background_color;
	state.text_color = text_color;

	idle_state = state;
}

Button::~Button() {
	std::cout << "deinit button\n";
}

void Button::take_input(Vector2 cursor) {
	auto& pos = state.pos;
	auto& hit = state.hit;
	auto& last_hit = state.last_hit;
	auto& dim = state.dim;
	auto& hovered = state.hovered;

	bool horz = pos.x < cursor.x && cursor.x < pos.x + dim.x;
	bool vert = pos.y < cursor.y && cursor.y < pos.y + dim.y;
	bool new_hovered = horz && vert;

	if (new_hovered && !hovered) {
		// Save idle state, aka this state
		std::cout << "copy constructor button\n";
		idle_state = state;
		on_hover(*this);
	}
	if (hit && !last_hit) {
		on_hit(*this);
		hit = false;
	}
	if (!new_hovered && hovered) {
		state = idle_state;
	}

	hovered = new_hovered;
	last_hit = hit;
}

void Button::render() const {
	auto& background_color = state.background_color;
	auto& hovered = state.hovered;
	auto& dim = state.dim;
	auto& pos = state.pos;
	auto& text = state.text;
	auto& text_color = state.text_color;

	auto color = background_color;
	if (hovered) {
		color = ColorLerp(background_color, BLACK, 0.4);
	}
	DrawRectangle(pos.x, pos.y, dim.x, dim.y, color);

	DrawText(text.c_str(), pos.x, pos.y, 20, text_color);
}