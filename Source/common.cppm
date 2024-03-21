module;

#include <tuple>
#include <vector>
#include <ranges>
#include <string>
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

// Method for splitting string by delimiter
export auto split_string(std::string_view str, std::string_view delimeter)
	-> std::vector<std::string> {
	return std::ranges::to<std::vector<std::string>>(
		str | std::views::split(delimeter) |
		std::views::transform([](auto &&part) { return std::string(part.begin(), part.end()); }));
}

// Method for returning a GUID string
export auto generate_guid() -> std::string {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dis(0, 15);
	const std::string hex = "0123456789abcdef";
	std::string uuid = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
	for (auto &ch : uuid) {
		if (ch == 'x') {
			ch = hex[dis(gen)];
		} else if (ch == 'y') {
			ch = hex[(dis(gen) & 0x3) | 0x8];
		}
	}
	return uuid;
}

// Simple struct for returning an result code and message with arguments
export struct Result {
	int code = 0;
	std::string message = "";
	std::tuple<std::string> args = {};

	explicit operator bool() const { return code == 0; }
};
