#ifndef BUTTON_H
#define BUTTON_H

#include <string>
#include <functional>
#include <raylib.h>
#include "ui_element.cpp"

// Add shader support later.
struct Button;
struct LevelEditor;
struct Game;

struct Button : UiElement {
	// Destructor exists so we have polymorphic type metadata at runtime.
	struct Owner {
		virtual ~Owner() = default;
	};

	struct State {
		Owner* owner;
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

	std::function<void(Button&)> on_hover;
	std::function<void(Button&)> on_pressed;

	static int debug_count;
	int debug_id;

	Button(
		Owner* owner, Rectangle bounds, const std::string& text, 
		decltype(on_hover) on_hover,
		decltype(on_pressed) on_pressed,
		Color background_color = { 253, 249, 0, 100 },
		Color text_color = { 0, 0, 0, 255 });

	virtual ~Button();

	void take_input(Vector2 cursor) override;
	void update() override;
	bool in_use() const override;
};

#endif
