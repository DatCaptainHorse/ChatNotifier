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

// Struct for a config option
export template <typename T>
struct ConfigOption {
	T value;
	T min;
	T max;
};

// Struct for keeping configs across launches
export struct Config {
	ConfigOption<float> notifAnimationLength{5.0f, 1.0f, 30.0f};
	ConfigOption<float> notifEffectSpeed{2.0f, 0.1f, 10.0f};
	ConfigOption<float> notifEffectIntensity{2.0f, 0.1f, 10.0f};
	ConfigOption<float> notifFontScale{1.0f, 0.5f, 2.0f};
	ConfigOption<float> globalAudioVolume{0.5f, 0.0f, 1.0f};
	std::vector<std::string> approvedUsers = {};
	std::string twitchAuthToken = "", twitchAuthUser = "", twitchChannel = "";
	CommandCooldownType enabledCooldowns = CommandCooldownType::eGlobal;
	ConfigOption<std::uint32_t> cooldownTime{5, 1, 60};
	ConfigOption<std::uint32_t> maxAudioTriggers{
		3, 0, 10}; //< How many audio triggers can a message cause
	ConfigOption<float> audioSequenceOffset{
		-1.0f, -5.0f, 0.0f}; //< Offset for how long to wait between audio triggers
	ConfigOption<float> ttsVoiceSpeed{1.0f, 0.1, 2.0f};	 //< Speed of TTS voice
	ConfigOption<float> ttsVoiceVolume{1.0f, 0.1, 1.0f}; //< Volume of TTS voice
	ConfigOption<float> ttsVoicePitch{1.0f, 0.1, 2.0f};	 //< Pitch of TTS voice

	auto save() -> Result {
		JSONed::JSON json;
		json["notifAnimationLength"].set<float>(notifAnimationLength.value);
		json["notifEffectSpeed"].set<float>(notifEffectSpeed.value);
		json["notifEffectIntensity"].set<float>(notifEffectIntensity.value);
		json["notifFontScale"].set<float>(notifFontScale.value);
		json["globalAudioVolume"].set<float>(globalAudioVolume.value);
		json["twitchAuthToken"].set<std::string>(twitchAuthToken);
		json["twitchAuthUser"].set<std::string>(twitchAuthUser);
		json["twitchChannel"].set<std::string>(twitchChannel);
		json["enabledCooldowns"].set<CommandCooldownType>(enabledCooldowns);
		json["cooldownTime"].set<std::uint32_t>(cooldownTime.value);
		json["maxAudioTriggers"].set<std::uint32_t>(maxAudioTriggers.value);
		json["audioSequenceOffset"].set<float>(audioSequenceOffset.value);
		json["ttsVoiceSpeed"].set<float>(ttsVoiceSpeed.value);
		json["ttVoiceVolume"].set<float>(ttsVoiceVolume.value);
		json["ttsVoicePitch"].set<float>(ttsVoicePitch.value);
		json["approvedUsers"].set<std::vector<std::string>>(approvedUsers);

		if (!json.save(get_config_path())) return Result(1, "Failed to save config");

		return Result();
	}

	auto load() -> Result {
		// Check if file exists first
		if (!std::filesystem::exists(get_config_path())) return Result();

		JSONed::JSON json;
		if (!json.load(get_config_path())) return Result(1, "Failed to load config");

		notifAnimationLength.value =
			json["notifAnimationLength"].get<float>().value_or(notifAnimationLength.value);
		notifEffectSpeed.value =
			json["notifEffectSpeed"].get<float>().value_or(notifEffectSpeed.value);
		notifEffectIntensity.value =
			json["notifEffectIntensity"].get<float>().value_or(notifEffectIntensity.value);
		notifFontScale.value = json["notifFontScale"].get<float>().value_or(notifFontScale.value);
		globalAudioVolume.value =
			json["globalAudioVolume"].get<float>().value_or(globalAudioVolume.value);
		twitchAuthToken = json["twitchAuthToken"].get<std::string>().value_or(twitchAuthToken);
		twitchAuthUser = json["twitchAuthUser"].get<std::string>().value_or(twitchAuthUser);
		twitchChannel = json["twitchChannel"].get<std::string>().value_or(twitchChannel);
		enabledCooldowns =
			json["enabledCooldowns"].get<CommandCooldownType>().value_or(enabledCooldowns);
		cooldownTime.value = json["cooldownTime"].get<std::uint32_t>().value_or(cooldownTime.value);
		maxAudioTriggers.value =
			json["maxAudioTriggers"].get<std::uint32_t>().value_or(maxAudioTriggers.value);
		audioSequenceOffset.value =
			json["audioSequenceOffset"].get<float>().value_or(audioSequenceOffset.value);
		ttsVoiceSpeed.value = json["ttsVoiceSpeed"].get<float>().value_or(ttsVoiceSpeed.value);
		ttsVoiceVolume.value = json["ttsVoiceVolume"].get<float>().value_or(ttsVoiceVolume.value);
		ttsVoicePitch.value = json["ttsVoicePitch"].get<float>().value_or(ttsVoicePitch.value);
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
