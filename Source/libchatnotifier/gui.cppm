module;

#ifndef CN_SUPPORTS_MODULES_STD
#include <standard.hpp>
#endif

#include <pybind11/embed.h>
#include <pybind11/stl.h>

#include <hv/json.hpp>
#include <hv/requests.h>
#include <hv/HttpServer.h>
#include <hv/WebSocketClient.h>

#define GLFW_INCLUDE_NONE				// Don't include OpenGL headers by default
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM // Same goes for ImGui
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

export module gui;

import standard;
import types;
import opengl;
import config;
import common;
import assets;
import audio;
import notification;
import twitch;
import commands;
import scripting;

// Class which manages the GUI + notifications
export class NotifierGUI {
	static inline bool m_keepRunning = false;
	static inline ImFont *m_mainFont;
	static inline ImFont *m_notifFont;
	static inline float m_DPI;

	// Vector of live-notifications
	static inline std::vector<std::unique_ptr<Notification>> m_notifications;
	static inline std::mutex m_notifMutex;

	static inline auto m_colorOK = ImVec4(0.0f, 0.8f, 0.0f, 1.0f);
	static inline auto m_colorError = ImVec4(0.8f, 0.0f, 0.0f, 1.0f);
	static inline auto m_colorWait = ImVec4(0.8f, 0.4f, 0.0f, 1.0f);
	static inline auto m_colorDisabled = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

public:
	// Initialize the GUI and it's resources
	static auto initialize() -> Result {
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
		const auto mode = OpenGLHandler::get_mode();
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
		ImGui_ImplGlfw_InitForOpenGL(OpenGLHandler::get_main_window(), true);
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
	}

	// Returns if the gui should close
	[[nodiscard]] static auto should_close() -> bool {
		return !m_keepRunning;
	}

	// GUI drawing and updating
	static void render() {
		// Backup context
		// const auto prev_ctx_main = glfwGetCurrentContext();
		// glfwMakeContextCurrent(m_guiWindow);

		// IMGUI NEW FRAME //
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// CONTROL WINDOW //
		/*{
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

			// On separate line, have message input box with test button
			static std::string testMsg = "Test message";
			ImGui::InputText("##testMsg", &testMsg);
			ImGui::SameLine();
			ImGui::BeginDisabled(!it->second.enabled);
			if (ImGui::Button("Test"))
				CommandHandler::execute_command(it->first,
												TwitchChatMessage("testButton", testMsg));

			ImGui::EndDisabled();

			// Input box for changing the command keyword, hint is selected command current callstr
			static std::string cmd_change_buf = "";
			ImGui::InputTextWithHint("##cmdChange", it->second.callstr.c_str(),
									 cmd_change_buf.data(), cmd_change_buf.size());
			ImGui::SameLine();
			// Button for setting the new keyword
			ImGui::BeginDisabled(cmd_change_buf.empty());
			if (ImGui::Button("Set keyword")) {
				// If the input is not empty, set the new keyword
				if (!cmd_change_buf.empty()) {
					CommandHandler::change_command_call(it->first, cmd_change_buf.c_str());
					// Clear the input buffer
					std::ranges::fill(cmd_change_buf, '\0');
				}
			}
			ImGui::EndDisabled();

			// End window
			ImGui::End();
		}*/

		// NOTIFICATIONS //
		{
			// Lock mutex for notifications
			std::scoped_lock lock(m_notifMutex);

			// Remove notifications that have lived their lifetime
			std::erase_if(m_notifications, [](const auto &notif) { return notif->is_dead(); });

			// Render notifications
			for (const auto &notif : m_notifications) notif->render(m_notifFont);
		}

		// IMGUI RENDERING //
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			// Backup context
			const auto prev_ctx = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(prev_ctx);
		}

		// GLFW SWAP BUFFERS //
		glfwSwapBuffers(OpenGLHandler::get_main_window());

		// Restore context
		// glfwMakeContextCurrent(prev_ctx_main);
	}

	// Method for launching new notification
	static void launch_notification(const std::string &notifStr, const TwitchChatMessage &msg) {
		// Lock mutex for notifications
		std::scoped_lock lock(m_notifMutex);
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
};
