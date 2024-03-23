module;

#include <string>
#include <vector>
#include <filesystem>

// #include <glaze/glaze.hpp>
#include <toml.hpp>

export module config;

import common;
import assets;

// Enum for cooldown types
export enum class CommandCooldownType { eNone, eGlobal, ePerUser };

// Struct for keeping configs across launches
export struct Config {
	float notifAnimationLength = 5.0f;
	float notifEffectSpeed = 5.0f;
	float notifFontScale = 1.0f;
	float globalAudioVolume = 0.75f;
	std::vector<std::string> approvedUsers;
	std::string twitchAuthToken, twitchAuthUser, twitchChannel;
	CommandCooldownType cooldownType = CommandCooldownType::eGlobal;
	std::uint32_t cooldownTime = 5;
	std::uint32_t maxAudioTriggers = 3; //< How many audio triggers can a message cause
	float audioSequenceOffset = -0.5f;	//< Offset for how long to wait between audio triggers
	float ttsVoiceSpeed = 1.0f;			//< Speed of TTS voice

	auto save() -> Result {
		// if (glz::write_file_json(this, (AssetsHandler::get_exec_path() /
		// "config.json").string(), 						 std::string{})) 	return Result(1,
		// "Failed to save config");

		// Use toml++ for now until MSVC fixes it's dumbness
		auto config = toml::table{{"notifications",
								   toml::table{
									   {"notifAnimationLength", notifAnimationLength},
									   {"notifEffectSpeed", notifEffectSpeed},
									   {"notifFontScale", notifFontScale},
								   }},
								  {"audio",
								   toml::table{
									   {"globalAudioVolume", globalAudioVolume},
									   {"maxAudioTriggers", maxAudioTriggers},
									   {"audioSequenceOffset", audioSequenceOffset},
								   }},
								  {"twitch",
								   toml::table{
									   {"twitchAuthToken", twitchAuthToken},
									   {"twitchAuthUser", twitchAuthUser},
									   {"twitchChannel", twitchChannel},
								   }},
								  {"commands",
								   toml::table{
									   {"cooldownType", static_cast<std::uint32_t>(cooldownType)},
									   {"cooldownTime", cooldownTime},
								   }},
								  {"tts",
								   toml::table{
									   {"ttsVoiceSpeed", ttsVoiceSpeed},
								   }},
								  {"approvedUsers", toml::array{}}};

		// Add users to the approvedUsers array
		if (!approvedUsers.empty()) {
			const auto node = config["approvedUsers"].as_array();
			for (const auto &user : approvedUsers)
				node->push_back(user);
		}

		std::ofstream file((AssetsHandler::get_exec_path() / "config.toml").string());
		if (!file.is_open())
			return Result(1, "Failed to save config");

		file << config;
		file.close();

		return Result();
	}

	auto load() -> Result {
		// Check if file exists first
		// if (!std::filesystem::exists(AssetsHandler::get_exec_path() / "config.json"))
		if (!std::filesystem::exists(AssetsHandler::get_exec_path() / "config.toml"))
			return Result();

		// if (glz::read_file_json(*this, (AssetsHandler::get_exec_path() /
		// "config.json").string(), 						std::string{})) 	return Result(1,
		// "Failed to load config");

		// Same here, toml++
		toml::table config;
		try {
			config = toml::parse_file((AssetsHandler::get_exec_path() / "config.toml").string());
		} catch (toml::parse_error &e) {
			return Result(1, e.what());
		}

		notifAnimationLength =
			config["notifications"]["notifAnimationLength"].value_or(notifAnimationLength);
		notifEffectSpeed = config["notifications"]["notifEffectSpeed"].value_or(notifEffectSpeed);
		notifFontScale = config["notifications"]["notifFontScale"].value_or(notifFontScale);
		globalAudioVolume = config["audio"]["globalAudioVolume"].value_or(globalAudioVolume);
		twitchAuthToken = config["twitch"]["twitchAuthToken"].value_or(twitchAuthToken);
		twitchAuthUser = config["twitch"]["twitchAuthUser"].value_or(twitchAuthUser);
		twitchChannel = config["twitch"]["twitchChannel"].value_or(twitchChannel);
		cooldownType = config["commands"]["cooldownType"].value_or(cooldownType);
		cooldownTime = config["commands"]["cooldownTime"].value_or(cooldownTime);
		maxAudioTriggers = config["audio"]["maxAudioTriggers"].value_or(maxAudioTriggers);
		audioSequenceOffset = config["audio"]["audioSequenceOffset"].value_or(audioSequenceOffset);
		ttsVoiceSpeed = config["tts"]["ttsVoiceSpeed"].value_or(ttsVoiceSpeed);

		// Load approved users
		if (const auto node = config["approvedUsers"].as_array(); node) {
			approvedUsers.clear();
			for (const auto &user : *node)
				approvedUsers.push_back(user.as_string()->get());
		}

		return Result();
	}
};

// Global config instance
export Config global_config;
