#ifndef TREE_H
#define TREE_H

#include <memory>
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

struct Tree;

struct TendrilConfig {
	public:
	enum class Id : size_t {};

	static const int MAX = 100;

	TextureWithCheck blank_tex;
	ShaderWithCheck branch_shader;
	Vector2 texture_pos {};
	Vector2 small {};
	Vector2 big {};
	std::vector<Branch> branches;
	std::vector<std::vector<Branch>> structured_branches;
	// Needed to simulate branch resistance to dome "bending"
	std::vector<Branch> original_branches;
	// Needed to simulate branch "twisting"
	std::vector<Vector2> prev_rel_dirs;
	std::vector<float> branch_twists;
	float depth = 0;
	RenderTexture2D target;
	Id id = (Id) 0;
	Rand rand;
	Tree* tree_owner = nullptr;

	// Needs a copy constructor to fulfill construct_at req for unique_ptrness?
	// TendrilConfig(const TendrilConfig& other) = default;
	TendrilConfig(Id id, const Rand& rand, Tree* tree_owner);
	~TendrilConfig();

	void send_vals_to_branch_shader();
	void level_editor_render(const TendrilRenderData& data);
	// Does not figure out how we want to render it.
	std::vector<std::vector<Branch>> gen_structured_branches(float total_length, float start_thickness, float start_rotation, float thickness_cutoff, Vector2 start_location, int MAX_TENDRILS = 5);
	Vector2 origin() const;
	void update_texture();
	bool past_me(const Petra& petra, const float epsilon = 0) const;
	void render_to_target(const Petra& petra);
	void on_updated_branch();
	void bounding_box(Vector2& small, Vector2& big);

	private:
	// Controls the horz zoom of texels, bigger = more zoomed in
	const float MAX_WIDTH = 300;
	const float MAX_HEIGHT = MAX_WIDTH;

	Vector2 compressed_branches[4][MAX] {};
	Vector2 btm_lefts[MAX] {};
	Vector2 top_rights[MAX] {};
	// Default to this resolution, it might not matter what this really is.
	const Vector2I blank_tex_dims { 10, 10 };
	TextureWithCheck sample_tex;
	// Contains same branches as in structured_branches

	void init_gfx(ShaderWithCheck branch_shader);
	void init_texture();
	void unload_textures();
};

struct Tree {
	// Default to this resolution, it might not matter what this really is.
	const Vector2I blank_tex_dims { 10, 10 };

	TextureWithCheck blank_tex;
	ShaderWithCheck trunk_shader;

	public:
	std::vector<std::unique_ptr<TendrilConfig>> tendril_configs;

	// Current idea for trunk
	std::vector<TrunkFace> trunk_faces;

	static TextureWithCheck branch_sampling_tex;
	static void dup_branches(const std::vector<Branch>& from, std::vector<Branch>& to);

	Tree();
	~Tree();

	void level_editor_render(const std::vector<TendrilRenderData>& tendrils_data);

	static std::vector<Branch> branches_from_structured_branches(std::vector<std::vector<Branch>> structured_branches);
};

#endif
