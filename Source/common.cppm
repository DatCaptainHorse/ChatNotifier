module;

// No matter what IDE says, don't remove headers, thanks MSVC
#include <map>
#include <array>
#include <print>
#include <tuple>
#include <vector>
#include <ranges>
#include <string>
#include <random>
#include <chrono>
#include <utility>
#include <numeric>
#include <algorithm>
#include <type_traits>

export module common;

// 2D and 3D position structs
export struct Position2D {
	float x = 0.0f, y = 0.0f;

	Position2D() = default;
	Position2D(float x, float y) : x(x), y(y) {}
};

export struct Position3D {
	float x = 0.0f, y = 0.0f, z = 0.0f;

	Position3D() = default;
	Position3D(float x, float y, float z) : x(x), y(y), z(z) {}
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

// Struct for Twitch message data
// TODO: Move to general module
export struct TwitchChatMessage {
	std::string user, message, command;
	std::chrono::time_point<std::chrono::steady_clock> time;
	std::map<std::string, std::vector<std::string>> args;

	TwitchChatMessage(std::string user, std::string message)
		: user(std::move(user)), message(std::move(message)),
		  time(std::chrono::steady_clock::now()) {
		args = get_command_args()[0];
		command = get_command();
	}

	TwitchChatMessage(std::string user, std::string message, std::string command,
					  std::chrono::time_point<std::chrono::steady_clock> time,
					  std::map<std::string, std::vector<std::string>> groupArgs)
		: user(std::move(user)), message(std::move(message)), command(std::move(command)),
		  time(time), args(std::move(groupArgs)) {}

	// Commands can be given arguments like so
	// "!cmd <arg1=value1,arg2=value2> message <arg3=value3|value4|value5> another message" etc.
	// @return map of arg/values groups (each <> pair is a group)
	// to another map of arg to it's values
	[[nodiscard]] auto get_command_args() const
		-> std::map<std::uint32_t, std::map<std::string, std::vector<std::string>>> {
		// Get each argument group
		auto result = std::map<std::uint32_t, std::map<std::string, std::vector<std::string>>>{};
		const auto argGroups = get_strings_between(message, "<", ">");
		for (std::uint32_t i = 0; i < argGroups.size(); ++i) {
			for (const auto argList = split_string(argGroups[i], ","); const auto &arg : argList) {
				const auto splitted = split_string(arg, "=");
				if (splitted.size() != 2) continue;
				if (const auto vals = split_string(splitted[1], "|"); !vals.empty())
					result[i][splitted[0]] = vals;
				else
					result[i][splitted[0]].emplace_back(splitted[1]);
			}
		}
		return result;
	}

	// A nicer way of getting command arguments
	// @return optional value of the argument within specified group
	template <typename T>
	auto get_command_arg(const std::string &argName) const -> std::optional<T> {
		if (!is_command()) return std::nullopt;
		if (!args.contains(argName)) return std::nullopt;
		if constexpr (std::same_as<T, std::string>)
			return args.at(argName)[0];
		else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>)
			return t_from_string<T>(args.at(argName)[0]);
		// Vector handling and Position 2D/3D handling
		else if constexpr (std::same_as<T, std::vector<std::string>>)
			return args.at(argName);
		else if constexpr (std::same_as<T, Position2D>) {
			if (args.at(argName).size() < 2) return std::nullopt;
			return Position2D{t_from_string<float>(args.at(argName)[0]),
							  t_from_string<float>(args.at(argName)[1])};
		} else if constexpr (std::same_as<T, Position3D>) {
			if (args.at(argName).size() < 2) return std::nullopt;
			// Allow for 2D positions to be used as 3D
			if (args.at(argName).size() == 2)
				return Position3D{t_from_string<float>(args.at(argName)[0]),
								  t_from_string<float>(args.at(argName)[1]), 0.0f};

			return Position3D{t_from_string<float>(args.at(argName)[0]),
							  t_from_string<float>(args.at(argName)[1]),
							  t_from_string<float>(args.at(argName)[2])};
		}
		return std::nullopt;
	}

	// Splits this message into multiple submessages
	[[nodiscard]] auto split_into_submessages() const -> std::vector<TwitchChatMessage> {
		if (!is_command()) return {*this};
		if (const auto argGroups = get_strings_between(message, "<", ">"); argGroups.empty())
			return {*this};
		else {
			auto command = get_command();
			auto groups = std::vector<TwitchChatMessage>{};
			for (const auto &argGroup : argGroups) {
				auto groupArgs = std::map<std::string, std::vector<std::string>>{};
				auto groupMsg = get_string_after(message, argGroup + ">");
				groupMsg = get_string_until(groupMsg, "<");
				for (const auto &arg : split_string(argGroup, ",")) {
					const auto argSplit = split_string(arg, "=");
					if (argSplit.size() != 2) continue;
					groupArgs[argSplit[0]] = split_string(argSplit[1], "|");
				}
				groups.emplace_back(user, groupMsg, command, time, groupArgs);
			}
			return groups;
		}
	}

	[[nodiscard]] auto is_command() const -> bool {
		return message.starts_with("!") || !command.empty();
	}

	[[nodiscard]] auto get_command() const -> std::string {
		if (!is_command()) return "";
		if (command.empty()) {
			if (message.starts_with("!")) {
				auto command = get_string_after(message, "!");
				command = get_string_until(command, " ");
				command = get_string_until(command, "<");
				return trim_string(command);
			} else
				return "";
		} else
			return command;
	}

	[[nodiscard]] auto get_message() const -> std::string {
		if (!is_command()) return message;
		if (const auto argGroups = get_strings_between(message, "<", ">"); argGroups.empty()) {
			if (message.starts_with("!") && !command.empty())
				return trim_string(get_string_after(message, command));
			else
				return message;
		} else {
			// Erase all argument groups from the message, joining remaining parts by space
			std::vector<std::string> messages;
			for (const auto &argGroup : argGroups) {
				const auto groupMsg = get_string_after(message, argGroup + ">");
				messages.emplace_back(get_string_until(groupMsg, "<"));
			}
			return std::accumulate(
				messages.begin(), messages.end(), std::string{},
				[](const std::string &a, const std::string &b) { return a + " " + b; });
		}
	}
};

// Struct for holding user data
// TODO: Move to general module
export struct TwitchUser {
	std::string name;
	bool bypassCooldown = false;
	TwitchChatMessage lastMessage;
	std::int32_t userVoice = -1;

	explicit TwitchUser(std::string name, TwitchChatMessage lastMessage)
		: name(std::move(name)), lastMessage(std::move(lastMessage)) {}
};

// Cache of users
// TODO: Move to general module
export std::map<std::string, std::shared_ptr<TwitchUser>> global_users;
