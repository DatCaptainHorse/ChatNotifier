#ifndef CN_SUPPORTS_MODULES_STD
module;
#include <standard.hpp>
#endif

export module common;

import standard;

// 2D and 3D position structs
export struct Position2D {
	float x = 0.0f, y = 0.0f;

	Position2D() = default;
	Position2D(const float x, const float y) : x(x), y(y) {}
};

export struct Position3D {
	float x = 0.0f, y = 0.0f, z = 0.0f;

	Position3D() = default;
	Position3D(const float x, const float y, const float z) : x(x), y(y), z(z) {}
};

// Method for returning random integer between min, max
export auto random_int(const int min, const int max) -> int {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution dist(min, max);
	return dist(gen);
}

// Method for converting string to lowercase
export auto lowercase(std::string str) -> std::string {
	std::ranges::transform(str, str.begin(), [](const auto &ch) { return std::tolower(ch); });
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
	if (endPos == std::string::npos) return str;
	return str.substr(pos, endPos - pos);
}

// Method for getting string after a delimiter, with optional starting position
export auto get_string_after(const std::string &str, const std::string &delim, const size_t pos = 0)
	-> std::string {
	const auto startPos = str.find(delim, pos);
	if (startPos == std::string::npos) return "";
	return str.substr(startPos + delim.size());
}

// Method for getting every instance of a string between two delimiters
export auto get_strings_between(const std::string &str, const std::string &start,
								const std::string &end) -> std::vector<std::string> {
	std::vector<std::string> strings;
	auto startPos = str.find(start);
	while (startPos != std::string::npos) {
		const auto endPos = str.find(end, startPos + start.size());
		if (endPos == std::string::npos) break;

		strings.push_back(str.substr(startPos + start.size(), endPos - startPos - start.size()));
		startPos = str.find(start, endPos);
	}
	return strings;
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
	return std::ranges::all_of(str, [](const auto &ch) { return std::isalpha(ch); });
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

// Template method for converting string to integer
export template <typename T>
requires std::is_integral_v<T>
constexpr auto integral_from_string(const std::string &str) -> T {
	if constexpr (std::same_as<T, std::int8_t>)
		return std::stoi(str);
	else if constexpr (std::same_as<T, std::int16_t>)
		return std::stoi(str);
	else if constexpr (std::same_as<T, std::int32_t>)
		return std::stoi(str);
	else if constexpr (std::same_as<T, std::int64_t>)
		return std::stoll(str);
	else if constexpr (std::same_as<T, std::uint8_t>)
		return std::stoul(str);
	else if constexpr (std::same_as<T, std::uint16_t>)
		return std::stoul(str);
	else if constexpr (std::same_as<T, std::uint32_t>)
		return std::stoul(str);
	else if constexpr (std::same_as<T, std::uint64_t>)
		return std::stoull(str);
	else
		return std::stoi(str);
}

// Tempalte method for converting string to floating point
export template <typename T>
requires std::is_floating_point_v<T>
constexpr auto floating_from_string(const std::string &str) -> T {
	if constexpr (std::same_as<T, float>)
		return std::stof(str);
	else if constexpr (std::same_as<T, double>)
		return std::stod(str);
	else if constexpr (std::same_as<T, long double>)
		return std::stold(str);
	else
		return std::stof(str);
}

// Stringable concept for checking if a type is convertible to a string
export template <typename T>
concept Stringable =
	std::same_as<T, bool> || std::is_integral_v<T> || std::is_floating_point_v<T> ||
	std::same_as<T, std::string> || std::is_integral_v<std::underlying_type_t<T>> ||
	std::is_floating_point_v<std::underlying_type_t<T>>;

// Template method for converting a string to a specific type
// takes care of calling proper conversion
export template <typename T>
requires Stringable<T>
constexpr auto t_from_string(const std::string &str) -> T {
	if constexpr (std::same_as<T, bool>)
		return str == "true";
	else if constexpr (std::same_as<T, std::string>)
		return std::string(str);
	else if constexpr (std::is_floating_point_v<T>)
		return floating_from_string<T>(str);
	else if constexpr (std::is_integral_v<T>)
		return integral_from_string<T>(str);
	else if constexpr (std::is_integral_v<std::underlying_type_t<T>>)
		return static_cast<T>(integral_from_string<std::underlying_type_t<T>>(str));
	else if constexpr (std::is_floating_point_v<std::underlying_type_t<T>>)
		return static_cast<T>(floating_from_string<std::underlying_type_t<T>>(str));
	else // Assume integral type
		return static_cast<T>(integral_from_string<std::underlying_type_t<T>>(str));
}

// Template method for converting a specific type to a string
// takes care of calling proper conversion
export template <typename T>
requires Stringable<T>
constexpr auto t_to_string(const T &value) -> std::string {
	if constexpr (std::same_as<T, bool>)
		return value ? "true" : "false";
	else if constexpr (std::same_as<T, std::string>)
		return value;
	else if constexpr (std::is_floating_point_v<T>)
		return std::to_string(value);
	else if constexpr (std::is_integral_v<T>)
		return std::to_string(value);
	else if constexpr (std::is_integral_v<std::underlying_type_t<T>>)
		return std::to_string(static_cast<std::underlying_type_t<T>>(value));
	else if constexpr (std::is_floating_point_v<std::underlying_type_t<T>>)
		return std::to_string(static_cast<std::underlying_type_t<T>>(value));
	else
		return std::to_string(value);
}

// Concepts to make enum classes work as bitmasks
export template <typename T>
requires(std::is_enum_v<T> and requires(T e) { enable_bitmask_operators(e); })
constexpr auto operator|(const T lhs, const T rhs) -> T {
	return static_cast<T>(std::to_underlying(lhs) | std::to_underlying(rhs));
}
export template <typename T>
requires(std::is_enum_v<T> and requires(T e) { enable_bitmask_operators(e); })
constexpr auto operator&(const T lhs, const T rhs) -> bool {
	return static_cast<bool>(std::to_underlying(lhs) & std::to_underlying(rhs));
}

// Non-member variants
export template <typename T>
requires(std::is_enum_v<T> and requires(T e) { enable_bitmask_operators(e); })
constexpr auto operator|=(T &lhs, const T rhs) -> T {
	lhs = static_cast<T>(std::to_underlying(lhs) | std::to_underlying(rhs));
	return lhs;
}
export template <typename T>
requires(std::is_enum_v<T> and requires(T e) { enable_bitmask_operators(e); })
constexpr auto operator&=(T &lhs, const T rhs) -> bool {
	lhs = static_cast<bool>(std::to_underlying(lhs) & std::to_underlying(rhs));
	return lhs;
}

// operator* for getting the underlying type of an enum class
export template <typename T>
requires(std::is_enum_v<T> and requires(T e) { enable_bitmask_operators(e); })
constexpr auto operator*(const T e) -> std::underlying_type_t<T> {
	return std::to_underlying(e);
}