module;

#ifndef CN_SUPPORTS_MODULES_STD
#include <standard.hpp>
#endif

#include <pybind11/embed.h>
#include <pybind11/stl.h>

#include <hv/json.hpp>
#include <hv/requests.h>

#define GLFW_INCLUDE_NONE
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui_impl_glfw.hpp>
#include <imgui/imgui_impl_glad.hpp>

#include <napi.h>

export module napi;

import standard;
import types;
import config;
import common;
import assets;
import audio;
import opengl;
import gui;
import twitch;
import commands;
import filesystem;
import scripting;
import runner;

Runner main_runner;
bool cn_initialized = false;

void print_error(const Result &res) {
	if (!res) std::println("Error: {}", res.message);
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

namespace libchatnotifier {
	Napi::Boolean printerWrapped(const Napi::CallbackInfo &info) {
		const auto env = info.Env();
		if (info.Length() >= 1 && info[0].IsString()) {
			const auto str = info[0].As<Napi::String>().Utf8Value();
			std::println("{}", str);
		}
		return Napi::Boolean::New(env, true);
	}

	auto initialize(const int argc, char **argv) -> int {
		if (cn_initialized) return 0;

		std::println("Initializing libchatnotifier");
		// Make sure UTF-8 is used
		std::locale::global(std::locale("en_US.UTF-8"));

		// Initialize filesystem to get the root path first
		std::println("Filesystem with argc: {} and argv: {}", argc, argv[0]);
		if (const auto res = Filesystem::initialize(argc, argv); !res) {
			print_error(res);
			return res.code;
		}

		// LOAD CONFIG //
		std::println("Config load");
		if (const auto res = global_config.load(); !res) {
			print_error(res);
			// Continue despite error, as might be outdated config
		}

		// INITIALIZE //
		std::println("ScriptingHandler initialize");
		if (const auto res = ScriptingHandler::initialize(); !res) {
			print_error(res);
			return res.code;
		}

		std::println("AssetsHandler initialize");
		if (const auto res = AssetsHandler::initialize(); !res) {
			print_error(res);
			return res.code;
		}

		std::println("AudioPlayer initialize");
		if (const auto res = AudioPlayer::initialize(); !res) {
			print_error(res);
			return res.code;
		}

		std::println("TwitchChatConnector initialize");
		if (const auto res = TwitchChatConnector::initialize(twc_callback_handler); !res) {
			print_error(res);
			return res.code;
		}

		main_runner.add_job_sync([&]() -> void {
			std::println("OpenGLHandler initialize");
			if (const auto res = OpenGLHandler::initialize(NotifierGUI::render); !res) {
				print_error(res);
				return;
			}

			std::println("NotifierGUI initialize");
			if (const auto res = NotifierGUI::initialize(); !res) {
				print_error(res);
				return;
			}
		});

		std::println("CommandHandler initialize");
		if (const auto res = CommandHandler::initialize(NotifierGUI::launch_notification); !res) {
			print_error(res);
			return res.code;
		}

		// Add scripts
		std::println("Add scripts");
		ScriptingHandler::refresh_scripts([] {
			for (const auto &script : ScriptingHandler::get_scripts()) {
				if (script->has_method("on_message")) {
					CommandHandler::add_command(
						script->get_name(),
						Command(script->get_call_string(), script->get_call_string(),
								[script](const TwitchChatMessage &msg) {
									ScriptingHandler::execute_script_method(script, "on_message",
																			msg);
								}));
				}
			}
		});

		std::println("Initialized!");
		cn_initialized = true;

		return 0;
	}
	void runner() {
		if (!cn_initialized) return;
		while (!NotifierGUI::should_close()) {
			// Poll events
			glfwPollEvents();
			// Render
			OpenGLHandler::render();
			// Update the audio
			AudioPlayer::update();
			// Sleep for 5ms to lighten the load on the CPU
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	}
	Napi::Number initializeWrapped(const Napi::CallbackInfo &info) {
		const auto env = info.Env();
		if (info.Length() >= 1 && info[0].IsString()) {
			std::println("Called initialize libchatnotifier");
			const auto argv = info[0].As<Napi::String>().Utf8Value();
			std::array argv_array = {argv.data()};
			const auto result = initialize(1, const_cast<char **>(argv_array.data()));
			if (result == 0) main_runner.add_job(runner);
			return Napi::Number::New(env, result);
		}
		Napi::Error::New(env, "Invalid arguments").ThrowAsJavaScriptException();
		return Napi::Number::New(env, -1);
	}

	void cleanup() {
		NotifierGUI::cleanup();
		OpenGLHandler::cleanup();
		CommandHandler::cleanup();
		ScriptingHandler::cleanup();
		TwitchChatConnector::cleanup();
		AudioPlayer::cleanup();
		AssetsHandler::cleanup();
	}
	Napi::Boolean cleanupWrapped(const Napi::CallbackInfo &info) {
		const auto env = info.Env();
		cleanup();
		return Napi::Boolean::New(env, true);
	}

	auto save_config() -> int {
		if (const auto res = global_config.save(); !res) {
			print_error(res);
			return res.code;
		}
		return 0;
	}
	Napi::Number save_configWrapped(const Napi::CallbackInfo &info) {
		const auto env = info.Env();
		const auto res = save_config();
		return Napi::Number::New(env, res);
	}

	auto get_config_json() -> std::string { return global_config.to_json().dump(); }
	Napi::String get_config_jsonWrapped(const Napi::CallbackInfo &info) {
		const auto env = info.Env();
		const auto res = get_config_json();
		return Napi::String::New(env, res);
	}

	void set_config_json(const std::string &json) { global_config.from_json_string(json); }
	Napi::Boolean set_config_jsonWrapped(const Napi::CallbackInfo &info) {
		const auto env = info.Env();
		if (info.Length() >= 1 && info[0].IsString()) {
			const auto json = info[0].As<Napi::String>().Utf8Value();
			set_config_json(json);
			return Napi::Boolean::New(env, true);
		}
		return Napi::Boolean::New(env, false);
	}

	Napi::Boolean connect_twitchWrapped(const Napi::CallbackInfo &info) {
		const auto env = info.Env();
		if (const auto res = TwitchChatConnector::connect(); !res) {
			print_error(res);
			return Napi::Boolean::New(env, false);
		}
		return Napi::Boolean::New(env, true);
	}
	Napi::Boolean disconnect_twitchWrapped(const Napi::CallbackInfo &info) {
		const auto env = info.Env();
		TwitchChatConnector::disconnect();
		return Napi::Boolean::New(env, true);
	}

	Napi::Boolean initializedWrapped(const Napi::CallbackInfo &info) {
		const auto env = info.Env();
		return Napi::Boolean::New(env, cn_initialized);
	}

	Napi::String twitch_connection_statusWrapped(const Napi::CallbackInfo &info) {
		const auto env = info.Env();
		std::string str;
		switch (TwitchChatConnector::get_connection_status()) {
		case ConnectionStatus::eConnected:
			str = "Connected";
			break;
		case ConnectionStatus::eDisconnected:
			str = "Disconnected";
			break;
		case ConnectionStatus::eConnecting:
			str = "Connecting";
			break;
		case ConnectionStatus::eError:
			str = "Error";
			break;
		default:
			str = "Unknown";
			break;
		}
		return Napi::String::New(env, str);
	}

	Napi::Value stop_all_soundsWrapped(const Napi::CallbackInfo &info) {
		const auto env = info.Env();
		AudioPlayer::stop_sounds();
		return env.Undefined();
	}

	Napi::Value find_new_assetsWrapped(const Napi::CallbackInfo &info) {
		const auto env = info.Env();
		AssetsHandler::refresh();
		return env.Undefined();
	}

	Napi::Value reload_scriptsWrapped(const Napi::CallbackInfo &info) {
		const auto env = info.Env();
		ScriptingHandler::refresh_scripts([] {
			for (const auto &script : ScriptingHandler::get_scripts()) {
				if (script->has_method("on_message")) {
					CommandHandler::add_command(
						script->get_name(),
						Command(script->get_call_string(), script->get_call_string(),
								[script](const TwitchChatMessage &msg) {
									ScriptingHandler::execute_script_method(script, "on_message",
																			msg);
								}));
				}
			}
		});
		return env.Undefined();
	}

	Napi::Object Init(const Napi::Env env, const Napi::Object exports) {
		exports.Set("printer", Napi::Function::New(env, printerWrapped));
		exports.Set("initialize", Napi::Function::New(env, initializeWrapped));
		exports.Set("cleanup", Napi::Function::New(env, cleanupWrapped));
		exports.Set("save_config", Napi::Function::New(env, save_configWrapped));
		exports.Set("get_config_json", Napi::Function::New(env, get_config_jsonWrapped));
		exports.Set("set_config_json", Napi::Function::New(env, set_config_jsonWrapped));
		exports.Set("connect_twitch", Napi::Function::New(env, connect_twitchWrapped));
		exports.Set("disconnect_twitch", Napi::Function::New(env, disconnect_twitchWrapped));
		exports.Set("initialized", Napi::Function::New(env, initializedWrapped));
		exports.Set("get_twitch_connection_status",
					Napi::Function::New(env, twitch_connection_statusWrapped));
		exports.Set("stop_all_sounds", Napi::Function::New(env, stop_all_soundsWrapped));
		exports.Set("find_new_assets", Napi::Function::New(env, find_new_assetsWrapped));
		exports.Set("reload_scripts", Napi::Function::New(env, reload_scriptsWrapped));
		return exports;
	}
	NODE_API_MODULE(addon, Init)
} // namespace libchatnotifier