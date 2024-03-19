#include <ranges>
#include <algorithm>
#include <fmt/format.h>

import common;
import assets;
import audio;
import gui;
import twitch;
import commands;

void twc_callback_handler(const TwitchChatMessage &msg);

// Main method
auto main(int argc, char **argv) -> int {
	// INITIALIZE //
	AssetsHandler::initialize(argv[0]);
	AudioPlayer::initialize();
	TwitchChatConnector::initialize(twc_callback_handler);

	// Play tutturuu (if it exists) to test audio
	if (AssetsHandler::get_egg_sound_exists("tutturuu"))
		AudioPlayer::play_oneshot(AssetsHandler::get_egg_sound_path("tutturuu").string());

	// GUI and CommandHandler initialization
	NotifierGUI::initialize();
	CommandHandler::initialize(NotifierGUI::launch_notification);

	// Run the main loop (GUI)
	while (!NotifierGUI::should_close()) {
		// Update the GUI
		NotifierGUI::render();
	}

	// CLEANUP //
	NotifierGUI::cleanup();
	CommandHandler::cleanup();
	TwitchChatConnector::cleanup();
	AudioPlayer::cleanup();
	AssetsHandler::cleanup();

	return 0;
}

void twc_callback_handler(const TwitchChatMessage &msg) {
	// If message does not begin with "!", skip
	if (msg.message[0] != '!')
		return;

	// Check that the user is in the approvedUsers list, if empty just allow it
	const auto approvedUsers = NotifierGUI::get_approved_users();
	if (approvedUsers.empty() || std::ranges::any_of(approvedUsers, [&](const auto &user) {
			return lowercase(user) == lowercase(msg.user);
		})) {
		// Check for command and execute
		for (const auto &[command, pair] : CommandHandler::get_commands_map()) {
			const auto &[_, func] = pair;

			// Command with the "!" prefix to match the message
			const auto fullCommand = fmt::format("!{}", command);

			// Make sure the message is long enough to contain the command
			if (msg.message.size() < fullCommand.size())
				continue;

			// Extract the command from the message
			// so until whitespace is met after command or end of string is reached
			auto extractedCommand = msg.message;
			if (const auto wsPos = msg.message.find(' '); wsPos != std::string::npos)
				extractedCommand = msg.message.substr(0, wsPos);

			// Remove odd characters from the extracted command
			std::erase_if(extractedCommand, [](const auto &ec) {
				return ec == '\n' || ec == '\r' || ec == '\0' || ec == '\t' || std::isspace(ec);
			});

			// If command is shorter than the extracted command, skip, prevents partial matches
			if (fullCommand.size() < extractedCommand.size())
				continue;

			// Strict comparison of the command, to prevent partial matches
			if (lowercase(extractedCommand).find(fullCommand) != std::string::npos) {
				// Cut off the command from the message + space if there is one
				const auto msgWithoutCommand = msg.message.substr(
					fullCommand.size() + (msg.message[fullCommand.size()] == ' '));
				func(msgWithoutCommand);
				break;
			}
		}
	}
}
