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

// Testing messages for TTS command
constexpr std::array ttsTestMessages = {
	"Hello, world!",
	"Do you like waffles?",
	"All your base are belong to us",
	"Look at my horse, my horse is amazing",
	"Hey, listen!",
	"Never gonna give you up, never gonna let you down, never gonna run around and desert you",
	"This is a test message OwO",
};

// Struct for command
export struct Command {
	bool enabled;
	std::string callstr, description;
	std::function<void(const std::string &)> func;

	Command() = default;
	Command(const std::string &call, const std::string &desc,
					 const std::function<void(const std::string &)> &f)
		: enabled(true), callstr(call), description(desc), func(f) {}
};

// Class for commands handling
export class CommandHandler {
	// Command map
	static inline std::map<std::string, Command> m_commandsMap;

public:
	// Initializes CommandHandler, adding the default commands
	// requires passing the method for launching notifications, circular dependency stuff..
	static auto initialize(const std::function<void(const std::string &)> &launch_notification)
		-> Result {
		if (m_commandsMap.empty()) {
			m_commandsMap["text_to_speech"] =
				Command("tts", "TTS Notification", [](const std::string &msg) {
					const std::string notifMsg = msg;
					TTSHandler::voiceString(notifMsg);
				});
			m_commandsMap["custom_notification"] =
				Command("cc", "Custom Notification", [launch_notification](const std::string &msg) {
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
				});
		}

		return Result();
	}

	// Cleans up resources used by CommandHandler
	static void cleanup() { m_commandsMap.clear(); }

	// Tests a command with random message from testMessages
	static void test_command(const std::string &command) {
		// If the command does not exist, skip
		if (!m_commandsMap.contains(command))
			return;

		// Make sure the command is enabled
		if (!m_commandsMap[command].enabled)
			return;

		std::string testMsg;
		if (command == "text_to_speech")
			testMsg = ttsTestMessages[random_int(0, ttsTestMessages.size() - 1)];
		else {
			// Choose either ascii or easter egg sound
			if (const auto choice = random_int(0, 1); choice == 0) {
				const auto asciiarts = AssetsHandler::get_ascii_art_keys();
				testMsg = AssetsHandler::get_ascii_art_text(
					asciiarts[random_int(0, asciiarts.size() - 1)]);
			} else {
				const auto eggSounds = AssetsHandler::get_egg_sound_keys();
				testMsg = eggSounds[random_int(0, eggSounds.size() - 1)];
			}
		}

		// Execute the command with the test message
		execute_command(command, testMsg);
	}

	// Returns a non-modifiable command map
	static auto get_commands_map() -> const std::map<std::string, Command> & {
		return m_commandsMap;
	}

	// Sets whether command is enabled
	static void set_command_enabled(const std::string &key, const bool enabled) {
		// If the key does not exist, skip
		if (!m_commandsMap.contains(key))
			return;

		m_commandsMap[key].enabled = enabled;
	}

	// Method for changing the call string of a command
	static void change_command_call(const std::string &key, const std::string &newCall) {
		// If the key does not exist, skip
		if (!m_commandsMap.contains(key))
			return;

		// Make sure the new call is not empty
		if (newCall.empty())
			return;

		// Change the callstr
		m_commandsMap[key].callstr = newCall;
	}

	// Method for executing a command
	static void execute_command(const std::string &key, const std::string &msg) {
		// If the command does not exist, skip
		if (!m_commandsMap.contains(key))
			return;

		// Make sure the command is enabled
		if (!m_commandsMap[key].enabled)
			return;

		// Execute the command
		m_commandsMap[key].func(msg);
	}
};