#include <raylib.h>
#include <vector>
#include <map>
#include <memory>
#include <format>

#include "ui_element.cpp"

struct UiElementManager {
	private:
	struct ManagerView {
		UiElementManager& manager;

		~ManagerView() {
			update();
		}

		bool any_in_use() const {
			for (const auto& name : manager.names) {
				if (manager.named_elements[name]->in_use()) {
					return true;
				}
			}
			return false;
		}
		
		bool any_hovered() const {
			for (const auto& name : manager.names) {
				if (manager.named_elements[name]->hovered) {
					return true;
				}
			}
			return false;
		}

		void take_input(Vector2 cursor) {
			for (const auto& name : manager.names)
				manager.named_elements[name]->take_input(cursor);
		}

		void update() {
			for (const auto& name : manager.names)
				manager.named_elements[name]->update();
		}
	};

	public:
	std::vector<std::string> names;
	// Must store a ptr because they are polymorphic
	std::map<std::string, std::unique_ptr<UiElement>> named_elements;

	std::string add(std::unique_ptr<UiElement> element, const std::string& s) {
		const std::string& str = std::format("{}#{}", s, names.size());
		named_elements[str] = std::move(element);
		names.push_back(str);
		return str;
	}

	void remove(const std::string& s) {
		named_elements.erase(s);
		// std::remove just moves elems to end
		names.erase(std::remove(names.begin(), names.end(), s), names.end());
	}

	template <typename T>
	requires std::derived_from<T, UiElement>
	T* get(const std::string& name) {
		// std::println("get call at?");
		T* data = dynamic_cast<T*>(named_elements.at(name).get());
		// std::println("get did call at");
		return data;
	}

	void render() const {
		for (const auto& name : names) {
			// std::println("render call at?");
			const auto& ui_elem = named_elements.at(name);
			// std::println("render did call at");
			ui_elem->render_fn(ui_elem.get());
		}
	}

	ManagerView update(Vector2 cursor) {
		ManagerView view { .manager = *this };
		view.take_input(cursor);

		return view;
	}
};
