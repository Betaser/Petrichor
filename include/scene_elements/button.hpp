#ifndef BUTTON_H
#define BUTTON_H

#include <string>
#include <functional>
#include <raylib.h>
#include "ui_element.cpp"

// Add shader support later.
struct LevelEditor;
struct Game;

namespace DebugButton {
	static int debug_count = 0;
}

template <typename T>
struct Button : UiElement {
	// Destructor exists so we have polymorphic type metadata at runtime.

	struct State {
		bool pressed = false;
		bool last_hovered = false;
		bool last_hit = false;
		// Set hit when trying to run on_hit.
		bool hit = false;
		Color text_color;
		std::string text;
	};

	State state;
	// This is good enough for hovering/not hovering, but we will need another state for pressing or not.
	// For now, just make pressing not require another state.
	State idle_state;
	Color hover_color;
	
	T data;

	std::function<void(Button<T>&)> on_hover;
	std::function<void(Button<T>&)> on_pressed;

	int debug_id;

	Button(
		const T& data, 
		Rectangle bounds, 
		const std::string& text, 
		std::function<void(Button<T>&)> on_hover,
		std::function<void(Button<T>&)> on_pressed,
		Color background_color = { 253, 249, 0, 100 },
		Color hover_color = { 0, 0, 0, 255 },
		Color text_color = { 0, 0, 0, 255 });

	Button(std::function<void(Button<T>&)> on_hover);

	virtual ~Button();

	void take_input(Vector2 cursor) override;
	void update() override;
	bool in_use() const override;

	static void render_button(const UiElement* ui_element) {
		const auto& self = *dynamic_cast<const Button<T>*>(ui_element);
		const auto [pos, dims] = to_pos_dims(self.bounds);
		const auto& text = self.state.text;
		const auto& text_color = self.state.text_color;

		const auto c = self.hovered
			? ColorLerp(self.color, self.hover_color, 0.4)
			: self.color;

		DrawRectangle(pos.x, pos.y, dims.x, dims.y, c);
		DrawText(text.c_str(), pos.x, pos.y, 20, text_color);
	}
};

#endif
