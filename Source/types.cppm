module;

#include <map>
#include <string>
#include <vector>
#include <chrono>
#include <numeric>
#include <optional>
#include <algorithm>
#include <functional>
#include <filesystem>

export module types;

import common;

// Struct for Twitch message data
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
				return get_string_after(message, command);
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
export struct TwitchUser {
	std::string name;
	bool bypassCooldown = false;
	TwitchChatMessage lastMessage;
	std::int32_t userVoice = -1;

	explicit TwitchUser(std::string name, TwitchChatMessage lastMessage)
		: name(std::move(name)), lastMessage(std::move(lastMessage)) {}
};