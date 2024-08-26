export module commands;

import standard;
import types;
import config;
import common;
import assets;
import audio;

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

public:
	// Initializes CommandHandler, adding the default commands
	// requires passing the method for launching notifications, circular dependency stuff..
	static auto initialize(const std::function<void(const std::string &, const TwitchChatMessage &)>
							   &launch_notification) -> Result {
		if (m_commandsMap.empty()) {
			m_commandsMap["custom_notification"] = Command(
				"cc", "Custom Notification",
				[launch_notification](const TwitchChatMessage &mainMsg) {
					for (auto splitMsgs = mainMsg.split_into_submessages(); auto &msg : splitMsgs) {
						std::string notifMsg = msg.get_message();
						if (notifMsg.empty()) return;

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

						auto audioOffset = msg.get_command_arg<float>("offset").value_or(
							global_config.audioSequenceOffset.value);

						const auto audioPos = msg.get_command_arg<Position3D>("pos");
						const auto audioEffects =
							msg.get_command_arg<std::vector<std::string>>("sfx");

						// Find all easter egg sound words, pushing into vector
						// limited to global_config.maxAudioTriggers
						std::vector<std::filesystem::path> sounds;
						std::vector<SoundOptions> soundOptions;
						const auto eggSounds = AssetsHandler::get_egg_sound_keys();
						for (const auto &word : words) {
							if (const auto found = std::ranges::find(eggSounds, word);
								found != eggSounds.end() &&
								sounds.size() < global_config.maxAudioTriggers.value) {
								sounds.emplace_back(AssetsHandler::get_egg_sound_path(*found));
								soundOptions.emplace_back(1.0f, audioPitch, audioOffset, audioPos,
														  audioEffects);
							}
						}
						// Play easter egg sounds
						if (!sounds.empty()) AudioPlayer::play_sequential(sounds, soundOptions);

						launch_notification(notifMsg, msg);
					}
				});
		}

		return Result();
	}

	// Cleans up resources used by CommandHandler
	static void cleanup() {
		m_commandsMap.clear();
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

		// Launch each subcommand in new thread
		m_commandsMap[key].func(msg);
	}

	static void add_command(const std::string &key, const Command &cmd) {
		m_commandsMap[key] = cmd;
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