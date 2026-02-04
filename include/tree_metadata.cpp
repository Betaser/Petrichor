#ifndef TREE_META_H
#define TREE_META_H

#include <vector>
#include "tree.hpp"

struct TreeMetadata {
	// Rotation affects all tendrils
	std::vector<Branch> branches;
	Vector2 offset {};
	float rotation = 0;
	Rectangle mark {};

	static TreeMetadata zero() {
		return {};
	}
	TreeMetadata(float rotation, Vector2 offset, Tree& tree, Rectangle mark) {
		std::cout << "init tree metadata\n";
		this->rotation = rotation;
		this->offset = offset;
		this->mark = mark;

		branches.reserve(tree.branches.size());
		for (const auto& branch : tree.branches)
			branches.emplace_back(branch);
	}
	~TreeMetadata() {
		std::cout << "deinit tree metadata\n";
	}

	private:
	TreeMetadata() {}
};

#endif