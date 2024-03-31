module;

#include <map>
#include <array>
#include <string>
#include <ranges>
#include <chrono>
#include <thread>
#include <algorithm>
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
	"According to all known laws of aviation, there is no way a bee should be able to fly",
	"One. Two. Three. Awoo!",
};

// Command function using
using CommandFunction = std::function<void(const TwitchChatMessage &)>;

// Struct for command
export struct Command {
	bool enabled;
	std::string callstr, description;
	CommandFunction func;
	float transitionTime = 0.0f;
	std::chrono::time_point<std::chrono::steady_clock> lastExecuted;

	Command() = default;
	Command(std::string call, std::string desc, const CommandFunction &f)
		: enabled(true), callstr(std::move(call)), description(std::move(desc)), func(f) {}
};

// Class for commands handling
export class CommandHandler {
	static inline std::map<std::string, Command> m_commandsMap;
	// Live threads running commands
	static inline std::vector<std::thread> m_commandThreads;

public:
	// Initializes CommandHandler, adding the default commands
	// requires passing the method for launching notifications, circular dependency stuff..
	static auto initialize(const std::function<void(const std::string &, const TwitchChatMessage &)>
							   &launch_notification) -> Result {
		if (m_commandsMap.empty()) {
			m_commandsMap["text_to_speech"] =
				Command("tts", "TTS Notification", [](const TwitchChatMessage &mainMsg) {
					const auto splitMsgs = mainMsg.split_into_submessages();
					std::vector<std::pair<TTSData, SoundOptions>> soundsToPlay;
					for (const auto &msg : splitMsgs) {
						const std::string notifMsg = msg.get_message();
						std::int32_t speakerID = -1;
						if (global_users.contains(msg.user))
							speakerID = global_users[msg.user]->userVoice;

						auto voiceSpeed = msg.get_command_arg<float>("speed").value_or(
							global_config.ttsVoiceSpeed.value);
						voiceSpeed = std::clamp(voiceSpeed, global_config.ttsVoiceSpeed.min,
												global_config.ttsVoiceSpeed.max);

						auto voicePitch = msg.get_command_arg<float>("pitch").value_or(
							global_config.ttsVoicePitch.value);
						voicePitch = std::clamp(voicePitch, global_config.ttsVoicePitch.min,
												global_config.ttsVoicePitch.max);

						const auto voicePos = msg.get_command_arg<Position3D>("pos");
						const auto voiceEffects =
							msg.get_command_arg<std::vector<std::string>>("sfx");

						const auto ttsData =
							TTSHandler::voiceString(notifMsg, speakerID, voiceSpeed);

						soundsToPlay.emplace_back(std::make_pair(
							ttsData, SoundOptions{global_config.ttsVoiceVolume.value, voicePitch,
												  voicePos, voiceEffects}));
					}

					for (const auto &[ttsData, soundOptions] : soundsToPlay) {
						const auto doneTime = AudioPlayer::play_oneshot_memory(
							ttsData.audio, ttsData.sampleRate, soundOptions);
						// Wait until the sound is done playing
						// TODO: Implement play_sequential_memory
						std::this_thread::sleep_for(doneTime);
					}
				});
			m_commandsMap["custom_notification"] = Command(
				"cc", "Custom Notification",
				[launch_notification](const TwitchChatMessage &mainMsg) {
					const auto splitMsgs = mainMsg.split_into_submessages();
					for (const auto &msg : splitMsgs) {
						std::string notifMsg = msg.get_message();
						// Split to words (space-separated)
						const auto words = split_string(notifMsg, " ");

						// Get the first ascii art from words and prepend it to the notification
						const auto asciiarts = AssetsHandler::get_ascii_art_keys();
						for (const auto &word : words) {
							if (const auto found = std::ranges::find(asciiarts, word);
								found != asciiarts.end()) {
								notifMsg =
									AssetsHandler::get_ascii_art_text(*found) + "\n" + notifMsg;
								break;
							}
						}

						auto audioPitch = msg.get_command_arg<float>("pitch").value_or(1.0f);
						audioPitch = std::clamp(audioPitch, 0.1f, 2.0f);

						const auto audioPos = msg.get_command_arg<Position3D>("pos");
						const auto audioEffects =
							msg.get_command_arg<std::vector<std::string>>("sfx");

						// Find all easter egg sound words, pushing into vector
						// limited to global_config.maxAudioTriggers
						std::vector<std::filesystem::path> sounds;
						const auto eggSounds = AssetsHandler::get_egg_sound_keys();
						for (const auto &word : words) {
							if (const auto found = std::ranges::find(eggSounds, word);
								found != eggSounds.end() &&
								sounds.size() < global_config.maxAudioTriggers.value) {
								sounds.push_back(AssetsHandler::get_egg_sound_path(*found));
							}
						}
						// Play easter egg sounds
						if (!sounds.empty())
							AudioPlayer::play_sequential(
								sounds, {1.0f, audioPitch, audioPos, audioEffects});

						launch_notification(notifMsg, msg);
					}
				});
		}

		return Result();
	}

	// Cleans up resources used by CommandHandler
	static void cleanup() {
		m_commandsMap.clear();
		for (auto &thread : m_commandThreads) {
			if (thread.joinable()) thread.join();
		}
	}

	// Tests a command with random message from testMessages
	static void test_command(const std::string &command) {
		// If the command does not exist, skip
		if (!m_commandsMap.contains(command)) return;
		// Make sure the command is enabled
		if (!m_commandsMap[command].enabled) return;

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
		execute_command(command, TwitchChatMessage("testUser", testMsg));
	}

	// Returns key for command with given call string
	static auto get_command_key(const std::string &call) -> std::string {
		for (const auto &[key, cmd] : m_commandsMap)
			if (cmd.callstr == call) return key;

		return "";
	}

	// Returns a non-modifiable command map
	static auto get_commands_map() -> const std::map<std::string, Command> & {
		return m_commandsMap;
	}

	// Sets whether command is enabled
	static void set_command_enabled(const std::string &key, const bool enabled) {
		// If the key does not exist, skip
		if (!m_commandsMap.contains(key)) return;
		m_commandsMap[key].enabled = enabled;
	}

	// Method for changing the call string of a command
	static void change_command_call(const std::string &key, const std::string &newCall) {
		// If the key does not exist, skip
		if (!m_commandsMap.contains(key)) return;
		// Make sure the new call is not empty
		if (newCall.empty()) return;
		// Change the callstr
		m_commandsMap[key].callstr = newCall;
	}

	// Method for executing a command
	static void execute_command(const std::string &key, const TwitchChatMessage &msg) {
		// If the command does not exist, skip
		if (!m_commandsMap.contains(key)) return;
		// Make sure the command is enabled
		if (!m_commandsMap[key].enabled) return;
		// Set new last executed time
		m_commandsMap[key].lastExecuted = std::chrono::steady_clock::now();

		// Cleanup old threads here before adding new one, join if joinable
		std::erase_if(m_commandThreads, [](std::thread &thread) {
			if (thread.joinable()) {
				thread.join();
				return true;
			}
			return false;
		});

		// Launch each subcommand in new thread
		m_commandThreads.emplace_back([key, msg] { m_commandsMap[key].func(msg); });
	}

	// Method for returning time when command was last executed
	static auto get_last_executed_time(const std::string &key)
		-> std::chrono::time_point<std::chrono::steady_clock> {
		// If the key does not exist, return epoch
		if (!m_commandsMap.contains(key))
			return std::chrono::time_point<std::chrono::steady_clock>();

		return m_commandsMap[key].lastExecuted;
	}
};