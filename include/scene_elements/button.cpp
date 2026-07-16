#include <print>

#include "button.hpp"

template <typename T>
Button<T>::Button(
	const T& data, 
	Rectangle bounds, 
	const std::string& text, 
	std::function<void(Button<T>&)> on_hover,
	std::function<void(Button<T>&)> on_pressed,
	Color background_color,
	Color hover_color,
	Color text_color) {
	hovered = false;
	color = background_color;
	this->hover_color = hover_color;
	state.last_hit = false;
	state.hit = false;
	state.text = text;
	
	debug_id = DebugButton::debug_count++;
	std::println("created button #{}", debug_id);
	this->data = data;
	this->bounds = bounds;
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
			? ColorLerp(self.color, self.hover_color, 0.4)
			: self.color;

		DrawRectangle(pos.x, pos.y, dims.x, dims.y, c);
		DrawText(text.c_str(), pos.x, pos.y, 20, text_color);
	};
}

template <typename T>
Button<T>::~Button() {
	std::println("deinit button #{}", debug_id);
}

template <typename T>
void Button<T>::take_input(Vector2 cursor) {
	state.last_hovered = hovered;
	UiElement::take_input(cursor);
}

template <typename T>
void Button<T>::update() {
	auto& hit = state.hit;
	auto& last_hit = state.last_hit;
	auto& last_hovered = state.last_hovered;

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

	// Run this first, since on_pressed could adjust hovered.
	if (state.pressed) {
		// Then the click has occured.
		on_pressed(*this);
	}

	// What?
	last_hit = hit;
	hit = false;

	if (last_hovered && !hovered && !state.pressed) {
		auto tmp = idle_state;
		idle_state = state;
		state = tmp;
	}
}

template <typename T>
bool Button<T>::in_use() const {
	return hovered || state.pressed;
}
