#ifndef TREE_H
#define TREE_H

#include <raylib.h>

struct Branch;
struct Game;
struct LevelEditor;
struct Petra;

#include "../globals/mylib.hpp"

struct Branch {
	std::vector<Vector2> verts;
	std::vector<unsigned int> nexts;

	Branch(std::vector<Vector2> verts);

	Vector2 forward() const;
	Vector2 front() const;
	Vector2 back() const;
	float front_thickness() const;
	float back_thickness() const;
	Branch clone() const;
};

struct TrunkLayer {
	float depth = 0;
	Vector2 position {};
	float radius = 0;
};

// I'm thinking of rendering the top segment differently, but it's not a pressing matter.
struct TrunkSegment {
	const TrunkLayer top;
	const TrunkLayer bottom;
};

struct TrunkFace {
	float depth = 0;
	Vector2 position {};
	float radius = 10;
};

struct TreeRenderData {
	float finishing_alpha;
	Vector4 rgb_tint;
};

struct Tree {
	private:
	static const int MAX = 100;
	// Controls the horz zoom of texels, bigger = more zoomed in
	const float MAX_WIDTH = 300;
	const float MAX_HEIGHT = MAX_WIDTH;

	Vector2 compressed_branches[4][MAX] {};
	Vector2 btm_lefts[MAX] {};
	Vector2 top_rights[MAX] {};
	// Default to this resolution, it might not matter what this really is.
	const Vector2I blank_tex_dims { 10, 10 };

	void init_texture();
	void unload_textures();

	public:
	float depth = 0;
	Vector2 small {};
	Vector2 big {};
	size_t id = 0;
	ShaderWithCheck tendril_shader;
	ShaderWithCheck trunk_shader;
	Rand rand;
	RenderTexture2D target;
	// Contains same branches as in tendrils
	std::vector<Branch> branches;
	// Needed to simulate branch resistance to dome "bending"
	std::vector<Branch> original_branches;
	// Needed to simulate branch "twisting"
	std::vector<Vector2> prev_rel_dirs;
	std::vector<float> branch_twists;
	Vector2 texture_pos {};
	std::vector<std::vector<Branch>> tendrils;
	std::vector<TrunkSegment> trunk_segments;

	// Current idea for trunk
	std::vector<TrunkFace> trunk_faces;

	// Hold onto tree_tex just to unload it.
	TextureWithCheck blank_tex, tree_tex;

	static TextureWithCheck static_tree_tex;
	static void dup_branches(const std::vector<Branch>& from, std::vector<Branch>& to);

	Tree(std::vector<Branch> branches, Rand& rand);
	~Tree();

	void bounding_box(Vector2& small, Vector2& big);
	void init(std::vector<Branch> branches, ShaderWithCheck tendril_shader, ShaderWithCheck trunk_shader, Rand& rand);
	void on_updated_branch();
	void update_texture();
	void send_vals_to_tendril_shader();
	void level_editor_render(const TreeRenderData& data);
	void render_to_target();

	constexpr Vector2 origin() const;

	static std::vector<Branch> branches_from_tendrils(std::vector<std::vector<Branch>> tendrils);

	// Does not figure out how we want to render it.
	std::vector<std::vector<Branch>> random_tendril_config(float total_length, float start_thickness, float start_rotation, float thickness_cutoff, Vector2 start_location, int MAX_TENDRILS = 5);
	bool past_me(const Petra& petra, const float epsilon = 0) const;
};

TextureWithCheck Tree::static_tree_tex;

#endif
