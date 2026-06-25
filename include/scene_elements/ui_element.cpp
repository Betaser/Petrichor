#ifndef UI_ELEMENT_H
#define UI_ELEMENT_H

#include <functional>
#include <raylib.h>

#include "../globals/mylib.hpp"

struct UiElement {
	Rectangle bounds;	
	Color color { 0, 0, 0, 0 };
	bool hovered = false;

	virtual ~UiElement() = default;

	std::function<void(UiElement*)> render_fn = nullptr;

	bool calc_is_hovered(Vector2 cursor) const {
		return pt_in_rect(cursor, bounds);
	}

	virtual void take_input(Vector2 cursor) {
		hovered = calc_is_hovered(cursor);
	}

	// Updates state, whatever the child classes want to do here.
	virtual void update() {}

	virtual bool in_use() const {
		return hovered;
	}
};

#endif
