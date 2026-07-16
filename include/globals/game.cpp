#include <fstream>
#include <print>

#include "../tree_metadata.cpp"
#include "game.hpp"

Game* Game::_game = nullptr;

Game::~Game() {}

Game::Game(const int screen_width, const int screen_height, const int fps) {
	this->screen_width = screen_width;
	this->screen_height = screen_height;	
	overall_time = 0;

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

void Game::load_trees(const char* filepath, std::function<void(BranchMetadata&, TendrilConfig*, size_t)> accept_metadata) {
	trees.clear();
	std::string line;
	std::ifstream file;
	file.open(filepath);

	float rotation;
	Vector2 offset;
	int seed;
	float depth;
	int tree_id;
	size_t num_tendril_configs = 0;

	std::string name;
	while (!file.eof()) {
		std::getline(file, line);
		const size_t separator_at = line.find(":");
		
		if (separator_at == std::string_view::npos)
			break;
		
		name = line.substr(0, separator_at);
		std::println("name: {}", name);
		const auto value = line.substr(separator_at + 1);

		if (name == "tree") {
			tree_id = std::stoi(value);
			(void) tree_id;
			make_tree();
		}
		else if (name == "rotation") {
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
			// This is the start of a config.
			depth = std::stof(value);

			auto curr_tree = trees.back().get();
			curr_tree->tendril_configs.push_back(
				std::make_unique<TendrilConfig>(
					(TendrilConfig::Id) num_tendril_configs++, 
					Rand(seed),
					curr_tree));
			auto config = curr_tree->tendril_configs.back().get();
			config->depth = depth;

			const Vector2 start_location { 100, 100 };
			auto structured_branches = config->gen_structured_branches(400, 20, 1.2, 0.1, start_location);
			config->branches = Tree::branches_from_structured_branches(structured_branches);
			config->structured_branches = structured_branches;

			BranchMetadata metadata(offset, rotation);
			accept_metadata(metadata, config, trees.size() - 1);

			config->on_updated_branch();
			config->update_texture();
		}
	}

	file.close();
}

void Game::make_tree() {
	trees.push_back(std::make_unique<Tree>());	
}
