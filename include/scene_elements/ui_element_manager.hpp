#ifndef UI_ELEMENT_MANAGER_H
#define UI_ELEMENT_MANAGER_H

#include <raylib.h>
#include <vector>
#include <map>
#include <memory>

#include "ui_element.cpp"

struct UiElementGroup {
	std::vector<std::string> names;
};

struct UiElementManager {
	private:
	struct ManagerView {
		UiElementManager& manager;

		~ManagerView();

		bool any_in_use() const;
		bool any_hovered() const;
		void take_input(Vector2 cursor);

		void update();
	};

	public:
	struct ManagedElement {
		std::unique_ptr<UiElement> elem;
		bool is_active;
	};

	std::vector<std::string> names;
	// Must store a ptr because they are polymorphic
	std::map<std::string, ManagedElement> named_elements;
	// In case things are "removed" during update running
	std::vector<std::string> mark_for_removal;

	void init();
	std::string add(UiElement* element, const std::string& name); 
	void set_active(const std::string& name, bool status);
	void remove(const std::string& name);

	// Used for obtaining a UiElement type where I want to ignore the template T type.
	template <typename T>
	requires std::derived_from<T, UiElement>
	T* reinterpret(const std::string& name);

	template <typename T>
	requires std::derived_from<T, UiElement>
	T* get(const std::string& name);

	void render() const;
	ManagerView update(Vector2 cursor);
};

#endif
