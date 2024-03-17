module;

#define GLFW_INCLUDE_NONE // Don't include OpenGL headers, we are using gl3w

#include <gl3w/GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <fmt/format.h>

#include <algorithm>
#include <numeric>
#include <vector>
#include <array>

export module gui;

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

	static inline float m_notifAnimationLength;
	static inline float m_notifEffectSpeed;
	static inline std::vector<std::string> m_approvedUsers;

	// Vector of live-notifications
	static inline std::vector<std::unique_ptr<Notification>> m_notifications;

public:
	// Initialize the GUI and it's resources
	static void initialize() {
		// Set default notification show time to 5 seconds
		m_notifAnimationLength = 5.0f;
		// Default effect speed multiplier to 5
		m_notifEffectSpeed = 5.0f;

		// GLFW INITIALIZATION //
		glfwSetErrorCallback(glfw_error_callback);
		// If on linux, force X11 as glfw lacks proper wayland support
#ifdef __linux__
		glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
		if (!glfwInit())
			return;

		// WINDOW CREATION //
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		// Dummy window since it's not as flexible as ImGui viewport windows
		m_mainWindow = glfwCreateWindow(1, 1, "ChatNotifier", nullptr, nullptr);
		if (!m_mainWindow)
			return;

		glfwMakeContextCurrent(m_mainWindow);
		glfwSwapInterval(1); // V-Sync

		const auto monitor = glfwGetPrimaryMonitor();
		const auto mode = glfwGetVideoMode(monitor);

		// GL3W INITIALIZATION //
		if (gl3wInit()) {
			fmt::println(stderr, "Failed to initialize GL3W");
			return;
		}
		if (!gl3wIsSupported(3, 3)) {
			fmt::println(stderr, "OpenGL 3.3 not supported");
			return;
		}

		// Print OpenGL version
		fmt::println("OpenGL Version: {}", get_gl_string(GL_VERSION));

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
		fmt::println("DPI: {}, Main font size: {}, Notification font size: {}", m_DPI, mainFontSize,
					 notifFontSize);

		// NotoSansMono.ttf for main text
		if (AssetsHandler::get_font_exists("NotoSansMono.ttf")) {
			m_mainFont = io.Fonts->AddFontFromFileTTF(
				AssetsHandler::get_font_path("NotoSansMono.ttf").c_str(),
				static_cast<float>(mainFontSize), nullptr, io.Fonts->GetGlyphRangesDefault());

			// Set as default font
			io.FontDefault = m_mainFont;
		} else {
			// We require this font
			fmt::print(stderr, "Required font NotoSansMono.ttf not found!");
			return;
		}

		// NotoSansSymbols2.ttf for notifications
		// Contains special glyps so we need to tell ImGui about it
		if (AssetsHandler::get_font_exists("NotoSansSymbols2.ttf")) {
			const std::array<ImWchar, 5> notif_ranges = {0x0020, 0x00FF, 0x2800, 0x28FF, 0};
			m_notifFont = io.Fonts->AddFontFromFileTTF(
				AssetsHandler::get_font_path("NotoSansSymbols2.ttf").c_str(),
				static_cast<float>(notifFontSize), nullptr, notif_ranges.data());
		} else {
			// NOT strictly required, warn about lacking symbols and just set notif font to main
			// font
			fmt::print(stderr, "Special symbols font NotoSansSymbols2.ttf not found, using main "
							   "font for notifications");
			m_notifFont = m_mainFont;
		}

		// MAIN WINDOW IMGUI INITIALIZATION //
		ImGui_ImplGlfw_InitForOpenGL(m_mainWindow, true);
		ImGui_ImplOpenGL3_Init("#version 330 core");

		// Build fonts
		io.Fonts->Build();

		// Ready to run
		m_keepRunning = true;
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
	[[nodiscard]] static auto should_close() -> bool { return !m_keepRunning; }

	// GUI drawing and updating
	static void render() {
		// GLFW POLL EVENTS //
		glfwPollEvents();

		// IMGUI NEW FRAME //
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Create invisible "root", transparent window for all other windows to merge
		const auto monitor = ImGui::GetViewportPlatformMonitor(ImGui::GetMainViewport());
		ImGui::SetNextWindowPos(monitor->MainPos, ImGuiCond_Once);
		ImGui::SetNextWindowSize(monitor->MainSize, ImGuiCond_Once);
		ImGui::Begin("##rootWindow", nullptr,
					 ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground |
						 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoFocusOnAppearing |
						 ImGuiWindowFlags_NoDocking);

		// Set root viewport settings
		const auto rootViewport = ImGui::GetWindowViewport();
		rootViewport->Flags |= ImGuiViewportFlags_TransparentClearColor;
		rootViewport->Flags |= ImGuiViewportFlags_TopMost;
		rootViewport->Flags |= ImGuiViewportFlags_NoInputs;
		rootViewport->Flags |= ImGuiViewportFlags_NoFocusOnAppearing;
		rootViewport->Flags |= ImGuiViewportFlags_NoFocusOnClick;
		rootViewport->Flags |= ImGuiViewportFlags_NoTaskBarIcon;
		rootViewport->Flags |= ImGuiViewportFlags_NoDecoration;

		// CONTROL WINDOW //
		{
			ImGui::SetNextWindowSize(ImVec2(420, 690), ImGuiCond_Once);
			ImGui::Begin("Chat Notifier Controls", &m_keepRunning,
						 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

			// Have control window as separate viewport
			ImGui::GetWindowViewport()->Flags |= ImGuiViewportFlags_NoAutoMerge;

			// Settings portion, separators n stuff
			ImGui::Separator();
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 -
								 ImGui::CalcTextSize("Notification Settings").x / 2);
			ImGui::Text("Notification Settings");
			ImGui::Separator();

			// Slider for notification show time, integer-steps from 1 to 10
			ImGui::Text("Notification show time:");
			ImGui::SliderFloat("##showTime", &m_notifAnimationLength, 1.0f, 10.0f, "%.0f");

			// Slider for notification effect speed, float from 0.1 to 10.0
			ImGui::Text("Notification effect speed:");
			ImGui::SliderFloat("##effectSpeed", &m_notifEffectSpeed, 0.1f, 10.0f, "%.1f");

			// Slider for notification font scale, float from 0.5 to 2.0
			ImGui::Text("Notification font scale:");
			ImGui::SliderFloat("##fontScale", &m_notifFont->Scale, 0.5f, 2.0f, "%.1f");

			// Slider for global audio volume, which is a float from 0.0f to 1.0f
			// Volume inputslider text formatting as 0 to 100% instead of 0.0 to 1.0
			ImGui::Text("Global audio volume:");
			static float global_vol_buf = AudioPlayer::get_global_volume() * 100.0f;
			if (ImGui::SliderFloat("##globalVol", &global_vol_buf, 0.0f, 100.0f, "%.0f%%")) {
				AudioPlayer::set_global_volume(global_vol_buf / 100.0f);
			}

			// Add padding before separators
			ImGui::Dummy(ImVec2(0, 10));
			// Dividers + Title for Twitch settings
			ImGui::Separator();
			// Center the text horizontally
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 -
								 ImGui::CalcTextSize("Twitch Settings").x / 2);
			ImGui::Text("Twitch Settings");
			ImGui::Separator();

			// Modifiable list of approved users
			ImGui::Text("Approved users:");
			static std::array<char, 32> user_buf = {""};
			if (ImGui::Button("Add")) {
				if (user_buf[0] != '\0') {
					// Add the user to the list
					m_approvedUsers.emplace_back(user_buf.data());
					// Clear the input buffer
					std::ranges::fill(user_buf, '\0');
				}
			}
			ImGui::SameLine();
			ImGui::InputText("##approvedUsers", user_buf.data(), user_buf.size());

			// List of approved users, if empty, show warning text
			if (m_approvedUsers.empty())
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
								   "If the list is empty,\nanyone can trigger commands!");
			else {
				for (const auto &user : m_approvedUsers) {
					ImGui::PushID(&user);
					if (ImGui::Button("Remove")) {
						// Remove the user from the list
						std::erase(m_approvedUsers, user);
					}
					ImGui::PopID();
					ImGui::SameLine();
					ImGui::Text("%s", user.c_str());
				}
			}

			// Padding
			ImGui::Dummy(ImVec2(0, 10));

			// Dropdown of command_map keys, inputbox and button for changing the command
			// keyword
			ImGui::Text("Choose command:");
			static int selectedCommand = 0;
			// Dropdown box of "cmdDescription [!cmdKey]" for each command
			ImGui::Combo("##commandsDrop", &selectedCommand,
						 std::accumulate(CommandHandler::get_commands_map().begin(),
										 CommandHandler::get_commands_map().end(), std::string(),
										 [](std::string acc, const auto &tuple) {
											 return acc + tuple.first + " [" + tuple.first + "]" +
													'\0';
										 })
							 .c_str());
			// On same line, have test button
			ImGui::SameLine();
			const auto it = std::next(CommandHandler::get_commands_map().begin(), selectedCommand);
			if (ImGui::Button("Test")) {
				std::string extraWord = "Message";
				// If there are any ascii art files or egg word sounds, choose one of them
				// randomly
				if (random_int(0, 1) == 0) {
					if (const auto asciis = AssetsHandler::get_ascii_art_keys(); !asciis.empty()) {
						extraWord = asciis[random_int(0, asciis.size() - 1)];
					}
				} else {
					if (const auto eggsounds = AssetsHandler::get_egg_sound_keys();
						!eggsounds.empty()) {
						extraWord = eggsounds[random_int(0, eggsounds.size() - 1)];
					}
				}
				CommandHandler::execute_command(it->first, fmt::format("Test {}", extraWord));
			}

			// Button for setting the new keyword
			static std::array<char, 16> cmd_change_buf = {""};
			if (ImGui::Button("Set as keyword")) {
				// If the input is not empty, set the new keyword
				if (cmd_change_buf[0] != '\0') {
					CommandHandler::change_command_key(it->first, cmd_change_buf.data());
					// Clear the input buffer
					std::ranges::fill(cmd_change_buf, '\0');
				}
			}
			ImGui::SameLine();
			// Input box for changing the command keyword, hint is selected command current key
			ImGui::InputTextWithHint("##cmdChange", it->first.c_str(), cmd_change_buf.data(),
									 cmd_change_buf.size());

			// Padding
			ImGui::Dummy(ImVec2(0, 10));

			// Input boxes for connection
			ImGui::Text("Twitch Auth Token:");
			static std::array<char, 64> auth_buf = {""};
			ImGui::InputText("##authToken", auth_buf.data(), auth_buf.size(),
							 ImGuiInputTextFlags_Password);

			ImGui::Text("Twitch Auth User:");
			static std::array<char, 32> authuser_buf = {""};
			ImGui::InputText("##authUser", authuser_buf.data(), authuser_buf.size(),
							 ImGuiInputTextFlags_Password);

			ImGui::Text("Twitch Channel:");
			static std::array<char, 32> channel_buf = {""};
			ImGui::InputText("##channel", channel_buf.data(), channel_buf.size());

			// Padding
			ImGui::Dummy(ImVec2(0, 10));

			// Connect button
			const auto connStatus = TwitchChatConnector::get_connection_status();
			if (connStatus == ConnectionStatus::eConnecting)
				ImGui::BeginDisabled();

			if (connStatus < ConnectionStatus::eConnected) {
				if (ImGui::Button("Connect", ImVec2(-1, 30))) {
					// Allow only if all fields are filled
					if (auth_buf[0] != '\0' && authuser_buf[0] != '\0' && channel_buf[0] != '\0') {
						TwitchChatConnector::connect(auth_buf.data(), authuser_buf.data(),
													 channel_buf.data());
					}
				}
			} else {
				if (ImGui::Button("Disconnect", ImVec2(-1, 30))) {
					TwitchChatConnector::disconnect();
				}
			}

			if (connStatus == ConnectionStatus::eConnecting)
				ImGui::EndDisabled();

			// Connection status (gray for disconnected, green for connected, orange for connecting)
			ImGui::TextColored(
				connStatus == ConnectionStatus::eDisconnected ? ImVec4(0.5f, 0.5f, 0.5f, 1.0f)
				: connStatus == ConnectionStatus::eConnected  ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f)
															  : ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
				"Connection status: %s",
				connStatus == ConnectionStatus::eDisconnected ? "Disconnected"
				: connStatus == ConnectionStatus::eConnected  ? "Connected"
															  : "Connecting");

			// End window
			ImGui::End();
		}

		// NOTIFICATIONS //
		{
			// Remove notifications that have lived their lifetime
			std::erase_if(m_notifications, [](const auto &notif) { return notif->is_dead(); });

			// Render notifications
			for (const auto &notif : m_notifications)
				notif->render(m_notifFont);
		}

		// End root window
		ImGui::End();

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
	static void launch_notification(std::string text) {
		m_notifications.emplace_back(
			std::make_unique<Notification>(std::move(text), m_notifAnimationLength,
										   static_cast<float>(glfwGetTime()), m_notifEffectSpeed));
	}

	// Method to return approved users
	[[nodiscard]] static auto get_approved_users() -> const std::vector<std::string> & {
		return m_approvedUsers;
	}

private:
	// GLFW error callback using fmt
	static void glfw_error_callback(int error, const char *description) {
		fmt::println(stderr, "Glfw Error {}: {}", error, description);
	}

	// Method that provides better glGetString, returns const char* instead of GLubyte*
	static auto get_gl_string(const GLenum name) -> const char * {
		return reinterpret_cast<const char *>(glGetString(name));
	}
};
