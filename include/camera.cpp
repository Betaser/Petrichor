#include "camera.hpp"

Cam Cam::clone() const {
    return {
        .pos = pos,
        .scale = scale
    };
}

void Cam::draw_texture(Shader& shader, Rectangle clip, Texture2D& texture, Rectangle src, Rectangle dest) {
	BeginShaderMode(shader);
	draw_texture(
		clip,
		texture,
		src,
		dest);
	EndShaderMode();
}

void Cam::draw_texture(Rectangle clip, Texture2D& texture, Rectangle src, Rectangle dest) {
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
	// Step 4: Figure out when there is a full clip and then return early
	if (!is_overlap(clip, dest))
		return;

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