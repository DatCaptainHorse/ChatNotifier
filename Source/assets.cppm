module;

#include <map>
#include <ranges>
#include <vector>
#include <fstream>
#include <filesystem>

export module assets;

import common;

// Returns path to assets folder
auto get_assets_path() -> std::filesystem::path { return "Assets/"; }
// Returns path to font assets folder
auto get_font_assets_path() -> std::filesystem::path { return get_assets_path() / "Fonts/"; }
// Returns path to trigger ASCII art folder
auto get_trigger_ascii_path() -> std::filesystem::path {
	return get_assets_path() / "TriggerASCII/";
}
// Returns path to trigger sounds folder
auto get_trigger_sounds_path() -> std::filesystem::path {
	return get_assets_path() / "TriggerSounds/";
}

// Class which handles assets
export class AssetsHandler {
	// Map of font files linked to their paths
	static inline std::map<std::string, std::filesystem::path> font_files;
	// Map of ASCII art files linked to their paths
	static inline std::map<std::string, std::filesystem::path> ascii_art_files;
	// Map of easter-egg sounds linked to their paths
	static inline std::map<std::string, std::filesystem::path> egg_sounds;

public:
	// Method that initializes assets, finds them and populates resources
	static void initialize() {
		populate_font_files();
		populate_ascii_art_files();
		populate_egg_sounds();
	}

	// Cleans up resources used by AssetsHandler
	static void cleanup() {
		font_files.clear();
		ascii_art_files.clear();
		egg_sounds.clear();
	}

	// Returns if the font file exists
	static auto get_font_exists(const std::string &font_name) -> bool {
		return font_files.contains(strip_extension(font_name));
	}
	// Returns path to font file
	static auto get_font_path(const std::string &font_name) -> std::filesystem::path {
		return font_files[strip_extension(font_name)];
	}
	// Returns available font keys as a vector
	static auto get_font_keys() -> std::vector<std::string> {
		std::vector<std::string> keys;
		keys.reserve(font_files.size());
		for (const auto &key : font_files | std::views::keys)
			keys.push_back(key);

		return keys;
	}

	// Returns if the ascii art file exists
	static auto get_ascii_art_exists(const std::string &ascii_art_name) -> bool {
		return ascii_art_files.contains(strip_extension(ascii_art_name));
	}
	// Returns path to ascii art file
	static auto get_ascii_art_path(const std::string &ascii_art_name) -> std::filesystem::path {
		return ascii_art_files[strip_extension(ascii_art_name)];
	}
	// Returns the text data of the ascii art file
	static auto get_ascii_art_text(const std::string &ascii_art_name) -> std::string {
		if (const auto &path = get_ascii_art_path(ascii_art_name); std::filesystem::exists(path)) {
			if (std::ifstream file(path); file.is_open()) {
				std::string ascii_art;
				std::string line;
				while (std::getline(file, line))
					ascii_art += line + '\n';

				return ascii_art;
			}
		}
		return "";
	}
	// Returns available ascii art keys as a vector
	static auto get_ascii_art_keys() -> std::vector<std::string> {
		std::vector<std::string> keys;
		keys.reserve(ascii_art_files.size());
		for (const auto &key : ascii_art_files | std::views::keys)
			keys.push_back(key);

		return keys;
	}

	// Returns if the easter-egg sound file exists
	static auto get_egg_sound_exists(const std::string &egg_word) -> bool {
		return egg_sounds.contains(strip_extension(egg_word));
	}
	// Returns path to easter-egg sound file
	static auto get_egg_sound_path(const std::string &egg_word) -> std::filesystem::path {
		return egg_sounds[strip_extension(egg_word)];
	}
	// Returns available easter-egg sound keys as a vector
	static auto get_egg_sound_keys() -> std::vector<std::string> {
		std::vector<std::string> keys;
		keys.reserve(egg_sounds.size());
		for (const auto &key : egg_sounds | std::views::keys)
			keys.push_back(key);

		return keys;
	}

private:
	// Method that populates the fonts map
	static void populate_font_files() {
		for (const auto &entry : std::filesystem::directory_iterator(get_font_assets_path())) {
			if (entry.is_regular_file()) {
				// Only files with ".ttf" extension
				if (const auto &path = entry.path(); path.extension() == ".ttf") {
					// Key is the filename without extension, value is the path
					font_files[path.stem().string()] = path;
				}
			}
		}
	}

	// Method that populates the ascii art map
	static void populate_ascii_art_files() {
		for (const auto &entry : std::filesystem::directory_iterator(get_trigger_ascii_path())) {
			if (entry.is_regular_file()) {
				// Only files with ".txt" extension
				if (const auto &path = entry.path(); path.extension() == ".txt") {
					// Key is the filename without extension, value is the path
					ascii_art_files[path.stem().string()] = path;
				}
			}
		}
	}

	// Method that populates the easter-egg sound map
	static void populate_egg_sounds() {
		for (const auto &entry : std::filesystem::directory_iterator(get_trigger_sounds_path())) {
			if (entry.is_regular_file()) {
				// Only files with ".opus" extension
				if (const auto &path = entry.path(); path.extension() == ".opus") {
					// Key is the filename without extension, value is the path
					egg_sounds[path.stem().string()] = path;
				}
			}
		}
	}
};