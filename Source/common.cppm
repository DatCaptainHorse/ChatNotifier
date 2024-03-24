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

// Method for trimming string of special characters (\n, \r, \t, space..)
export auto trim_string(std::string str) -> std::string {
	std::erase_if(str, [](const auto &ch) {
		return ch == '\n' || ch == '\r' || ch == '\t' || std::isspace(ch);
	});
	return str;
}

// Method for trimming string, but with a custom string to trim
export auto trim_string(std::string str, const std::string &trim) -> std::string {
	std::erase_if(str, [&trim](const auto &ch) { return trim.find(ch) != std::string::npos; });
	return str;
}

// Method for getting string between two delimiters, with optional starting position
export auto get_string_between(const std::string &str, const std::string &start,
							   const std::string &end, const size_t pos = 0) -> std::string {
	const auto startPos = str.find(start, pos);
	if (startPos == std::string::npos) return "";

	const auto endPos = str.find(end, startPos + start.size());
	if (endPos == std::string::npos) return "";

	return str.substr(startPos + start.size(), endPos - startPos - start.size());
}

// Method for getting string until a delimiter, with optional starting position
export auto get_string_until(const std::string &str, const std::string &delim, const size_t pos = 0)
	-> std::string {
	const auto endPos = str.find(delim, pos);
	if (endPos == std::string::npos) return "";

	return str.substr(pos, endPos - pos);
}

// Method for getting invidual letters of a string for multi-byte characters
export auto get_letters_mb(const std::string &str) -> std::vector<std::string> {
	std::vector<std::string> letters;
	const auto *start = str.c_str();
	const auto *end = start + str.size();
	while (start < end) {
		const auto bytes = std::mblen(start, end - start);
		if (bytes < 1) break;
		letters.emplace_back(start, start + bytes);
		start += bytes;
	}
	return letters;
}

// Returns whether string has letters only
export auto is_letters(const std::string &str) -> bool {
	return std::ranges::all_of(str, ::isalpha);
}

// Remaps a value from one range to another
export auto remap_value(const float value, const float inMin, const float inMax, const float outMin,
						const float outMax) -> float {
	return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
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
