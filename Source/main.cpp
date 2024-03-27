#include <ranges>
#include <thread>
#include <chrono>
#include <format>
#include <iostream>
#include <algorithm>
#include <filesystem>

import config;
import common;
import assets;
import audio;
import gui;
import twitch;
import commands;
import tts;

// Method for printing Result errors
void print_error(const Result &res) {
	if (!res) std::cerr << "Error: " << res.message << std::endl;
}

void twc_callback_handler(const TwitchChatMessage &msg);

// Main method
auto main(int argc, char **argv) -> int {
	// LOAD CONFIG //
	if (const auto res = global_config.load(); !res) {
		print_error(res);
		return res.code;
	}

	// INITIALIZE //
	if (const auto res = AssetsHandler::initialize(argv[0]); !res) {
		print_error(res);
		return res.code;
	}

	if (const auto res = AudioPlayer::initialize(); !res) {
		print_error(res);
		return res.code;
	}

	if (const auto res = TwitchChatConnector::initialize(twc_callback_handler); !res) {
		print_error(res);
		return res.code;
	}

	if (const auto res = TTSHandler::initialize(); !res) {
		print_error(res);
		return res.code;
	}

	// Play tutturuu (if it exists) to test audio
	if (AssetsHandler::get_egg_sound_exists("tutturuu"))
		AudioPlayer::play_oneshot(AssetsHandler::get_egg_sound_path("tutturuu").string());

	// GUI and CommandHandler initialization
	if (const auto res = NotifierGUI::initialize(); !res) {
		print_error(res);
		return res.code;
	}

	if (const auto res = CommandHandler::initialize(NotifierGUI::launch_notification); !res) {
		print_error(res);
		return res.code;
	}

	// Run the main loop
	while (!NotifierGUI::should_close()) {
		// Update the GUI
		NotifierGUI::render();
		// Update the audio
		AudioPlayer::update();
		// TTS update
		TTSHandler::update();
		// Sleep for 5ms to lighten the load on the CPU
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	// SAVE CONFIG //
	if (const auto res = global_config.save(); !res) {
		print_error(res);
		return res.code;
	}

	// CLEANUP //
	NotifierGUI::cleanup();
	CommandHandler::cleanup();
	TTSHandler::cleanup();
	TwitchChatConnector::cleanup();
	AudioPlayer::cleanup();
	AssetsHandler::cleanup();

	return 0;
}

void twc_callback_handler(const TwitchChatMessage &msg) {
	// Check that the user is in the approvedUsers list, if empty just allow it
	if (global_config.approvedUsers.empty() ||
		std::ranges::any_of(global_config.approvedUsers, [&](const auto &user) {
			return lowercase(user) == lowercase(msg.user);
		})) {
		// Check for command and execute
		for (const auto &[key, command] : CommandHandler::get_commands_map()) {
			const auto fullCommand = command.callstr;
			if (fullCommand.empty()) continue;

			// Make sure the message is long enough to contain the command
			if (msg.message.size() < fullCommand.size()) continue;

			// Extract the command from the message
			auto extractedCommand = msg.get_command();

			// If command is shorter than the extracted command, skip, prevents partial matches
			if (fullCommand.size() < extractedCommand.size()) continue;

			// Strict comparison of the command, to prevent partial matches
			if (lowercase(extractedCommand).contains(fullCommand)) {
				CommandHandler::execute_command(key, msg);
				break;
			}
		}
	}
}
