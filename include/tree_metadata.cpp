#ifndef TREE_META_H
#define TREE_META_H

#include <raylib.h>

// TODO:
// Either this should be BRANCH metadata and we adjust what things metadata applies to
// Or this has a vector for every branch of struct { offset, rotation }
// Option 2 seems less breaking. But ALSO, no matter which option, the save+reload has to associate { offset, rotation } with each branch
// And that means each branch needs its own seed value; which probably becomes a vector on metadata.
// Which means a lot of functions inside game.load_trees has to change too.
struct BranchMetadata {
	Vector2 offset {};
	float rotation = 0;
	BranchMetadata() {}
	BranchMetadata(Vector2 offset, float rotation) {
		this->offset = offset;
		this->rotation = rotation;
	}
};

/*
struct TreeMetadata {
	std::vector<BranchMetadata> each_branch;

	TreeMetadata(const std::vector<BranchMetadata>& each_branch) {
		std::println("init tree metadata");
		this->each_branch = each_branch;
	}

	~TreeMetadata() {
		std::println("deinit tree metadata");
	}

	TreeMetadata() = delete;
};
*/

/*
struct TreeMetadata {
	Vector2 offset {};
	float rotation = 0;

	TreeMetadata(float rotation, Vector2 offset) {
		std::println("init tree metadata");
		this->rotation = rotation;
		this->offset = offset;
	}

	~TreeMetadata() {
		std::println("deinit tree metadata");
	}

	static TreeMetadata zero() {
		return {};
	}

	private:
	TreeMetadata() {}
};
*/

#endif
