#ifndef LEVEL_H
#define LEVEL_H

#include <functional>
#include <map>
#include <raylib.h>

struct Tree;
struct TendrilConfig;

#include "../entities/petra.hpp"
#include "../scene_elements/camera.hpp"

struct Dome {
    Vector2 pos;
    float max_radius;
    std::function<float(float)> depth_to_radius_fn;

	void flatten_tendril_config(TendrilConfig& config, const Circle& circle, std::map<std::string, std::string>& debug) const;
};

struct Level {
    private:
    void manage_debug_rotate_state();
    void manage_debug_spring_state();
	void move_nearby_tendril_config(TendrilConfig& config, float boundary_dist);
	void tendril_config_interp_rigid(TendrilConfig& config);
    void debug_render_dome_radii(Game& game) const;
	std::vector<Vector2> calc_rel_dirs(TendrilConfig& config);
	void calc_twist(TendrilConfig& config);
	void follow_petra_with_cam(Game& game);

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
	void push_tendril_config_aside(TendrilConfig& config, float cam_dist);
    void render(Game& game);
    void render_trees_to_target(Game& game);
    void render_fog(Game& game);
	constexpr float dist_from_cam(const TendrilConfig& config) const;
    std::vector<std::tuple<size_t, float>> calc_dome_radii(const Dome& dome, std::vector<std::unique_ptr<Tree>>& trees) const;
    Cam calc_depth_cam(float dist) const;
};

#endif
