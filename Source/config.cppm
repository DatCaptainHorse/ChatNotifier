module;

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

// #include <glaze/glaze.hpp>

#include <json_struct/json_struct.h>

export module config;

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

// Config struct for JSON
struct ConfigJSON {
	float notifAnimationLength;
	float notifEffectSpeed;
	float notifEffectIntensity;
	float notifFontScale;
	float globalAudioVolume;
	std::vector<std::string> approvedUsers;
	std::string twitchChannel;
	std::string refreshToken;
	std::uint8_t enabledCooldowns;
	std::uint32_t cooldownTime;
	std::uint32_t maxAudioTriggers;
	float audioSequenceOffset;
	float ttsVoiceSpeed;
	float ttsVoiceVolume;
	float ttsVoicePitch;

	JS_OBJ(notifAnimationLength, notifEffectSpeed, notifEffectIntensity, notifFontScale,
		   globalAudioVolume, approvedUsers, twitchChannel, refreshToken, enabledCooldowns,
		   cooldownTime, maxAudioTriggers, audioSequenceOffset, ttsVoiceSpeed, ttsVoiceVolume,
		   ttsVoicePitch);
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
	ConfigOption<std::uint32_t> cooldownTime{5, 1, 60};
	ConfigOption<std::uint32_t> maxAudioTriggers{
		3, 0, 10}; //< How many audio triggers can a message cause
	ConfigOption<float> audioSequenceOffset{
		-1.0f, -5.0f, 0.0f}; //< Offset for audio sequence, reduces time until next audio trigger
	ConfigOption<float> ttsVoiceSpeed{1.0f, 0.1, 2.0f};	 //< Speed of TTS voice
	ConfigOption<float> ttsVoiceVolume{1.0f, 0.1, 1.0f}; //< Volume of TTS voice
	ConfigOption<float> ttsVoicePitch{1.0f, 0.1, 2.0f};	 //< Pitch of TTS voice

	auto save() -> Result {
		/*if (glz::write_file_json(this, get_config_path().string(),
								 std::string{}))
			return Result(1, "Failed to save config");
*/
		json.notifAnimationLength = notifAnimationLength.value;
		json.notifEffectSpeed = notifEffectSpeed.value;
		json.notifEffectIntensity = notifEffectIntensity.value;
		json.notifFontScale = notifFontScale.value;
		json.globalAudioVolume = globalAudioVolume.value;
		json.approvedUsers = approvedUsers;
		json.twitchChannel = twitchChannel;
		json.refreshToken = refreshToken;
		json.enabledCooldowns = static_cast<std::uint8_t>(enabledCooldowns);
		json.cooldownTime = cooldownTime.value;
		json.maxAudioTriggers = maxAudioTriggers.value;
		json.audioSequenceOffset = audioSequenceOffset.value;
		json.ttsVoiceSpeed = ttsVoiceSpeed.value;
		json.ttsVoiceVolume = ttsVoiceVolume.value;
		json.ttsVoicePitch = ttsVoicePitch.value;

		const auto pretty_json = JS::serializeStruct(json);
		auto file = std::ofstream(get_config_path());
		file << pretty_json;
		file.close();

		return Result();
	}

	auto load() -> Result {
		// Check if file exists first
		if (!std::filesystem::exists(get_config_path())) return Result();

		/*if (glz::read_file_json(*this, get_config_path().string(),
								std::string{}))
			return Result(1, "Failed to load config");
*/
		// Read the file
		auto file = std::ifstream(get_config_path());
		const std::string file_contents((std::istreambuf_iterator<char>(file)),
								  std::istreambuf_iterator<char>());

		if (JS::ParseContext context(file_contents); context.parseTo(json) != JS::Error::NoError) {
			return Result(1, "Failed to parse config");
		}

		notifAnimationLength.value = json.notifAnimationLength;
		notifEffectSpeed.value = json.notifEffectSpeed;
		notifEffectIntensity.value = json.notifEffectIntensity;
		notifFontScale.value = json.notifFontScale;
		globalAudioVolume.value = json.globalAudioVolume;
		approvedUsers = json.approvedUsers;
		twitchChannel = json.twitchChannel;
		refreshToken = json.refreshToken;
		enabledCooldowns = static_cast<CommandCooldownType>(json.enabledCooldowns);
		cooldownTime.value = json.cooldownTime;
		maxAudioTriggers.value = json.maxAudioTriggers;
		audioSequenceOffset.value = json.audioSequenceOffset;
		ttsVoiceSpeed.value = json.ttsVoiceSpeed;
		ttsVoiceVolume.value = json.ttsVoiceVolume;
		ttsVoicePitch.value = json.ttsVoicePitch;

		return Result();
	}

private:
	static inline ConfigJSON json;

	static auto get_config_path() -> std::filesystem::path {
		return Filesystem::get_root_path() / "config.json";
	}
};

// Global config instance
export Config global_config;
