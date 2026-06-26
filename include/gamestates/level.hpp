#ifndef LEVEL_H
#define LEVEL_H

#include <functional>
#include <map>
#include <raylib.h>

struct Tree;

#include "../entities/petra.hpp"
#include "../scene_elements/camera.hpp"

struct Dome {
    Vector2 pos;
    float max_radius;
    std::function<float(float)> depth_to_radius_fn;

    void flatten_tree(Tree& tree, const Circle& circle, std::map<std::string, std::string>& debug) const;
};

struct Level {
    private:
    void manage_debug_rotate_state();
    void manage_debug_spring_state();
    void move_tree_thats_too_close(Tree& tree, const float boundary_dist);
    void tree_interp_rigid(Tree& tree);
    void debug_render_dome_radii(Game& game) const;
    std::vector<Vector2> calc_rel_dirs(Tree& tree);
    void calc_twist(Tree& tree);

    public:
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
    void push_trees_aside(Tree& tree, const float cam_dist);
    void render(Game& game);
    void render_trees_to_target(Game& game);
    void render_fog(Game& game);
	constexpr float dist_from_cam(const Tree& tree) const;
    std::vector<std::tuple<size_t, float>> calc_dome_radii(const Dome& dome, std::vector<std::unique_ptr<Tree>>& trees) const;
    Cam calc_depth_cam(float dist) const;
};

#endif
