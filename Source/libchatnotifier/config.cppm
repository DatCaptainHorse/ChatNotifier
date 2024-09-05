module;

#ifndef CN_SUPPORTS_MODULES_STD
#include <standard.hpp>
#endif

#include <hv/json.hpp>

export module config;

import standard;
import common;
import filesystem;

// Enum for cooldown types
export enum CommandCooldownType : std::uint8_t {
	eNone = 0,
	eGlobal = 1 << 0,
	ePerUser = 1 << 1,
	ePerCommand = 1 << 2,
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
	std::string twitchChannel = "", refreshToken = "";
	CommandCooldownType enabledCooldowns = CommandCooldownType::eGlobal;
	ConfigOption<std::uint32_t> cooldownGlobal{5, 1, 600};
	ConfigOption<std::uint32_t> cooldownPerUser{5, 1, 600};
	ConfigOption<std::uint32_t> cooldownPerCommand{5, 1, 600};
	ConfigOption<std::uint32_t> maxAudioTriggers{
		3, 0, 10}; //< How many audio triggers can a message cause
	ConfigOption<float> audioSequenceOffset{
		-1.0f, -5.0f, 0.0f}; //< Offset for audio sequence, reduces time until next audio trigger
	ConfigOption<float> ttsVoiceSpeed{1.0f, 0.1, 2.0f};	 //< Speed of TTS voice
	ConfigOption<float> ttsVoiceVolume{1.0f, 0.1, 1.0f}; //< Volume of TTS voice
	ConfigOption<float> ttsVoicePitch{1.0f, 0.1, 2.0f};	 //< Pitch of TTS voice

	auto save() -> Result {
		nlohmann::json json;

		json["notifAnimationLength"] = notifAnimationLength.value;
		json["notifEffectSpeed"] = notifEffectSpeed.value;
		json["notifEffectIntensity"] = notifEffectIntensity.value;
		json["notifFontScale"] = notifFontScale.value;
		json["globalAudioVolume"] = globalAudioVolume.value;
		json["twitchChannel"] = twitchChannel;
		json["refreshToken"] = refreshToken;
		json["enabledCooldowns"] = enabledCooldowns;
		json["cooldownGlobal"] = cooldownGlobal.value;
		json["cooldownPerUser"] = cooldownPerUser.value;
		json["cooldownPerCommand"] = cooldownPerCommand.value;
		json["maxAudioTriggers"] = maxAudioTriggers.value;
		json["audioSequenceOffset"] = audioSequenceOffset.value;
		json["ttsVoiceSpeed"] = ttsVoiceSpeed.value;
		json["ttsVoiceVolume"] = ttsVoiceVolume.value;
		json["ttsVoicePitch"] = ttsVoicePitch.value;

		// Approved users has to be made into comma separated string
		std::string approvedUsersStr;
		for (const auto &user : approvedUsers) approvedUsersStr += user + ",";

		json["approvedUsers"] = approvedUsersStr;

		std::ofstream file(get_config_path());
		file << json.dump(4);
		file.close();

		return Result();
	}

	auto load() -> Result {
		// Check if file exists first
		if (!std::filesystem::exists(get_config_path())) return Result();

		// Read the file
		auto file = std::ifstream(get_config_path());
		const std::string file_contents((std::istreambuf_iterator<char>(file)),
										std::istreambuf_iterator<char>());

		auto json = nlohmann::json::parse(file_contents);

		notifAnimationLength.value = json["notifAnimationLength"].get<float>();
		notifEffectSpeed.value = json["notifEffectSpeed"].get<float>();
		notifEffectIntensity.value = json["notifEffectIntensity"].get<float>();
		notifFontScale.value = json["notifFontScale"].get<float>();
		globalAudioVolume.value = json["globalAudioVolume"].get<float>();
		twitchChannel = json["twitchChannel"].get<std::string>();
		refreshToken = json["refreshToken"].get<std::string>();
		enabledCooldowns =
			static_cast<CommandCooldownType>(json["enabledCooldowns"].get<std::uint8_t>());
		cooldownGlobal.value = json["cooldownGlobal"].get<std::uint32_t>();
		cooldownPerUser.value = json["cooldownPerUser"].get<std::uint32_t>();
		cooldownPerCommand.value = json["cooldownPerCommand"].get<std::uint32_t>();
		maxAudioTriggers.value = json["maxAudioTriggers"].get<std::uint32_t>();
		audioSequenceOffset.value = json["audioSequenceOffset"].get<float>();
		ttsVoiceSpeed.value = json["ttsVoiceSpeed"].get<float>();
		ttsVoiceVolume.value = json["ttsVoiceVolume"].get<float>();
		ttsVoicePitch.value = json["ttsVoicePitch"].get<float>();

		// Approved users has to be made into vector from comma separated string
		const auto approvedUsersStr = json["approvedUsers"].get<std::string>();
		const auto splitted = split_string(approvedUsersStr, ",");
		approvedUsers.clear();
		for (const auto &user : splitted) {
			if (!user.empty()) approvedUsers.push_back(user);
		}

		return Result();
	}

