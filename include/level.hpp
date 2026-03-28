#ifndef LEVEL_H
#define LEVEL_H

#include "petra.hpp"
#include "camera.hpp"
#include "game.hpp"

struct Dome {
    Vector2 pos;
    float max_radius;
    std::function<float(float)> depth_to_radius_fn;

    void flatten_tree(Tree& tree) const;
};

struct Level {
    float collision_dist = 60;
	Petra petra;
    Cam camera;
    TextureWithCheck fog_texture;
    ShaderWithCheck fog_shader;
    ShaderWithCheck tree_foggy_blur_shader;
    Dome dome;

    Level();
    ~Level();

    void init(int screen_width, int screen_height);
    void update(Game& game);
    void render(Game& game);
    void render_trees_to_target(Game& game);
    void render_fog(Game& game);
	constexpr float dist_from_cam(const Tree& tree) const;
    std::vector<std::tuple<size_t, float>> calc_dome_radii(const Dome& dome, std::vector<std::unique_ptr<Tree>>& trees) const;
    void debug_render_dome_radii(Game& game) const;
    Cam calc_depth_cam(float dist) const;
};

#endif
