module;

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

export module config;

import jsoned;
import common;
import assets;

// Enum for cooldown types
export enum class CommandCooldownType { eNone, eGlobal, ePerUser };

// Struct for keeping configs across launches
export struct Config {
	float notifAnimationLength = 5.0f;
	float notifEffectSpeed = 2.0f;
	float notifEffectIntensity = 2.0f;
	float notifFontScale = 1.0f;
	float globalAudioVolume = 0.75f;
	std::vector<std::string> approvedUsers;
	std::string twitchAuthToken, twitchAuthUser, twitchChannel;
	CommandCooldownType cooldownType = CommandCooldownType::eGlobal;
	std::uint32_t cooldownTime = 5;
	std::uint32_t maxAudioTriggers = 3; //< How many audio triggers can a message cause
	float audioSequenceOffset = -0.5f;	//< Offset for how long to wait between audio triggers
	float ttsVoiceSpeed = 1.0f;			//< Speed of TTS voice
	float ttsVoiceVolume = 1.0f;		//< Volume of TTS voice

	auto save() -> Result {
		JSONed::JSON json;
		json["notifAnimationLength"].set<float>(notifAnimationLength);
		json["notifEffectSpeed"].set<float>(notifEffectSpeed);
		json["notifEffectIntensity"].set<float>(notifEffectIntensity);
		json["notifFontScale"].set<float>(notifFontScale);
		json["globalAudioVolume"].set<float>(globalAudioVolume);
		json["twitchAuthToken"].set<std::string>(twitchAuthToken);
		json["twitchAuthUser"].set<std::string>(twitchAuthUser);
		json["twitchChannel"].set<std::string>(twitchChannel);
		json["cooldownType"].set<int>(static_cast<int>(cooldownType));
		json["cooldownTime"].set<std::uint32_t>(cooldownTime);
		json["maxAudioTriggers"].set<std::uint32_t>(maxAudioTriggers);
		json["audioSequenceOffset"].set<float>(audioSequenceOffset);
		json["ttsVoiceSpeed"].set<float>(ttsVoiceSpeed);
		json["ttVoiceVolume"].set<float>(ttsVoiceVolume);
		json["approvedUsers"].set<std::vector<std::string>>(approvedUsers);

		if (!json.save(get_config_path())) return Result(1, "Failed to save config");

		return Result();
	}

	auto load() -> Result {
		// Check if file exists first
		if (!std::filesystem::exists(get_config_path())) return Result();

		JSONed::JSON json;
		if (!json.load(get_config_path())) return Result(1, "Failed to load config");

		notifAnimationLength = json["notifAnimationLength"].get<float>();
		notifEffectSpeed = json["notifEffectSpeed"].get<float>();
		notifEffectIntensity = json["notifEffectIntensity"].get<float>();
		notifFontScale = json["notifFontScale"].get<float>();
		globalAudioVolume = json["globalAudioVolume"].get<float>();
		twitchAuthToken = json["twitchAuthToken"].get<std::string>();
		twitchAuthUser = json["twitchAuthUser"].get<std::string>();
		twitchChannel = json["twitchChannel"].get<std::string>();
		cooldownType = static_cast<CommandCooldownType>(json["cooldownType"].get<int>());
		cooldownTime = json["cooldownTime"].get<std::uint32_t>();
		maxAudioTriggers = json["maxAudioTriggers"].get<std::uint32_t>();
		audioSequenceOffset = json["audioSequenceOffset"].get<float>();
		ttsVoiceSpeed = json["ttsVoiceSpeed"].get<float>();
		ttsVoiceVolume = json["ttsVoiceVolume"].get<float>();
		approvedUsers = json["approvedUsers"].get<std::vector<std::string>>();

		return Result();
	}

private:
	static auto get_config_path() -> std::filesystem::path {
		return AssetsHandler::get_exec_path() / "config.json";
	}
};

// Global config instance
export Config global_config;