	auto to_json() -> nlohmann::json {
		nlohmann::json json;

		json["notifAnimationLength"] = notifAnimationLength.value;
		json["notifEffectSpeed"] = notifEffectSpeed.value;
		json["notifEffectIntensity"] = notifEffectIntensity.value;
		json["notifFontScale"] = notifFontScale.value;
		json["globalAudioVolume"] = globalAudioVolume.value;
		json["twitchChannel"] = twitchChannel;
		json["refreshToken"] = refreshToken;
		json["enabledCooldowns"] = enabledCooldowns;
		json["cooldownGlobal"] = cooldownGlobal.value;
		json["cooldownPerUser"] = cooldownPerUser.value;
		json["cooldownPerCommand"] = cooldownPerCommand.value;
		json["maxAudioTriggers"] = maxAudioTriggers.value;
		json["audioSequenceOffset"] = audioSequenceOffset.value;
		json["ttsVoiceSpeed"] = ttsVoiceSpeed.value;
		json["ttsVoiceVolume"] = ttsVoiceVolume.value;
		json["ttsVoicePitch"] = ttsVoicePitch.value;

		// Approved users has to be made into comma separated string
		std::string approvedUsersStr;
		for (const auto &user : approvedUsers) approvedUsersStr += user + ",";

		json["approvedUsers"] = approvedUsersStr;

		return json;
	}

	void from_json_string(const std::string &jsonStr) {
		auto json = nlohmann::json::parse(jsonStr);

		notifAnimationLength.value = json["notifAnimationLength"].get<float>();
		notifEffectSpeed.value = json["notifEffectSpeed"].get<float>();
		notifEffectIntensity.value = json["notifEffectIntensity"].get<float>();
		notifFontScale.value = json["notifFontScale"].get<float>();
		globalAudioVolume.value = json["globalAudioVolume"].get<float>();
		twitchChannel = json["twitchChannel"].get<std::string>();
		refreshToken = json["refreshToken"].get<std::string>();
		enabledCooldowns =
			static_cast<CommandCooldownType>(json["enabledCooldowns"].get<std::uint8_t>());
		cooldownGlobal.value = json["cooldownGlobal"].get<std::uint32_t>();
		cooldownPerUser.value = json["cooldownPerUser"].get<std::uint32_t>();
		cooldownPerCommand.value = json["cooldownPerCommand"].get<std::uint32_t>();
		maxAudioTriggers.value = json["maxAudioTriggers"].get<std::uint32_t>();
		audioSequenceOffset.value = json["audioSequenceOffset"].get<float>();
		ttsVoiceSpeed.value = json["ttsVoiceSpeed"].get<float>();
		ttsVoiceVolume.value = json["ttsVoiceVolume"].get<float>();
		ttsVoicePitch.value = json["ttsVoicePitch"].get<float>();

		// Approved users has to be made into vector from comma separated string
		const auto approvedUsersStr = json["approvedUsers"].get<std::string>();
		const auto splitted = split_string(approvedUsersStr, ",");
		approvedUsers.clear();
		for (const auto &user : splitted) {
			if (!user.empty()) approvedUsers.push_back(user);
		}
	}

private:
	static auto get_config_path() -> std::filesystem::path {
		return Filesystem::get_root_path() / "config.json";
	}
};

// Global config instance
export Config global_config;
