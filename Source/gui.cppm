module;

#define GLFW_INCLUDE_NONE				// Don't include OpenGL headers by default
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM // Same goes for ImGui

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include <filesystem>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>
#include <format>
#include <array>
#include <memory>
#include <ranges>

export module gui;

import config;
import common;
import assets;
import audio;
import notification;
import twitch;
import commands;

// Class which manages the GUI + notifications
export class NotifierGUI {
	static inline bool m_keepRunning = false;
	static inline GLFWwindow *m_mainWindow;
	static inline ImFont *m_mainFont;
	static inline ImFont *m_notifFont;
	static inline float m_DPI;

	// Vector of live-notifications
	static inline std::vector<std::unique_ptr<Notification>> m_notifications;

	static inline auto m_colorOK = ImVec4(0.0f, 0.8f, 0.0f, 1.0f);
	static inline auto m_colorError = ImVec4(0.8f, 0.0f, 0.0f, 1.0f);
	static inline auto m_colorWait = ImVec4(0.8f, 0.4f, 0.0f, 1.0f);
	static inline auto m_colorDisabled = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

public:
	// Initialize the GUI and it's resources
	static auto initialize() -> Result {
		// GLFW INITIALIZATION //
		glfwSetErrorCallback(glfw_error_callback);
		if (!glfwInit()) return Result(1, "Failed to initialize GLFW!");

		// WINDOW CREATION //
		const auto monitor = glfwGetPrimaryMonitor();
		const auto mode = glfwGetVideoMode(monitor);

		// GLFW window hints
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
		glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_TRUE);
		glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

		// Main window
		m_mainWindow = glfwCreateWindow(420, 690, "ChatNotifier", nullptr, nullptr);
		if (!m_mainWindow) return Result(2, "Failed to create GLFW window!");

		glfwMakeContextCurrent(m_mainWindow);
		glfwSwapInterval(1); // V-Sync

		// GL3W INITIALIZATION //
		if (gl3wInit()) {
			return Result(3, "Failed to initialize GL3W!");
		}
		if (!gl3wIsSupported(3, 3)) {
			return Result(4, "OpenGL 3.3 not supported!");
		}

		// Print OpenGL version
		std::cout << std::format("OpenGL Version: {}", get_gl_string(GL_VERSION)) << std::endl;

		// IMGUI INITIALIZATION //
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		auto &io = ImGui::GetIO();

		// Enable docking and viewports
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigViewportsNoTaskBarIcon = true;
		io.ConfigViewportsNoDecoration = true;

		// Calculate pixel density
		// (DPI = (square root of (horizontal pixels² + vertical pixels²)) / diagonal screen size in
		// inches)
		m_DPI = std::sqrt(std::pow(mode->width, 2) + std::pow(mode->height, 2)) / mode->width;
		const auto mainFontSize = 18.0 * m_DPI;
		const auto notifFontSize = 64.0 * m_DPI;

		// NotoSansMono.ttf for main text
		if (AssetsHandler::get_font_exists("NotoSansMono.ttf")) {
			m_mainFont = io.Fonts->AddFontFromFileTTF(
				AssetsHandler::get_font_path("NotoSansMono.ttf").string().c_str(),
				static_cast<float>(mainFontSize), nullptr, io.Fonts->GetGlyphRangesDefault());

			// Set as default font
			io.FontDefault = m_mainFont;
		} else {
			// We require this font
			return Result(5, "Main font NotoSansMono.ttf not found!");
		}

		// NotoSansSymbols2.ttf for notifications
		// Contains special glyps so we need to tell ImGui about it
		if (AssetsHandler::get_font_exists("NotoSansSymbols2.ttf")) {
			const std::array<ImWchar, 5> notif_ranges = {0x0020, 0x00FF, 0x2800, 0x28FF, 0};
			m_notifFont = io.Fonts->AddFontFromFileTTF(
				AssetsHandler::get_font_path("NotoSansSymbols2.ttf").string().c_str(),
				static_cast<float>(notifFontSize), nullptr, notif_ranges.data());
		} else {
			// NOT strictly required, warn about lacking symbols
			std::cout
				<< "Warning: Notification font NotoSansSymbols2.ttf not found, using default font."
				<< std::endl;
			m_notifFont = m_mainFont;
		}

		// MAIN WINDOW IMGUI INITIALIZATION //
		ImGui_ImplGlfw_InitForOpenGL(m_mainWindow, true);
		ImGui_ImplOpenGL3_Init("#version 330 core");

		// Build fonts
		io.Fonts->Build();

