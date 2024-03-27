module;

#include <sherpa-onnx/c-api/c-api.h>

#include <ranges>
#include <string>
#include <vector>
#include <thread>
#include <filesystem>

export module tts;

import config;
import common;
import assets;
import audio;

struct TTSData {
	std::vector<float> audio;
	std::int32_t sampleRate;
	float pitch;
	float x, y, z;
	std::thread::id owner;
};

// Simple TTS module
export class TTSHandler {
	static inline SherpaOnnxOfflineTtsConfig m_config;
	static inline SherpaOnnxOfflineTts *m_tts;
	static inline std::vector<std::thread> m_threads;
	static inline std::vector<TTSData> m_results;

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
		// Join all threads
		for (auto &thread : m_threads)
			if (thread.joinable()) thread.join();

		m_threads.clear();

		// Clear all results
		m_results.clear();

		SherpaOnnxDestroyOfflineTts(m_tts);
	}

	static void update() {
		std::vector<std::thread::id> clearableThreads;
		// Play results
		for (auto it = m_results.begin(); it != m_results.end();) {
			if (!it->audio.empty()) {
				AudioPlayer::play_oneshot_memory(it->audio, 22000, global_config.ttsVoiceVolume,
												 it->pitch, it->x, it->y, it->z);
				clearableThreads.push_back(it->owner);
				it = m_results.erase(it);
			} else
				++it;
		}

		// Join and remove finished threads
		if (!clearableThreads.empty()) {
			for (auto it = m_threads.begin(); it != m_threads.end();) {
				if (std::ranges::find(clearableThreads, it->get_id()) != clearableThreads.end() &&
					it->joinable())
					it->join();
				else
					++it;
			}
		}
	}

	static auto get_num_voices() -> std::int32_t { return SherpaOnnxOfflineTtsNumSpeakers(m_tts); }

	static void voiceString(const std::string &text, std::int32_t speakerID = -1,
							const float voiceSpeed = 1.0f, const float voicePitch = 1.0f,
							const float x = 0.0f, const float y = 0.0f, const float z = 0.0f) {
		if (speakerID == -1 || speakerID >= get_num_voices())
			speakerID = random_int(0, get_num_voices() - 1);

		// Do in separate thread
		m_threads.emplace_back([text, speakerID, voiceSpeed, voicePitch, x, y, z]() {
			const auto audio =
				SherpaOnnxOfflineTtsGenerate(m_tts, text.c_str(), speakerID, voiceSpeed);
			m_results.emplace_back(TTSData({audio->samples, audio->samples + audio->n},
										   audio->sample_rate, voicePitch, x, y, z));
			SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
		});
	}
};
