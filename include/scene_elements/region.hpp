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
		Color color);

	float t() const;

	void set_transition_time();

	void update() override;
};

#endif
