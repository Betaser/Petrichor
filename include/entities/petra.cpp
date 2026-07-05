#include <string>
#include <print>
#include <memory>

#include "petra.hpp"
#include "../globals/mylib.hpp"
#include "../globals/game.hpp"
#include "../gamestates/level.hpp"

Petra::Petra() {
	std::println("initialized Petra");

	// Idk, seems about right?
	hitbox_radius = 12;
}

Circle Petra::get_hitbox() const {
	return {
		.pos = pos,
		.radius = hitbox_radius
	};
}

void Petra::update(Level& level, Game& game) {
	if (IsKeyPressed(KEY_LEFT_SHIFT))
		we_are_debugging = !we_are_debugging;

	if (we_are_debugging)
		debug_update_movement();
	else
		update_movement();

	collision_detection(level, game);
}

void Petra::collision_detection(Level& level, Game& game) {
	for (size_t tree_index = 0; tree_index < game.trees.size(); tree_index++) {
		const auto& tree = game.trees[tree_index];
		for (size_t config_index = 0; config_index < tree->tendril_configs.size(); config_index++) {
			const auto& config = tree->tendril_configs[config_index];
			// past us
			if (config->depth - depth < level.collision_dist) 
				continue;

			if ((config->depth - depth) / level.collision_dist > 1.2)
				continue;

			for (const auto& branch : config->branches) {
				const float dist = dist_from_pt_to_polygon(pos, branch.verts);
				if (dist < 0 || dist / hitbox_radius < 0.99) {
					collision = std::unique_ptr<Collision>(new Collision(tree_index, config_index));
					return;
				}
			}
		}
	}
	
	collision = nullptr;
}

void Petra::render(Game& game, Level& level) {
	// Show info that we are debugging
	float font_size = 30;
	auto [color, str] = we_are_debugging ? 
		std::tuple<Color, const char*>(PURPLE, "petra debug movement") :
		std::tuple<Color, const char*>(ORANGE, "petra standard movement");

	int text_length = MeasureText(str, font_size);
	DrawText(str, (game.screen_width - text_length) / 2, game.screen_height / 2 + 200, font_size, color);

	// Render her hitbox
	Color hitbox_color { 255, 255, 255, 180 };
	if (collision)
		hitbox_color = { 255, 0, 0, 100 };

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
		pos.x -= 2;
	if (IsKeyDown(KEY_D))
		pos.x += 2;
	if (IsKeyDown(KEY_W))
		pos.y -= 2;
	if (IsKeyDown(KEY_S))
		pos.y += 2;

	const float scroll = GetMouseWheelMove();
	if (scroll != 0)
		depth += scroll;
	if (IsKeyDown(KEY_UP))
		depth -= 1;
	if (IsKeyDown(KEY_DOWN))
		depth += 1;
	// depth += 0.09;
}

void Petra::update_movement() {
}

std::string Petra::say_hello() {
	return "hii";
}
