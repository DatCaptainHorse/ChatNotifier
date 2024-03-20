module;

#include <string>
#include <vector>
#include <filesystem>

//#include <glaze/glaze.hpp>

export module config;

import common;
import assets;

// Struct for keeping configs across launches
struct Config {
	float notifAnimationLength = 5.0f;
	float notifEffectSpeed = 5.0f;
	float notifFontScale = 1.0f;
	float globalAudioVolume = 1.0f;
	std::vector<std::string> approvedUsers;
	std::string twitchAuthToken, twitchAuthUser, twitchChannel;

	auto save() -> Result {
		//if (const auto ec = glz::write_file_json(
		//		this, (AssetsHandler::get_assets_path() / "config.json").string(), std::string{}))
		//	return Result(1, "Failed to save config: {}", std::to_string(ec));

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

		//if (const auto ec = glz::read_file_json(
		//		*this, (AssetsHandler::get_assets_path() / "config.json").string(), std::string{}))
		//	return Result(1, "Failed to load config: {}", std::to_string(ec));

		// Resize twitch variables so ImGui can handle them (64 ought to be enough)
		twitchAuthToken.resize(64);
		twitchAuthUser.resize(64);
		twitchChannel.resize(64);

		return Result();
	}
};

// Global config instance
export Config global_config;
