#ifndef REGION_H
#define REGION_H

#include <functional>

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
		if (hovered)
			on_hover(*this);

		if (hovered != state->last_hovered)
			state.swap(idle_state);

		state->last_hovered = hovered;
	}
};

template <typename T>
struct TRegion : Region {
	struct TState : Region::State {
		float last_hovered_time;
		T hover_data;

		TState(float last_hovered_time, const T& hover_data) {
			this->last_hovered_time = last_hovered_time;
			this->hover_data = hover_data;
		}

		virtual ~TState() = default;
	};

	double DURATION;
	float transition_time = -999;
	float* time;
	std::unique_ptr<State>* target;
	std::function<void(TRegion&, TState&, TState&, float)> lerp_fn;
	std::function<float(float)> adjust_t = [](float t) { return t; };

	// Too weird to define in a cpp file.
	TRegion(
		double DURATION,
		float* time,
		Rectangle bounds, 
		decltype(lerp_fn) lerp_fn,
		std::unique_ptr<TState> state, 
		std::unique_ptr<TState> idle_state, 
		Color color) : 
		Region(
			bounds, 
			[](auto& self) {
				auto& r = dynamic_cast<TRegion&>(self);
				if (!r.state->last_hovered) {
					r.target = &self.idle_state;	
					r.set_transition_time();
				}
			}, 
			std::move(state), 
			std::move(idle_state), 
			color) {
			this->DURATION = DURATION;
			this->time = time;
			target = &this->state;
			this->lerp_fn = lerp_fn;
		}

	float t() {
		float n = std::max(0.0, std::min(1.0, (*time - transition_time) / DURATION));
		return adjust_t(n);
	}

	void set_transition_time() {
		float t_remaining = std::max(0.0, 1.0 - t());
		transition_time = *time - t_remaining * DURATION;
	}

	void update() override {
		if (!hovered && state->last_hovered) {
			target = &this->idle_state;
			set_transition_time();
		}

		Region::update();

		auto curr_s = *dynamic_cast<TState*>(state.get());
		auto target_s = *dynamic_cast<TState*>(target->get());
		lerp_fn(*this, curr_s, target_s, t());
	}
};

#endif
