#ifndef LEVEL_H
#define LEVEL_H

#include <map>

#include "petra.hpp"
#include "camera.hpp"
#include "game.hpp"

struct Dome {
    Vector2 pos;
    float max_radius;
    std::function<float(float)> depth_to_radius_fn;

    void flatten_tree(Tree& tree, const Circle& circle, std::map<std::string, std::string>& debug) const;
};

struct Level {
    // 60
    float collision_dist = 60;
	Petra petra;
    Cam camera;
    TextureWithCheck fog_texture;
    ShaderWithCheck fog_shader;
    ShaderWithCheck tree_foggy_blur_shader;
    Dome dome;

    // Debugging
    static bool debug_apply_rotation;
	std::map<std::string, std::string> debug;

    Level();
    ~Level();

    void init(int screen_width, int screen_height);
    void update(Game& game);
    void tree_interp_rigid(Tree& tree);
    void render(Game& game);
    void render_trees_to_target(Game& game);
    void render_fog(Game& game);
	constexpr float dist_from_cam(const Tree& tree) const;
    std::vector<std::tuple<size_t, float>> calc_dome_radii(const Dome& dome, std::vector<std::unique_ptr<Tree>>& trees) const;
    void debug_render_dome_radii(Game& game) const;
    Cam calc_depth_cam(float dist) const;
};

bool Level::debug_apply_rotation = false;

#endif
