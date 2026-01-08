#ifndef LEVEL_H
#define LEVEL_H

#include "petra.hpp"
#include "camera.hpp"
#include "game.hpp"

struct Level {
	Petra petra;
    Cam camera;
    TextureWithCheck fog_texture;
    ShaderWithCheck fog_shader;
    ShaderWithCheck tree_foggy_blur_shader;
    RenderTexture2D trees_target;

    Level();
    ~Level();

    void init(int screen_width, int screen_height);
    void update(Game& game);
    void render(Game& game);
    void render_tree_to_target(Game& game);
    void render_fog(Game& game);
};

#endif