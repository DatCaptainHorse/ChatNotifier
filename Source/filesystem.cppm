module;

#include <string>
#include <fstream>
#include <filesystem>

export module filesystem;

import common;

export class Filesystem {
	static inline std::filesystem::path root_path;

public:
	static auto initialize(const int argc, char **argv) -> Result {
		if (argc < 1) return Result(1, "Invalid argc provided");

		root_path = argv[0];
		root_path = root_path.parent_path();

		return Result();
	}

	static auto get_root_path() -> std::filesystem::path {
		return root_path;
	}

	// Method for loading contents of a file as a string
	static auto load_file_string(const std::filesystem::path &path) -> std::string {
		if (!std::filesystem::exists(path)) return {};

		std::ifstream file(path);
		std::string contents;
		file.seekg(0, std::ios::end);
		contents.resize(file.tellg());
		file.seekg(0, std::ios::beg);
		file.read(contents.data(), contents.size());
		file.close();

		return contents;
	}
};
