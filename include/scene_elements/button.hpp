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
		Color text_color = { 0, 0, 0, 255 });

	Button(std::function<void(Button<T>&)> on_hover);

	virtual ~Button();

	void take_input(Vector2 cursor) override;
	void update() override;
	bool in_use() const override;
};

#endif
