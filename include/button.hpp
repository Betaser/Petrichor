#ifndef BUTTON_H
#define BUTTON_H

#include <string>
#include <functional>
#include <raylib.h>

#include "mylib.hpp"
#include "level_editor.hpp"

// Add shader support later.
struct Button;
struct LevelEditor;
struct Game;

struct Button {
	// Destructor exists so we have polymorphic type metadata at runtime.
	struct Owner {
		virtual ~Owner() = default;
	};
	struct State {
		Owner* owner;
		Vector2 pos {};
		Vector2 dim {};
		bool hovered = false;
		bool last_hit = false;
		// Set hit when trying to run on_hit.
		bool hit = false;
		Color background_color;
		Color text_color;
		std::string text;
	} state;

	// This is good enough for hovering/not hovering, but we will need another state for pressing or not.
	// For now, just make pressing not require another state.
	State idle_state;

	std::function<void(Button&)> on_hover;
	std::function<void(Button&)> on_hit;

	Button(Owner* owner, Vector2 pos, Vector2 dim, std::string text, 
		std::function<void(Button&)> on_hover,
		std::function<void(Button&)> on_hit,
		Color background_color = { 253, 249, 0, 100 },
		Color text_color = { 0, 0, 0, 255 });
	~Button();
	void take_input(Vector2 cursor);
	void render() const;
};

#endif