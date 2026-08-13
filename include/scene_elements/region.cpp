#include "region.hpp"

template <typename T>
template <typename F>
Region<T>* Region<T>::init(
	double duration,
	float* time,
	Rectangle bounds, 
	const F& lerp_fn,
	const T& state_data, 
	const T& idle_state_data, 
	Color color) {
	// Can I just use std::move?
	this->state = std::make_unique<State>(false, state_data);
	this->idle_state = std::make_unique<State>(false, idle_state_data);
	on_hover = [](auto& self) {
		auto& r = dynamic_cast<Region&>(self);
		if (!r.state->last_hovered) {
			r.target = &self.idle_state;	
			r.set_transition_time();
		}
	}, 
	this->bounds = bounds;
	this->color = color;
	this->duration = duration;
	this->time = time;
	target = &this->state;
	this->lerp_fn = lerp_fn;
	return this;
}

template <typename T>
float Region<T>::t() const {
	float n = std::max(0.0, std::min(1.0, (*time - transition_time) / duration));
	return adjust_t(n);
};

template <typename T>
void Region<T>::set_transition_time() {
	float t_remaining = std::max(0.0, 1.0 - t());
	transition_time = *time - t_remaining * duration;
}

template <typename T>
void Region<T>::update() {
	if (!hovered && state->last_hovered) {
		target = &this->idle_state;
		set_transition_time();
	}

	if (hovered)
		on_hover(*this);

	if (hovered != state->last_hovered)
		state.swap(idle_state);

	state->last_hovered = hovered;

	auto curr_s = *dynamic_cast<State*>(state.get());
	auto target_s = *dynamic_cast<State*>(target->get());
	lerp_fn(*this, curr_s, target_s, t());
}

/*
#ifndef REGION_H
#define REGION_H

#include <functional>

#include "ui_element.cpp"

template <typename T>
struct Region : UiElement {
	struct State {
		bool last_hovered = false;
		T data;
	};
	std::unique_ptr<State> state, idle_state;
	std::function<void(Region&)> on_hover;
	double duration;
	float transition_time = -999;
	float* time;
	decltype(state)* target;
	std::function<void(Region<T>&, typename Region<T>::State&, typename Region<T>::State&, float)> lerp_fn;
	std::function<float(float)> adjust_t = [](float t) { return t; };

	template <typename F>
	Region* init(
		double duration,
		float* time,
		Rectangle bounds, 
		const F& lerp_fn,
		const T& state_data, 
		const T& idle_state_data, 
		Color color) {
		// Can I just use std::move?
		this->state = std::make_unique<State>(false, state_data);
		this->idle_state = std::make_unique<State>(false, idle_state_data);
		on_hover = [](auto& self) {
			auto& r = dynamic_cast<Region&>(self);
			if (!r.state->last_hovered) {
				r.target = &self.idle_state;	
				r.set_transition_time();
			}
		}, 
		this->bounds = bounds;
		this->color = color;
		this->duration = duration;
		this->time = time;
		target = &this->state;
		this->lerp_fn = lerp_fn;
		return this;
	}

	float t() {
		float n = std::max(0.0, std::min(1.0, (*time - transition_time) / duration));
		return adjust_t(n);
	}

	void set_transition_time() {
		float t_remaining = std::max(0.0, 1.0 - t());
		transition_time = *time - t_remaining * duration;
	}

	void update() override {
		if (!hovered && state->last_hovered) {
			target = &this->idle_state;
			set_transition_time();
		}

		if (hovered)
			on_hover(*this);

		if (hovered != state->last_hovered)
			state.swap(idle_state);

		state->last_hovered = hovered;

		auto curr_s = *dynamic_cast<State*>(state.get());
		auto target_s = *dynamic_cast<State*>(target->get());
		lerp_fn(*this, curr_s, target_s, t());
	}
};

#endif
*/
