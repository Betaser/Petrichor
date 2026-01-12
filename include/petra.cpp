#include <string>
#include <tuple>

#include "petra.hpp"
#include "mylib.hpp"

Petra::Petra() {
	std::cout << "initialized Petra\n";

	hitbox_radius = 50;
}

Circle Petra::get_hitbox() const {
	return {
		.pos = pos,
		.radius = hitbox_radius
	};
}

void Petra::update() {
	if (IsKeyPressed(KEY_LEFT_SHIFT))
		we_are_debugging = !we_are_debugging;

	if (we_are_debugging)
		debug_update_movement();
	else
		update_movement();
}

void Petra::render(Game& game, Level& level) {
	// Show info that we are debugging
	float font_size = 30;
	auto [color, str] = we_are_debugging ? 
		std::tuple<Color, const char*>(PURPLE, "petra debug movement") :
		std::tuple<Color, const char*>(ORANGE, "petra standard movement");
	if (we_are_debugging) {
		str = "petra debug movement";
		color = PURPLE;
	} else {
		str = "petra standard movement";
		color = ORANGE;
	}
	int text_length = MeasureText(str, font_size);
	DrawText(str, (game.screen_width - text_length) / 2, game.screen_height / 2 + 200, font_size, color);

	// Render her hitbox
	Color hitbox_color { 255, 255, 255, 180 };

	auto new_pos = pos;

	// Right now, cam USES petra's depth, therefore the scale is 100%.
	Cam depth_cam = level.camera.clone();
	depth_cam.scale = 1.0 / (level.collision_dist * depth_cam.lens_mult);
	depth_cam.transform({ &new_pos });
	DrawCircleV(new_pos, hitbox_radius * depth_cam.scale, hitbox_color);
}

void Petra::debug_update_movement() {
	// Debug movement conflicts with the keybindings I want for regular movement, so we need to have an indicator.
	if (IsKeyDown(KEY_A))
		pos.x -= 1;
	if (IsKeyDown(KEY_D))
		pos.x += 1;
	if (IsKeyDown(KEY_W))
		pos.y -= 1;
	if (IsKeyDown(KEY_S))
		pos.y += 1;
	float scroll = GetMouseWheelMove();
	if (scroll != 0)
		depth += scroll;
	if (IsKeyDown(KEY_UP))
		depth -= 1;
	if (IsKeyDown(KEY_DOWN))
		depth += 1;
}

void Petra::update_movement() {
}

std::string Petra::say_hello() {
	return "hii";
}
