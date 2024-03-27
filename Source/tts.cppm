module;

#include <sherpa-onnx/c-api/c-api.h>

#include <deque>
#include <string>
#include <vector>
#include <thread>
#include <filesystem>

export module tts;

import config;
import common;
import assets;
import audio;

// Simple TTS module
export class TTSHandler {
	static inline SherpaOnnxOfflineTtsConfig m_config;
	static inline SherpaOnnxOfflineTts *m_tts;
	static inline std::deque<std::thread> m_threads;

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
			m_config.model.num_threads = maxThreads / 4;
		else
			m_config.model.num_threads = maxThreads / 2;

		m_config.model.vits.length_scale = 1.0f;
		m_config.model.vits.noise_scale = 0.667f;
		m_config.model.vits.noise_scale_w = 0.8f;

		m_tts = SherpaOnnxCreateOfflineTts(&m_config);

		return Result();
	}

	static void cleanup() {
		for (auto it = m_threads.begin(); it != m_threads.end();) {
			if (it->joinable()) {
				it->join();
				it = m_threads.erase(it);
			}
		}

		SherpaOnnxDestroyOfflineTts(m_tts);
	}

	static void update() {
		// Cleanup finished threads
		for (auto it = m_threads.begin(); it != m_threads.end();) {
			if (it->joinable()) {
				it->join();
				it = m_threads.erase(it);
			}
		}
	}

	static auto get_num_voices() -> std::int32_t { return SherpaOnnxOfflineTtsNumSpeakers(m_tts); }

	static void voiceString(const std::string &text, std::int32_t speakerID = -1,
							float voiceSpeed = 1.0f) {
		if (speakerID == -1 || speakerID >= get_num_voices())
			speakerID = random_int(0, get_num_voices() - 1);

		// Do in separate thread
		m_threads.emplace_back([text, speakerID, voiceSpeed]() {
			const auto audio =
				SherpaOnnxOfflineTtsGenerate(m_tts, text.c_str(), speakerID, voiceSpeed);
			AudioPlayer::play_oneshot_memory({audio->samples, audio->samples + audio->n}, 22000,
											 global_config.ttsVoiceVolume);
			SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
		});
	}
};
