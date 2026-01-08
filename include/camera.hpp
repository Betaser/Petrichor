#ifndef CAMERA_H
#define CAMERA_H

#include <raylib.h>
#include <vector>
#include <iostream>

#include "mylib.hpp"

struct Cam {
    Vector2 pos {};
    Vector2 screen_offset {};
    float scale = 1;
    float lens_mult = 1;

    // Use a different camera for rendering at different depths.
    Cam clone() const;
    void transform(std::vector<Vector2*> vec_refs) const;

	// Let's achieve the same thing that tree.render() does 
    void draw_texture_begin_end(Shader& shader, Rectangle clip, Texture2D& texture, Rectangle src, Rectangle dest);
    void draw_texture(Rectangle clip, Texture2D& texture, Rectangle src, Rectangle dest);
};

#endif