		// Ready to run
		m_keepRunning = true;

		return Result();
	}

	// Cleans up the GUI and it's resources
	static void cleanup() {
		m_keepRunning = false;

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwMakeContextCurrent(nullptr);
		glfwDestroyWindow(m_mainWindow);
		glfwTerminate();
	}

	// Returns if the gui should close
	[[nodiscard]] static auto should_close() -> bool {
		return !m_keepRunning || glfwWindowShouldClose(m_mainWindow);
	}

	// GUI drawing and updating
	static void render() {
		// GLFW POLL EVENTS //
		glfwPollEvents();

		// IMGUI NEW FRAME //
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// CONTROL WINDOW //
		{
			const auto mainViewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(mainViewport->Pos, ImGuiCond_Once);
			ImGui::SetNextWindowSize(mainViewport->Size, ImGuiCond_Once);
			ImGui::Begin("Chat Notifier Controls", &m_keepRunning,
						 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
							 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration |
							 ImGuiWindowFlags_AlwaysVerticalScrollbar);

			// Settings portion, separators n stuff
			ImGui::Separator();
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 -
								 ImGui::CalcTextSize("Notification Settings").x / 2);
			ImGui::Text("Notification Settings");
			ImGui::Separator();

			// Slider for notification show time, integer-steps from 1 to 20
			ImGui::Text("Notification show time:");
			ImGui::SliderFloat("##showTime", &global_config.notifAnimationLength, 1.0f, 20.0f,
							   "%.0f");

			// Slider for notification effect speed, float from 0.1 to 10.0
			ImGui::Text("Notification effect speed:");
			ImGui::SliderFloat("##effectSpeed", &global_config.notifEffectSpeed, 0.1f, 10.0f,
							   "%.1f");

			// Slider for notification effect intensity, float from 0.1 to 10.0
			ImGui::Text("Notification effect intensity:");
			ImGui::SliderFloat("##effectIntensity", &global_config.notifEffectIntensity, 0.1f,
							   10.0f, "%.1f");

			// Slider for notification font scale, float from 0.5 to 2.0
			ImGui::Text("Notification font scale:");
			if (ImGui::SliderFloat("##fontScale", &global_config.notifFontScale, 0.5f, 2.0f,
								   "%.1f"))
				m_notifFont->Scale = global_config.notifFontScale;

			// Button to refresh assets
			ImGui::Dummy(ImVec2(0, 10));
			if (ImGui::Button("Find New Assets", ImVec2(-1, 30))) AssetsHandler::refresh();

			// Add padding before separators
			ImGui::Dummy(ImVec2(0, 10));

			// Audio settings
			ImGui::Separator();
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 -
								 ImGui::CalcTextSize("Audio Settings").x / 2);
			ImGui::Text("Audio Settings");
			ImGui::Separator();

			// Slider for global audio volume, which is a float from 0.0f to 1.0f
			ImGui::Text("Global audio volume:");
			if (ImGui::SliderFloat("##globalVol", &global_config.globalAudioVolume, 0.0f, 1.0f,
								   "%.2f"))
				AudioPlayer::set_global_volume(global_config.globalAudioVolume);

			// Slider for audio sequence offset, which is a float from -5.0f to 0.0f
			ImGui::Text("Audio sequence offset:");
			ImGui::SliderFloat("##audioSeqOffset", &global_config.audioSequenceOffset, -5.0f, 0.0f,
							   "%.1f");

			// Slider for max audio triggers, which is an integer from 0 to 10
			ImGui::Text("Max audio triggers:");
			ImGui::SliderInt("##maxAudioTriggers",
							 reinterpret_cast<int *>(&global_config.maxAudioTriggers), 0, 10);

			// Button to stop all sounds
			ImGui::Dummy(ImVec2(0, 10));
			if (ImGui::Button("Stop Sounds", ImVec2(-1, 30))) AudioPlayer::stop_sounds();

			// Add padding before separators
			ImGui::Dummy(ImVec2(0, 10));

			// TTS settings
			ImGui::Separator();
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 -
								 ImGui::CalcTextSize("TTS Settings").x / 2);
			ImGui::Text("TTS Settings");
			ImGui::Separator();

			// Slider for TTS voice speed, which is a float from 0.1f to 2.0f
			ImGui::Text("TTS voice speed:");
			ImGui::SliderFloat("##ttsVoiceSpeed", &global_config.ttsVoiceSpeed, 0.1f, 2.0f, "%.1f");

			// Slider for TTS voice volume, which is a float from 0.0f to 1.0f
			ImGui::Text("TTS voice volume:");
			ImGui::SliderFloat("##ttsVoiceVolume", &global_config.ttsVoiceVolume, 0.0f, 1.0f,
							   "%.2f");

			// Slider for TTS voice pitch, which is a float from 0.1f to 2.0f
			ImGui::Text("TTS voice pitch:");
			ImGui::SliderFloat("##ttsVoicePitch", &global_config.ttsVoicePitch, 0.1f, 2.0f, "%.1f");

			// Add padding before separators
			ImGui::Dummy(ImVec2(0, 10));

			// Dividers + Title for Twitch settings
			ImGui::Separator();
			// Center the text horizontally
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 -
								 ImGui::CalcTextSize("Twitch Settings").x / 2);
			ImGui::Text("Twitch Settings");
			ImGui::Separator();

			// Multiselect for enabledCooldowns bitmask (eNone, eGlobal, ePerUser, ePerCommand)
			ImGui::Text("Enabled cooldowns:");
			ImGui::CheckboxFlags("Global", &*global_config.enabledCooldowns,
								 static_cast<unsigned int>(CommandCooldownType::eGlobal));
			ImGui::SameLine();
			ImGui::CheckboxFlags("Per User", &*global_config.enabledCooldowns,
								 static_cast<unsigned int>(CommandCooldownType::ePerUser));
			ImGui::SameLine();
			ImGui::CheckboxFlags("Per Command", &*global_config.enabledCooldowns,
								 static_cast<unsigned int>(CommandCooldownType::ePerCommand));

			// Input box for cooldown time
			ImGui::BeginDisabled(global_config.enabledCooldowns == CommandCooldownType::eNone);
			{
				ImGui::Text("Cooldown time:");
				ImGui::InputScalar("##cooldownTime", ImGuiDataType_U32,
								   &global_config.cooldownTime);
			}
			ImGui::EndDisabled();

			// Padding
			ImGui::Dummy(ImVec2(0, 10));

			// Modifiable list of approved users
			ImGui::Text("Approved users:");
			static std::array<char, 32> user_buf = {""};
			if (ImGui::Button("Add")) {
				if (user_buf[0] != '\0') {
					// Add the user to the list
					global_config.approvedUsers.emplace_back(user_buf.data());
					// Clear the input buffer
					std::ranges::fill(user_buf, '\0');
				}
			}
			ImGui::SameLine();
			ImGui::InputText("##approvedUsers", user_buf.data(), user_buf.size());

			// List of approved users, if empty, show warning text
			if (global_config.approvedUsers.empty())
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
								   "If the list is empty,\nanyone can trigger commands!");
			else {
				// List inside child window for scrolling
				ImGui::BeginChild("##approvedUsersList", ImVec2(0, 100), true);
				for (const auto &user : global_config.approvedUsers) {
					ImGui::PushID(&user);
					if (ImGui::Button("Remove")) {
						// Remove the user from the list
						std::erase(global_config.approvedUsers, user);
					}
					ImGui::PopID();
					ImGui::SameLine();
					ImGui::Text("%s", user.c_str());
				}
				ImGui::EndChild();
			}

			// Padding
			ImGui::Dummy(ImVec2(0, 10));

			// Dropdown of command_map keys, inputbox and button for changing the command call str
			ImGui::Text("Choose command:");
			static int selectedCommand = 0;
			const auto it = std::next(CommandHandler::get_commands_map().begin(), selectedCommand);
			// Checkbox for enabling/disabling the command
			auto enabled = it->second.enabled;
			if (ImGui::Checkbox("##cmdEnabled", &enabled))
				CommandHandler::set_command_enabled(it->first, enabled);

			ImGui::SameLine();
			// Dropdown box of "cmdDescription [!cmdKey]" for each command
			ImGui::Combo("##commandsDrop", &selectedCommand,
						 std::accumulate(CommandHandler::get_commands_map().begin(),
										 CommandHandler::get_commands_map().end(), std::string{},
										 [](const auto &acc, const auto &pair) {
											 return acc + pair.second.description + " [" +
													pair.second.callstr + "]" + '\0';
										 })
							 .c_str());

			// On same line, have test button
			ImGui::SameLine();
			ImGui::BeginDisabled(!it->second.enabled);
			if (ImGui::Button("Test")) CommandHandler::test_command(it->first);

			ImGui::EndDisabled();

			// Button for setting the new keyword
			static std::array<char, 16> cmd_change_buf = {""};
			ImGui::BeginDisabled(cmd_change_buf[0] == '\0');
			if (ImGui::Button("Set as new keyword")) {
				// If the input is not empty, set the new keyword
				if (cmd_change_buf[0] != '\0') {
					CommandHandler::change_command_call(it->first, cmd_change_buf.data());
					// Clear the input buffer
					std::ranges::fill(cmd_change_buf, '\0');
				}
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			// Input box for changing the command keyword, hint is selected command current callstr
			ImGui::InputTextWithHint("##cmdChange", it->second.callstr.c_str(),
									 cmd_change_buf.data(), cmd_change_buf.size());

			// Padding
			ImGui::Dummy(ImVec2(0, 10));

			// Input boxes for connection
			ImGui::Text("Twitch Auth Token:");
			ImGui::InputText("##authToken", &global_config.twitchAuthToken,
							 ImGuiInputTextFlags_Password);

			ImGui::Text("Twitch Auth User:");
			ImGui::InputText("##authUser", &global_config.twitchAuthUser,
							 ImGuiInputTextFlags_Password);

			ImGui::Text("Twitch Channel:");
			ImGui::InputText("##channel", &global_config.twitchChannel);

			// Padding
			ImGui::Dummy(ImVec2(0, 10));

			// Connect button
			static auto connResult = Result();
			const auto connStatus = TwitchChatConnector::get_connection_status();
			const auto buttonText =
				connStatus == ConnectionStatus::eConnected ? "Disconnect" : "Connect";
			const auto textColor = get_connection_status_color(connStatus, connResult);

			const auto fieldsFilled = !global_config.twitchAuthToken.empty() &&
									  !global_config.twitchAuthUser.empty() &&
									  !global_config.twitchChannel.empty();
			ImGui::BeginDisabled(connStatus == ConnectionStatus::eConnecting || !fieldsFilled);

			if (ImGui::Button(buttonText, ImVec2(-1, 30))) {
				if (connStatus == ConnectionStatus::eConnected)
					TwitchChatConnector::disconnect();
				else {
					// Allow only if all fields are filled
					if (fieldsFilled) connResult = TwitchChatConnector::connect();
				}
			}

			ImGui::TextColored(textColor, "Connection Status: [%s]",
							   get_connection_status_string(connStatus, connResult).c_str());

			ImGui::EndDisabled();

			// End window
			ImGui::End();
		}

		// NOTIFICATIONS //
		{
			// Remove notifications that have lived their lifetime
			std::erase_if(m_notifications, [](const auto &notif) { return notif->is_dead(); });

			// Render notifications
			for (const auto &notif : m_notifications) notif->render(m_notifFont);
		}

		// IMGUI RENDERING //
		ImGui::Render();
		int display_w = 0, display_h = 0;
		glfwGetFramebufferSize(m_mainWindow, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			// Backup context
			const auto prev_ctx = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(prev_ctx);
		}

		// GLFW SWAP BUFFERS //
		glfwSwapBuffers(m_mainWindow);
	}

	// Method for launching new notification
	static void launch_notification(const std::string &notifStr, const TwitchChatMessage &msg) {
		m_notifications.emplace_back(std::make_unique<Notification>(notifStr, msg));
	}

