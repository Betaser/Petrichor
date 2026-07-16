// Only lives in game, hence no header file 
#include <vector>

#include "../globals/game.hpp"
#include "../scene_elements/button.hpp"
#include "../gamestates/states.cpp"

struct PauseMenu {
	struct Setting {
		std::string text;
		Color background_color;
		State state;
	};

	// Instead of a shader, just render a plain background
	std::vector<std::unique_ptr<Button<nullptr_t>>> buttons;
	std::vector<Setting> settings;
	const float HORZ_SPACING = 50;
	const float BUTTON_WIDTH = 130;
	const float BUTTON_HEIGHT = 70;
	bool active = false;
	Game* game;

	private:
	void make_button(Vector2 pos, Setting& setting, Game& game) {
		buttons.push_back(std::make_unique<Button<nullptr_t>>(
			nullptr,
			to_rect(pos, { BUTTON_WIDTH, BUTTON_HEIGHT }),
			setting.text,
			[](Button<nullptr_t>& _) {},
			[&](Button<nullptr_t>& _) {
				game.state = setting.state;
				active = false;
			},
			setting.background_color,
			Color { 50, 0, 50, 255 },
			WHITE
		));
	}

	public:
	PauseMenu(Game& game) {
		Color light_blue { 0, 50, 255, 255 };
		settings = {
			{ 
				"Play", 
				light_blue, 
				PlayLevel, 
			},
			{ 
				"Edit Level", 
				ORANGE, 
				EditLevel 
			}
		};
		// Centered vertically, with HORZ_SPACING
		float y_pos = ((float) game.screen_height - BUTTON_HEIGHT) / 2;
		float leftmost_x = ((float) game.screen_width - BUTTON_WIDTH * (float) settings.size() - HORZ_SPACING * ((float) settings.size() - 1)) / 2;
		for (size_t i = 0; i < settings.size(); i++) {
			make_button(
				{ 
					(BUTTON_WIDTH + HORZ_SPACING) * i + leftmost_x, 
					y_pos 
				},
				settings[i],
				game);
		}
	}

	void update() {
		if (IsKeyPressed(KEY_TAB))
			active = !active;

		if (active) {
			const Vector2 cursor = GetMousePosition();
			for (auto& button : buttons) {
				if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && button->hovered)
					button->state.hit = true;

				button->take_input(cursor);
				button->update();
			}
		}
	}

	void render(int screen_width, int screen_height) {
		if (!active)
			return;

		Color background_color { 50, 0, 0, 100 };
		DrawRectangle(0, 0, screen_width, screen_height, background_color);

		for (const auto& button : buttons)
			button->render_fn(button.get());
	}
};
