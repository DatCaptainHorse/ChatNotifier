module;

#include <map>
#include <string>
#include <ranges>
#include <functional>
#include <filesystem>

export module commands;

import config;
import common;
import assets;
import audio;
import tts;

// Class for commands handling
export class CommandHandler {
	// Command map of string used to trigger the command = description string + function to execute
	static inline std::map<std::string,
						   std::tuple<std::string, std::function<void(const std::string &)>>>
		m_commandsMap;
	// Vector of notification strings
	static inline std::vector<std::string> m_notificationStrings;

public:
	// Initializes CommandHandler, adding the default commands
	// requires passing the method for launching notifications, circular dependency stuff..
	static auto initialize(const std::function<void(const std::string &)> &launch_notification)
		-> Result {
		if (m_notificationStrings.empty()) {
			m_notificationStrings = {
				"Check the chat, nerd!",
				"Chat requires your attention",
				"You should check the chat..",
				"Feed the chatters, they're hungry",
				"Did you know that chat exists?",
				"Chat exploded! Just kidding, check it!",
				"Free chat check, limited time offer!",
				"Insert funny chat notification here",
				"Chatters are waffling, provide syrup",
			};
		}

		if (m_commandsMap.empty()) {
			m_commandsMap["tts"] = {
				"TTS Notification", [launch_notification](const std::string &msg) {
					const std::string notifMsg = msg;
					TTSHandler::voiceString(notifMsg);
					//launch_notification(notifMsg);
				}};
			m_commandsMap["cc"] = {
				"Custom Notification", [launch_notification](const std::string &msg) {
					std::string notifMsg = msg;
					// Split to words (space-separated)
					const auto words = split_string(notifMsg, " ");

					// Get the first ascii art from words and prepend it to the notification
					const auto asciiarts = AssetsHandler::get_ascii_art_keys();
					for (const auto &word : words) {
						if (const auto found = std::ranges::find(asciiarts, word);
							found != asciiarts.end()) {
							notifMsg = AssetsHandler::get_ascii_art_text(*found) + "\n" + notifMsg;
							break;
						}
					}

					// Find all easter egg sound words, pushing into vector
					// limited to global_config.maxAudioTriggers
					std::vector<std::filesystem::path> sounds;
					const auto eggSounds = AssetsHandler::get_egg_sound_keys();
					for (const auto &word : words) {
						if (const auto found = std::ranges::find(eggSounds, word);
							found != eggSounds.end() &&
							sounds.size() < global_config.maxAudioTriggers) {
							sounds.push_back(AssetsHandler::get_egg_sound_path(*found));
						}
					}
					// Play easter egg sounds
					if (!sounds.empty())
						AudioPlayer::play_sequential(sounds);

					launch_notification(notifMsg);
				}};
		}

		return Result();
	}

	// Cleans up resources used by CommandHandler
	static void cleanup() { m_commandsMap.clear(); }

	// Returns a non-modifiable command map
	static auto get_commands_map() -> const
		std::map<std::string, std::tuple<std::string, std::function<void(const std::string &)>>> & {
		return m_commandsMap;
	}

	// Method for changing the key of a command
	static void change_command_key(const std::string &oldKey, const std::string &newKey) {
		// If the old key does not exist, skip
		if (!m_commandsMap.contains(oldKey))
			return;

		// If the new key already exists, skip
		if (m_commandsMap.contains(newKey))
			return;

		// Get the tuple from the old key
		const auto tuple = m_commandsMap[oldKey];

		// Remove the old key
		m_commandsMap.erase(oldKey);

		// Add the new key with the old tuple
		m_commandsMap[newKey] = tuple;
	}

	// Method for executing a command
	static void execute_command(const std::string &command, const std::string &msg) {
		// If the command does not exist, skip
		if (!m_commandsMap.contains(command))
			return;

		// Execute the command
		const auto &[_, func] = m_commandsMap[command];
		func(msg);
	}
};