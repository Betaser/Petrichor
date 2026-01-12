#ifndef LEVEL_H
#define LEVEL_H

#include "petra.hpp"
#include "camera.hpp"
#include "game.hpp"

struct Level {
    float collision_dist = 60;
	Petra petra;
    Cam camera;
    TextureWithCheck fog_texture;
    ShaderWithCheck fog_shader;
    ShaderWithCheck tree_foggy_blur_shader;

    Level();
    ~Level();

    void init(int screen_width, int screen_height);
    void update(Game& game);
    void render(Game& game);
    void render_trees_to_target(Game& game);
    void render_fog(Game& game);
	constexpr float dist_from_cam(Tree& tree);
};

#endif
