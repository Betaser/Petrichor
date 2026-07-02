#ifndef TREE_H
#define TREE_H

#include <raylib.h>

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

struct TrunkFace {
	float depth = 0;
	Vector2 position {};
	float radius = 10;
};

struct TendrilRenderData {
	float finishing_alpha;
	Vector4 rgb_tint;
};

struct TendrilConfig {
	public:
	enum class Id : size_t {};

	static const int MAX = 100;

	TextureWithCheck blank_tex;
	ShaderWithCheck branch_shader;
	Vector2 texture_pos {};
	Vector2 small {};
	Vector2 big {};

	void send_vals_to_branch_shader();
	void level_editor_render(const TendrilRenderData& data);
	// Does not figure out how we want to render it.
	std::vector<std::vector<Branch>> gen_structured_branches(float total_length, float start_thickness, float start_rotation, float thickness_cutoff, Vector2 start_location, int MAX_TENDRILS = 5);

	~TendrilConfig();

	private:
	// Controls the horz zoom of texels, bigger = more zoomed in
	const float MAX_WIDTH = 300;
	const float MAX_HEIGHT = MAX_WIDTH;

	Vector2 compressed_branches[4][MAX] {};
	Vector2 btm_lefts[MAX] {};
	Vector2 top_rights[MAX] {};
	// Default to this resolution, it might not matter what this really is.
	const Vector2I blank_tex_dims { 10, 10 };
	Rand rand;
	Id id = (Id) 0;
	float depth = 0;
	TextureWithCheck sample_tex;
	// Contains same branches as in structured_branches
	std::vector<Branch> branches;
	std::vector<std::vector<Branch>> structured_branches;
	// Needed to simulate branch resistance to dome "bending"
	std::vector<Branch> original_branches;
	// Needed to simulate branch "twisting"
	std::vector<Vector2> prev_rel_dirs;
	std::vector<float> branch_twists;

	TendrilConfig(Id id, const Rand& rand);

	void init_gfx(ShaderWithCheck branch_shader);
	void init_texture();
	void unload_textures();
	void bounding_box(Vector2& small, Vector2& big);
	void on_updated_branch();
	void update_texture();
	constexpr Vector2 origin() const;
	bool past_me(const Petra& petra, const float epsilon = 0) const;
};

struct Tree {
	// Default to this resolution, it might not matter what this really is.
	const Vector2I blank_tex_dims { 10, 10 };

	TextureWithCheck blank_tex;
	ShaderWithCheck trunk_shader;
	RenderTexture2D target;

	public:
	std::vector<TendrilConfig> tendril_configs;

	// Current idea for trunk
	std::vector<TrunkFace> trunk_faces;

	static TextureWithCheck branch_sampling_tex;
	static void dup_branches(const std::vector<Branch>& from, std::vector<Branch>& to);

	Tree(ShaderWithCheck trunk_shader);
	~Tree();

	void level_editor_render(const std::vector<TendrilRenderData>& tendrils_data);
	void render_to_target();

	static std::vector<Branch> branches_from_tendrils(std::vector<std::vector<Branch>> tendril_config);
};

#endif
