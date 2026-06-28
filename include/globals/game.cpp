#include <fstream>

#include "../tree_metadata.cpp"
#include "game.hpp"

Game* Game::_game = nullptr;

Game::~Game() {}

Game::Game(const int screen_width, const int screen_height, const int fps) {
	this->screen_width = screen_width;
	this->screen_height = screen_height;	

	level.init(screen_width, screen_height);

	set_fps(fps);
}

void Game::set_fps(int fps) {
	this->fps = fps;
	SetTargetFPS(fps);
}

Game* Game::get() {
	std::println("using game.get");
	return Game::_game;
}

void Game::load_trees(const char* filepath, std::function<void(TreeMetadata&, Tree&)> accept_metadata) {
	trees.clear();
	std::string line;
	std::ifstream file;
	file.open(filepath);

	float rotation;
	Vector2 offset;
	int seed;
	float depth;

	std::string name;
	while (!file.eof()) {
		std::getline(file, line);
		const size_t separator_at = line.find(":");
		
		if (separator_at == std::string_view::npos)
			break;
		
		name = line.substr(0, separator_at);
		const auto value = line.substr(separator_at + 1);

		if (name == "rotation") {
			rotation = std::stof(value);
		}
		else if (name == "offset") {
			const size_t xy_sep = value.find(" ");
			float x = std::stof(value.substr(0, xy_sep));
			float y = std::stof(value.substr(xy_sep + 1));
			offset = { x, y };
		}
		else if (name == "seed") {
			seed = std::stoi(value);
		}
		else if (name == "depth") {
			depth = std::stof(value);

			make_tree();

			auto& tree = trees.back();
			tree->depth = depth;
			tree->rand = Rand(seed);
			tree->id = (Tree::Id) (trees.size() - 1);

			const Vector2 start_location { 100, 100 };
			auto tendrils = tree->random_tendril_config(400, 20, 1.2, 0.1, start_location);
			tree->branches = Tree::branches_from_tendrils(tendrils);
			tree->tendrils = tendrils;

			TreeMetadata metadata(rotation, offset);
			accept_metadata(metadata, *tree);

			tree->on_updated_branch();
			tree->update_texture();
		}
	}

	file.close();
}

void Game::make_tree() {
	Rand rand(69);

	// Black magic that is required to ensure trees are not created and copied, even though that would be fine.
	// auto t = std::unique_ptr<Tree>(new Tree({}, rand));
	// t->id = trees.size();
	// trees.emplace_back(std::move(t));
	
	trees.push_back(std::unique_ptr<Tree>(new Tree({}, rand)));	
	trees.back()->id = (Tree::Id) (trees.size() - 1);
}
