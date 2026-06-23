#include "ui_element.cpp"

struct Region : UiElement {
	// Can be adjusted for any use
	struct State {
		bool last_hovered = false;
		virtual ~State() = default;
	};
	std::unique_ptr<State> state;
	std::unique_ptr<State> idle_state;
	std::function<void(Region&)> on_hover;

	Region(
		Rectangle bounds, 
		decltype(on_hover) on_hover, 
		std::unique_ptr<State> state, 
		std::unique_ptr<State> idle_state, 
		Color color) {
		this->state = std::move(state);
		this->idle_state = std::move(idle_state);
		this->on_hover = on_hover;
		this->bounds = bounds;
		this->color = color;
	}

	virtual ~Region() = default;

	void update() override {
		// Don't do this dumb shit
		if (hovered != state->last_hovered)
			state.swap(idle_state);
		
		if (hovered)
			on_hover(*this);

		state->last_hovered = hovered;
	}
};
