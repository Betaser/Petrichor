#ifndef TREE_H
#define TREE_H

#include "mylib.hpp"
#include <array>
#include <vector>
#include "raylib.h"
#include <random>

struct Branch;
using Tendrils = std::vector<std::vector<std::vector<Branch>>>;

struct Branch {
	std::vector<Vector2> verts;

	Branch(std::vector<Vector2> verts);

	Vector2 forward() const;
	Vector2 front() const;
	Vector2 back() const;
	float front_thickness() const;
	float back_thickness() const;
	Branch clone() const;
};

// TODO: make branches
struct Tree {
	private:
	static const int MAX = 100;
	const float MAX_WIDTH = 300;
	const float MAX_HEIGHT = MAX_WIDTH;

	// std::array<std::array<Vector2, 4>, MAX> compressed_branches;
	Vector2 compressed_branches[4][MAX] {};
	Vector2 btm_lefts[MAX] {};
	Vector2 top_rights[MAX] {};

	public:
	float depth = 0;
	size_t id = 0;
	Shader shader;
	Rand rand;
	// Contains same branches as in tendrils
	std::vector<Branch> branches;
	Vector2I texture_pos {};
	Tendrils tendrils;
	// Hold onto tree_tex just to unload it.
	Texture2D blank_tex, tree_tex;

	Tree();
	// Tree(const Tree&) = delete;
	// Tree& operator=(const Tree&) = delete;
	Tree(std::vector<Branch> branches, Shader& shader, Rand& rand);
	~Tree();

	void bounding_box(Vector2& small, Vector2& big);
	void init(std::vector<Branch> branches, Shader& shader, Rand& rand);
	void unload_textures();
	void init_texture();
	void render();

	static std::vector<Branch> branches_from_tendrils(Tendrils tendrils);

	// Does not figure out how we want to render it.
	std::vector<std::vector<Branch>> random_tendril_config(float total_length, float start_thickness, float start_rotation, float thickness_cutoff, Vector2 start_location, int MAX_TENDRILS = 5);
};

#endif