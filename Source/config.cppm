module;

#include <string>
#include <vector>
#include <filesystem>

#include <glaze/glaze.hpp>

export module config;

import common;
import filesystem;

// Enum for cooldown types
export enum class CommandCooldownType : std::uint8_t {
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
	std::string twitchAuthToken = "", twitchAuthUser = "", twitchChannel = "";
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
		if (glz::write_file_json(this, get_config_path().string(),
								 std::string{}))
			return Result(1, "Failed to save config");

		return Result();
	}

	auto load() -> Result {
		// Check if file exists first
		if (!std::filesystem::exists(get_config_path())) return Result();

		if (glz::read_file_json(*this, get_config_path().string(),
								std::string{}))
			return Result(1, "Failed to load config");

		return Result();
	}

private:
	static auto get_config_path() -> std::filesystem::path {
		return Filesystem::get_root_path() / "config.json";
	}
};

// Global config instance
export Config global_config;
