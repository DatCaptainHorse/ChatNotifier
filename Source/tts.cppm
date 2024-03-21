module;

#include <piper.hpp>

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <optional>
#include <functional>
#include <filesystem>

export module tts;

import config;
import common;
import assets;
import audio;

// Simple TTS module
export class TTSHandler {
	static inline piper::PiperConfig m_config;
	static inline std::shared_ptr<piper::Voice> m_voice;

public:
	static auto initialize() -> Result {
		const auto model = AssetsHandler::get_tts_model_path() / "default.onnx";
		std::optional<piper::SpeakerId> speakerId = std::nullopt;
		m_voice = std::make_shared<piper::Voice>();
		m_config.useESpeak = false;
		piper::loadVoice(m_config, model.string(), model.string() + ".json", *m_voice, speakerId);
		m_voice->phonemizeConfig.phonemeType = piper::PhonemeType::TextPhonemes;
		piper::initialize(m_config);
		return Result();
	}

	static void cleanup() {
		piper::terminate(m_config);
		m_voice = nullptr;
		if (std::filesystem::exists(get_temp_audio_path()))
			std::filesystem::remove(get_temp_audio_path());
	}

	static void voiceString(const std::string &text) {
		piper::SynthesisResult result;
		std::ofstream audiotemp(get_temp_audio_path().string(), std::ios::binary);
		piper::textToWavFile(m_config, *m_voice, text, audiotemp, result);
		audiotemp.close();
		AudioPlayer::play_oneshot(get_temp_audio_path(), 0.3f);
	}

private:
	static auto get_temp_audio_path() -> std::filesystem::path {
		return AssetsHandler::get_exec_path() / "temp.wav";
	}
};
