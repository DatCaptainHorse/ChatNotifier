module;

#include <sherpa-onnx/c-api/c-api.h>

#include <string>
#include <vector>
#include <thread>
#include <filesystem>

export module tts;

import config;
import common;
import assets;

export struct TTSData {
	std::vector<float> audio;
	std::int32_t sampleRate;
};

// Simple TTS module
export class TTSHandler {
	static inline SherpaOnnxOfflineTtsConfig m_config;
	static inline SherpaOnnxOfflineTts *m_tts;

public:
	static auto initialize() -> Result {
		// Zero-out the config
		m_config = {};
		m_config.model = {};
		m_config.model.vits = {};

		// Set the model paths
		const auto vitsModel = (AssetsHandler::get_tts_model_path() / "model.onnx").string();
		m_config.model.vits.model = vitsModel.c_str();
		const auto vitsTokens = (AssetsHandler::get_tts_model_path() / "tokens.txt").string();
		m_config.model.vits.tokens = vitsTokens.c_str();
		const auto vitsLexicon = (AssetsHandler::get_tts_model_path() / "lexicon/").string();
		m_config.model.vits.data_dir = vitsLexicon.c_str();

		m_config.max_num_sentences = 1;

		if (const auto maxThreads = std::thread::hardware_concurrency(); maxThreads >= 12)
			m_config.model.num_threads = static_cast<std::int32_t>(maxThreads / 4);
		else
			m_config.model.num_threads = static_cast<std::int32_t>(maxThreads / 2);

		m_config.model.vits.length_scale = 1.0f;
		m_config.model.vits.noise_scale = 0.667f;
		m_config.model.vits.noise_scale_w = 0.8f;

		m_tts = SherpaOnnxCreateOfflineTts(&m_config);

		return Result();
	}

	static void cleanup() {
		// Destroy the TTS engine
		SherpaOnnxDestroyOfflineTts(m_tts);
	}

	static auto get_num_voices() -> std::int32_t { return SherpaOnnxOfflineTtsNumSpeakers(m_tts); }

	static auto voiceString(const std::string &text, std::int32_t speakerID = -1,
							const float voiceSpeed = 1.0f) -> TTSData {
		if (speakerID == -1 || speakerID >= get_num_voices())
			speakerID = random_int(0, get_num_voices() - 1);

		const auto audio = SherpaOnnxOfflineTtsGenerate(m_tts, text.c_str(), speakerID, voiceSpeed);
		auto data = TTSData({audio->samples, audio->samples + audio->n}, audio->sample_rate);
		SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
		return data;
	}
};
