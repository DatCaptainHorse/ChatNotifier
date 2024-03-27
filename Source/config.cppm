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
export enum class CommandCooldownType : unsigned int {
	eNone = 0,
	eGlobal = 1 << 0,
	ePerUser = 1 << 1,
	ePerCommand = 1 << 2,
};
// Enable bitmask operators for CommandCooldownType
template <>
struct FEnableBitmaskOperators<CommandCooldownType> {
	static constexpr bool enable = true;
};

// Struct for keeping configs across launches
export struct Config {
	float notifAnimationLength = 5.0f;
	float notifEffectSpeed = 2.0f;
	float notifEffectIntensity = 2.0f;
	float notifFontScale = 1.0f;
	float globalAudioVolume = 0.5f;
	std::vector<std::string> approvedUsers = {};
	std::string twitchAuthToken = "", twitchAuthUser = "", twitchChannel = "";
	CommandCooldownType enabledCooldowns = CommandCooldownType::eGlobal;
	std::uint32_t cooldownTime = 5;
	std::uint32_t maxAudioTriggers = 3; //< How many audio triggers can a message cause
	float audioSequenceOffset = -1.0f;	//< Offset for how long to wait between audio triggers
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
		json["enabledCooldowns"].set<CommandCooldownType>(enabledCooldowns);
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

		notifAnimationLength =
			json["notifAnimationLength"].get<float>().value_or(notifAnimationLength);
		notifEffectSpeed = json["notifEffectSpeed"].get<float>().value_or(notifEffectSpeed);
		notifEffectIntensity =
			json["notifEffectIntensity"].get<float>().value_or(notifEffectIntensity);
		notifFontScale = json["notifFontScale"].get<float>().value_or(notifFontScale);
		globalAudioVolume = json["globalAudioVolume"].get<float>().value_or(globalAudioVolume);
		twitchAuthToken = json["twitchAuthToken"].get<std::string>().value_or(twitchAuthToken);
		twitchAuthUser = json["twitchAuthUser"].get<std::string>().value_or(twitchAuthUser);
		twitchChannel = json["twitchChannel"].get<std::string>().value_or(twitchChannel);
		enabledCooldowns = json["enabledCooldowns"].get<CommandCooldownType>().value_or(enabledCooldowns);
		cooldownTime = json["cooldownTime"].get<std::uint32_t>().value_or(cooldownTime);
		maxAudioTriggers = json["maxAudioTriggers"].get<std::uint32_t>().value_or(maxAudioTriggers);
		audioSequenceOffset =
			json["audioSequenceOffset"].get<float>().value_or(audioSequenceOffset);
		ttsVoiceSpeed = json["ttsVoiceSpeed"].get<float>().value_or(ttsVoiceSpeed);
		ttsVoiceVolume = json["ttsVoiceVolume"].get<float>().value_or(ttsVoiceVolume);
		approvedUsers =
			json["approvedUsers"].get<std::vector<std::string>>().value_or(approvedUsers);

		return Result();
	}

private:
	static auto get_config_path() -> std::filesystem::path {
		return AssetsHandler::get_exec_path() / "config.json";
	}
};

// Global config instance
export Config global_config;
