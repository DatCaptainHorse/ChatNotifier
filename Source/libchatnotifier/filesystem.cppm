#ifndef CN_SUPPORTS_MODULES_STD
module;
#include <standard.hpp>
#endif

export module filesystem;

import standard;
import common;

export class Filesystem {
	static inline std::filesystem::path root_path;

public:
	static auto initialize(const int argc, char **argv) -> Result {
		if (argc < 1) return Result(1, "Invalid argc provided");

		root_path = argv[0];
		//root_path = root_path.parent_path();

		return Result();
	}

	static auto get_root_path() -> std::filesystem::path { return root_path; }
};
