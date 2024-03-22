module;

#include <string>
#include <vector>
#include <filesystem>

// #include <glaze/glaze.hpp>

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
	std::size_t cooldownTime = 5;
	std::size_t maxAudioTriggers = 3;  //< How many audio triggers can a message cause
	float audioSequenceOffset = -0.5f; //< Offset for how long to wait between audio triggers

	auto save() -> Result {
		// if (glz::write_file_json(this, (AssetsHandler::get_exec_path() /
		// "config.json").string(), 						 std::string{})) 	return Result(1,
		// "Failed to save config");

		return Result();
	}

	auto load() -> Result {
		// Check if file exists first
		if (!std::filesystem::exists(AssetsHandler::get_exec_path() / "config.json"))
			return Result();

		// if (glz::read_file_json(*this, (AssetsHandler::get_exec_path() /
		// "config.json").string(), 						std::string{})) 	return Result(1,
		// "Failed to load config");

		return Result();
	}
};

// Global config instance
export Config global_config;