private:
	// Returns string depending on connection status and result
	static auto get_connection_status_string(const ConnectionStatus status, const Result &res)
		-> std::string {
		switch (status) {
		case ConnectionStatus::eConnecting:
			return "Connecting..";
		case ConnectionStatus::eConnected:
			return "Connected!";
		case ConnectionStatus::eDisconnected:
			return "Disconnected";
		default:
			return std::format("Error: {}", res.message);
		}
	}

	// Returns color depending on connection status and result
	static auto get_connection_status_color(const ConnectionStatus status, const Result &res)
		-> ImVec4 {
		switch (status) {
		case ConnectionStatus::eConnecting:
			return m_colorWait;
		case ConnectionStatus::eConnected:
			return m_colorOK;
		case ConnectionStatus::eDisconnected:
			return m_colorDisabled;
		default:
			return m_colorError;
		}
	}

	// GLFW error callback using format
	static void glfw_error_callback(int error, const char *description) {
		// Ignore GLFW_FEATURE_UNAVAILABLE as they're expected with Wayland
		if (error != GLFW_FEATURE_UNAVAILABLE)
			std::cerr << std::format("GLFW error {}: {}", error, description);
	}

	// Method that provides better glGetString, returns const char* instead of GLubyte*
	static auto get_gl_string(const GLenum name) -> const char * {
		return reinterpret_cast<const char *>(glGetString(name));
	}
};
