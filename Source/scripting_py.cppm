module;

#include <pybind11/embed.h>
#include <pybind11/stl.h>

export module pythonmodule;

namespace py = pybind11;

import assets;
import audio;
import types;

/* Assets */
static auto Py_Assets_getAssetsPath() -> std::string {
	return AssetsHandler::get_assets_path().string();
}

static auto Py_Assets_getFontAssetsPath() -> std::string {
	return AssetsHandler::get_font_assets_path().string();
}

static auto Py_Assets_getTTSAssetsPath() -> std::string {
	return AssetsHandler::get_tts_assets_path().string();
}

static auto Py_Assets_getTriggerASCIIPath() -> std::string {
	return AssetsHandler::get_trigger_ascii_path().string();
}

static auto Py_Assets_getTriggerSoundsPath() -> std::string {
	return AssetsHandler::get_trigger_sounds_path().string();
}

/* Audio */
static auto Py_Audio_playOneshot(const std::string &path) -> void {
	AudioPlayer::play_oneshot(std::filesystem::path(path));
}

static auto Py_Audio_playOneshotMemory(const std::vector<float> &data, std::uint32_t samplerate,
									   std::uint32_t channels) -> void {
	AudioPlayer::play_oneshot_memory(SoundData{data, samplerate, channels}, {});
}

/* Module */
PYBIND11_EMBEDDED_MODULE(chatnotifier, m) {
	py::class_<TwitchChatMessage>(m, "TwitchChatMessage")
		.def(py::init<std::string, std::string, std::string,
					  std::chrono::time_point<std::chrono::steady_clock>,
					  std::map<std::string, std::vector<std::string>>>())
		.def_readonly("user", &TwitchChatMessage::user)
		.def_readonly("message", &TwitchChatMessage::message)
		.def_readonly("command", &TwitchChatMessage::command)
		.def_readonly("time", &TwitchChatMessage::time)
		.def_readonly("args", &TwitchChatMessage::args);

	m.def("get_assets_path", &Py_Assets_getAssetsPath);
	m.def("get_font_assets_path", &Py_Assets_getFontAssetsPath);
	m.def("get_tts_assets_path", &Py_Assets_getTTSAssetsPath);
	m.def("get_ascii_assets_path", &Py_Assets_getTriggerASCIIPath);
	m.def("get_sound_assets_path", &Py_Assets_getTriggerSoundsPath);

	m.def("play_oneshot_file", &Py_Audio_playOneshot);
	m.def("play_oneshot_memory", &Py_Audio_playOneshotMemory);
}
