#include <print>
#include <locale>
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
import filesystem;
import scripting;

// Method for printing Result errors
void print_error(const Result &res) {
	if (!res) std::println("Error: {}", res.message);
}

void twc_callback_handler(const TwitchChatMessage &msg);

// Main method
auto main(int argc, char **argv) -> int {
	// Make sure UTF-8 is used
	std::locale::global(std::locale("en_US.UTF-8"));

	// Initialize filesystem to get the root path first //
	if (const auto res = Filesystem::initialize(argc, argv); !res) {
		print_error(res);
		return res.code;
	}

	// LOAD CONFIG //
	if (const auto res = global_config.load(); !res) {
		print_error(res);
		// Continue despite error, as might be outdated config
	}

	// INITIALIZE //
	if (const auto res = ScriptingHandler::initialize(); !res) {
		print_error(res);
		return res.code;
	}

	if (const auto res = AssetsHandler::initialize(); !res) {
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

	// GUI and CommandHandler initialization
	if (const auto res = NotifierGUI::initialize(); !res) {
		print_error(res);
		return res.code;
	}

	if (const auto res = CommandHandler::initialize(NotifierGUI::launch_notification); !res) {
		print_error(res);
		return res.code;
	}

	// Create module
	if (const auto res = ScriptingHandler::create_module(); !res) {
		print_error(res);
		return res.code;
	}

	// Add scripts
	ScriptingHandler::refresh_scripts([] {
		for (const auto script : ScriptingHandler::get_scripts()) {
			if (!script->is_valid()) continue;
			ScriptingHandler::has_script_method(
				script, "on_message", [script](const bool hasMethod) {
					if (!hasMethod) return;
					CommandHandler::add_command(
						script->get_name(),
						Command(script->get_call_string(), script->get_call_string(),
								[script](const TwitchChatMessage &msg) {
									ScriptingHandler::execute_script_msg(script, msg);
								}));
				});
		}
	});

	// Run the main loop
	while (!NotifierGUI::should_close()) {
		// Update the GUI
		NotifierGUI::render();
		// Update the audio
		AudioPlayer::update();
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
	ScriptingHandler::cleanup();
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
