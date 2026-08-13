#include "ui_element_manager.hpp"
#include <format>
#include <print>
#include <algorithm>

#include "ui_element.cpp"

UiElementManager::ManagerView::~ManagerView() {
	update();
}

bool UiElementManager::ManagerView::any_in_use() const {
	for (const auto& name : manager.names) {
		const auto& elem = manager.named_elements[name];
		if (!elem.is_active)
			continue;
		if (elem.elem->in_use())
			return true;
	}
	return false;
}

bool UiElementManager::ManagerView::any_hovered() const {
	for (const auto& name : manager.names) {
		const auto& elem = manager.named_elements[name];
		if (!elem.is_active)
			continue;
		if (elem.elem->hovered)
			return true;
	}
	return false;
}

void UiElementManager::ManagerView::take_input(Vector2 cursor) {
	for (const auto& name : manager.names) {
		const auto& elem = manager.named_elements[name];
		if (!elem.is_active)
			continue;
		elem.elem->take_input(cursor);
	}
}

void UiElementManager::ManagerView::update() {
	// Danger, manager.names can be adjusted during iteration, that's sus!!!
	// Therefore, we use deferred removal.
	for (const auto& name : manager.names) {
		if (std::ranges::contains(manager.mark_for_removal, name))
			continue;
		manager.named_elements[name].elem->update();
	}
	for (const auto& name : manager.mark_for_removal) {
		manager.named_elements.erase(name);
		std::erase(manager.names, name);
	}
	manager.mark_for_removal.clear();
}

void UiElementManager::init() {
	names.clear();
	named_elements.clear();
	mark_for_removal.clear();
}

std::string UiElementManager::add(UiElement* element, const std::string& name) {
	const std::string& str = std::format("{}#{}", name, names.size());
	std::println("ADD {}", name);
	named_elements[str] = { std::unique_ptr<UiElement>(element), true };
	names.push_back(str);
	return str;
}

void UiElementManager::set_active(const std::string& name, bool status) {
	named_elements[name].is_active = status;
}

void UiElementManager::remove(const std::string& name) {
	const bool debug = true;
	if (debug && !named_elements.contains(name))
		throw std::runtime_error(std::format(".remove {} NOT IN named_elements", name));
	std::println("mark for removal {} from NAMED_ELEMENTS", name);

	mark_for_removal.push_back(name);
}

template <typename T>
requires std::derived_from<T, UiElement>
T* UiElementManager::reinterpret(const std::string& name) {
	const bool debug = true;
	if (debug && !named_elements.contains(name))
		throw std::runtime_error(std::format(".reinterpret {} NOT IN named_elements", name));
	T* data = reinterpret_cast<T*>(named_elements.at(name).elem.get());
	return data;
}

template <typename T>
requires std::derived_from<T, UiElement>
T* UiElementManager::get(const std::string& name) {
	// For searchability
	const bool debug = true;
	if (debug && !named_elements.contains(name))
		throw std::runtime_error(std::format(".get {} NOT IN named_elements", name));
	T* data = dynamic_cast<T*>(named_elements.at(name).elem.get());
	if (debug && data == nullptr)
		throw std::runtime_error(std::format("BAD CAST for named_elements[{}]", name));
	return data;
}

void UiElementManager::render() const {
	for (const auto& name : names) {
		// std::println("render call at?");
		const auto& ui_elem = named_elements.at(name);
		if (!ui_elem.is_active)
			continue;
		// std::println("render did call at");
		ui_elem.elem->render_fn(ui_elem.elem.get());
	}
}

UiElementManager::ManagerView UiElementManager::update(Vector2 cursor) {
	ManagerView view { .manager = *this };
	view.take_input(cursor);

	return view;
}
