module;

#include <random>
#include <algorithm>

export module common;

// Method for returning random integer between min, max
export auto random_int(const int min, const int max) -> int {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution dist(min, max);
	return dist(gen);
}

// Method for converting string to lowercase
export auto lowercase(std::string str) -> std::string {
	std::ranges::transform(str, str.begin(), ::tolower);
	return str;
}

// Method for stripping away the extension from a filename string, if one is found
export auto strip_extension(const std::string &filename) -> std::string {
	return filename.find('.') != std::string::npos ? filename.substr(0, filename.find('.'))
												   : filename;
}
