module;

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

//#include <glaze/glaze.hpp>
#include <nlohmann/json.hpp>

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
	float audioSequenceOffset = -1.0f; //< Offset for how long to wait between audio triggers

	auto save() -> Result {
		//if (glz::write_file_json(this, (AssetsHandler::get_assets_path() / "config.json").string(),
		//						 std::string{}))
		//	return Result(1, "Failed to save config");

		// Using nlohmann json for now
		nlohmann::json j;
		j["notifAnimationLength"] = notifAnimationLength;
		j["notifEffectSpeed"] = notifEffectSpeed;
		j["notifFontScale"] = notifFontScale;
		j["globalAudioVolume"] = globalAudioVolume;
		j["approvedUsers"] = approvedUsers;
		j["twitchAuthToken"] = twitchAuthToken;
		j["twitchAuthUser"] = twitchAuthUser;
		j["twitchChannel"] = twitchChannel;
		j["cooldownType"] = static_cast<int>(cooldownType);
		j["cooldownTime"] = cooldownTime;
		j["maxAudioTriggers"] = maxAudioTriggers;
		j["audioSequenceOffset"] = audioSequenceOffset;

		std::ofstream file(AssetsHandler::get_assets_path() / "config.json");
		if (!file.is_open())
			return Result(1, "Failed to open config file for writing");

		file << j.dump(4);
		file.close();

		return Result();
	}

	auto load() -> Result {
		// Check if file exists first
		if (!std::filesystem::exists(AssetsHandler::get_assets_path() / "config.json")) {
			twitchAuthToken.resize(64);
			twitchAuthUser.resize(64);
			twitchChannel.resize(64);
			return Result();
		}

		//if (glz::read_file_json(*this, (AssetsHandler::get_assets_path() / "config.json").string(),
		//						std::string{}))
		//	return Result(1, "Failed to load config");

		// Using nlohmann json for now
		nlohmann::json j;
		std::ifstream file(AssetsHandler::get_assets_path() / "config.json");
		if (!file.is_open())
			return Result(1, "Failed to open config file for reading");

		file >> j;
		file.close();

		notifAnimationLength = j["notifAnimationLength"];
		notifEffectSpeed = j["notifEffectSpeed"];
		notifFontScale = j["notifFontScale"];
		globalAudioVolume = j["globalAudioVolume"];
		approvedUsers = j["approvedUsers"];
		twitchAuthToken = j["twitchAuthToken"];
		twitchAuthUser = j["twitchAuthUser"];
		twitchChannel = j["twitchChannel"];
		cooldownType = static_cast<CommandCooldownType>(j["cooldownType"]);
		cooldownTime = j["cooldownTime"];
		maxAudioTriggers = j["maxAudioTriggers"];
		audioSequenceOffset = j["audioSequenceOffset"];

		// Resize twitch variables so ImGui can handle them (64 ought to be enough)
		twitchAuthToken.resize(64);
		twitchAuthUser.resize(64);
		twitchChannel.resize(64);

		return Result();
	}
};

// Global config instance
export Config global_config;
