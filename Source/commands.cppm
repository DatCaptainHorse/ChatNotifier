module;

#include <map>
#include <string>
#include <functional>

export module commands;

import common;
import assets;
import audio;

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
	static void initialize(const std::function<void(const std::string &)> &launch_notification) {
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
			m_commandsMap["cc"] = {
				"Random Notification", [launch_notification](const std::string &) {
					// Pick a random notification string
					const auto &notifMsg =
						m_notificationStrings[random_int(0, m_notificationStrings.size() - 1)];

					launch_notification(notifMsg);
				}};
			m_commandsMap["ccc"] = {
				"Custom Notification", [launch_notification](const std::string &msg) {
					std::string notifMsg = msg;

					// Ascii art prepending, if key with the same name exists
					const auto asciiarts = AssetsHandler::get_ascii_art_keys();
					// Get the first ascii art key that is found in the message
					if (const auto asciiart = std::ranges::find_if(asciiarts,
																   [&](const auto &key) {
																	   return notifMsg.find(key) !=
																			  std::string::npos;
																   });
						asciiart != asciiarts.end()) {
						// Reformat notifMsg
						notifMsg = AssetsHandler::get_ascii_art_text(*asciiart) + "\n" + notifMsg;
					}

					// Play easter-egg sound if one is found, just the first found sound
					const auto eggSounds = AssetsHandler::get_egg_sound_keys();
					if (const auto eggSound = std::ranges::find_if(eggSounds,
																   [&](const auto &key) {
																	   return notifMsg.find(key) !=
																			  std::string::npos;
																   });
						eggSound != eggSounds.end()) {
						AudioPlayer::play_oneshot(AssetsHandler::get_egg_sound_path(*eggSound));
					}

					launch_notification(notifMsg);
				}};
		}
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