#include "camera.hpp"

Cam Cam::clone() const {
    return {
        .pos = pos,
        .scale = scale
    };
}

void Cam::draw_texture(Shader& shader, int screen_width, int screen_height, Texture2D& texture, Rectangle src, Rectangle dest) {
	Vector2 dims { 500, 300 };
	Rectangle clip {
		.x = ((float) screen_width - dims.x) / 2,
		.y = ((float) screen_height - dims.y) / 2,
		.width = dims.x,
		.height = dims.y
	};
	// Step 0: Overlay a gray color over everything to separate previous tree renders from current.
	// DrawRectangle(0, 0, screen_width, screen_height, { 0, 0, 0, 200 });
	// Step 1: Render out some transparent overlay to show where the clip is
	// DrawRectangleRec(clip, { 255, 0, 0, 100 });

	BeginShaderMode(shader);
	draw_texture(
		clip,
		texture,
		src,
		dest);
	EndShaderMode();
}

void Cam::draw_texture(Rectangle clip, Texture2D& texture, Rectangle src, Rectangle dest) {
	(void) clip;
	// Step 2: Call this function such that a tree's render call is mimiced
	/*
	DrawTexturePro(
		texture,
		src,
		dest,
		{},
		0,
		WHITE);
	*/
	// Step 3: Call this function with some clipping
	// For now, assume there is in fact overlap.
	auto rect_overlap = [](Rectangle r1, Rectangle r2) -> Rectangle {
		float left = std::max(r1.x, r2.x);
		float right = std::min(r1.x + r1.width, r2.x + r2.width);
		float btm = std::max(r1.y, r2.y);
		float top = std::min(r1.y + r1.height, r2.y + r2.height);
		return {
			.x = left,
			.y = btm,
			.width = right - left,
			.height = top - btm
		};
	};
	Rectangle clipped_dest = rect_overlap(clip, dest);
	Vector2 clipped_src_pos(
		((clipped_dest.x - dest.x) / dest.width) * src.width, 
		((clipped_dest.y - dest.y) / dest.height) * src.height);
	Rectangle clipped_src {
		.x = clipped_src_pos.x,
		.y = clipped_src_pos.y,
		.width = (clipped_dest.width / dest.width) * src.width,
		.height = (clipped_dest.height / dest.height) * src.height
	};
	// std::cout << "clipped_src_pos " << to_str(clipped_src_pos, 2) << "\n";
	// std::cout << "clipped_src dims " << to_str({ clipped_src.width, clipped_src.height }, 2) << "\n";

	// std::cout << "src dims " << to_str({ src.width, src.height }, 2) << "\n";

	DrawTexturePro(
		texture,
		clipped_src,
		clipped_dest,
		{},
		0,
		WHITE);
}

void Cam::transform(std::vector<Vector2*>& vec_refs) const {
    // Does this work? I'm actually not sure yet.
	for (auto& vec : vec_refs) {
		// blah
		vec->x += 1;
		vec->y += 1;
	}
}