#include <print>

#include "button.hpp"

int Button::debug_count = 0;

Button::Button(Button::Owner* owner, Rectangle bounds, const std::string& text, 
	std::function<void(Button&)> on_hover,
	std::function<void(Button&)> on_pressed,
	Color background_color,
	Color text_color) {
	hovered = false;
	color = background_color;
	state.last_hit = false;
	state.hit = false;
	state.owner = owner;
	
	debug_id = debug_count++;
	std::println("created button #{}", debug_id);
	this->bounds = bounds;
	state.text = text;
	this->on_hover = on_hover;
	this->on_pressed = on_pressed;

	// Default values
	state.text_color = text_color;

idle_state = state;

	render_fn = [](const UiElement* ui_element) {
		const auto& self = *dynamic_cast<const Button*>(ui_element);
		const auto [pos, dims] = to_pos_dims(self.bounds);
		const auto& text = self.state.text;
		const auto& text_color = self.state.text_color;

		const auto c = self.hovered
			? ColorLerp(self.color, BLACK, 0.4)
			: self.color;

		DrawRectangle(pos.x, pos.y, dims.x, dims.y, c);
		DrawText(text.c_str(), pos.x, pos.y, 20, text_color);
	};
}

Button::~Button() {
	std::println("deinit button #{}", debug_id);
}

void Button::take_input(Vector2 cursor) {
	state.last_hovered = hovered;
	UiElement::take_input(cursor);
}

void Button::update() {
	auto& hit = state.hit;
	auto& last_hit = state.last_hit;
	auto& last_hovered = state.last_hovered;

	// Run this first, since on_pressed could adjust hovered.
	if (state.pressed) {
		// Then the click has occured.
		on_pressed(*this);
	}

	// We need to go through a hover state with no click and then a hover state with click to switch to pressed.

	// Update queued state.

	if (!last_hovered && hovered) {
		// Save idle state, aka this state
		on_hover(*this);

		if (!state.hit) {
			auto tmp = idle_state;
			idle_state = state;
			state = tmp;
		}
	}

	if (hit && !last_hit && hovered && last_hovered)
		state.pressed = true;

	if (!hit)
		state.pressed = false;

	// What?
	last_hit = hit;
	hit = false;

	if (last_hovered && !hovered && !state.pressed) {
		auto tmp = idle_state;
		idle_state = state;
		state = tmp;
	}
}

bool Button::in_use() const {
	return hovered || state.pressed;
}
