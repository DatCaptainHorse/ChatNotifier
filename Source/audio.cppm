module;

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_WAV
#define MA_NO_MP3
#define MA_NO_FLAC
#include <miniaudio.h>

#include <map>
#include <deque>
#include <array>
#include <vector>
#include <memory>
#include <string>
#include <ranges>
#include <filesystem>

export module audio;

import common;
import opus_decoder;

// Super-duper simple audio player
export class AudioPlayer {
	static inline ma_engine m_engine;
	static inline float m_volume;

	// Vector of sounds
	static inline std::map<std::string, std::deque<std::shared_ptr<ma_sound>>> m_sounds;

	static inline ma_decoding_backend_vtable decoding_backend_opus{
		ma_decoding_backend_init_libopus, ma_decoding_backend_init_file_libopus,
		nullptr, /* onInitFileW() */
		nullptr, /* onInitMemory() */
		ma_decoding_backend_uninit_libopus};

public:
	static auto initialize() -> Result {
		auto engineConfig = ma_engine_config_init();
		engineConfig.pResourceManager = new ma_resource_manager();
		if (engineConfig.pResourceManager == nullptr) {
			return Result(-1, "Failed to allocate memory for resource manager");
		}

		std::array pCustomBackendVTables = {&decoding_backend_opus};

		auto resourceManagerConfig = ma_resource_manager_config_init();
		resourceManagerConfig.pLog = engineConfig.pLog;
		resourceManagerConfig.decodedFormat = ma_format_f32;
		resourceManagerConfig.decodedChannels = 0;
		resourceManagerConfig.decodedSampleRate = engineConfig.sampleRate;
		resourceManagerConfig.allocationCallbacks = engineConfig.allocationCallbacks;
		resourceManagerConfig.pVFS = engineConfig.pResourceManagerVFS;
		resourceManagerConfig.ppCustomDecodingBackendVTables = pCustomBackendVTables.data();
		resourceManagerConfig.customDecodingBackendCount = pCustomBackendVTables.size();

		auto mares =
			ma_resource_manager_init(&resourceManagerConfig, engineConfig.pResourceManager);
		if (mares != MA_SUCCESS) {
			return Result(-1, "Failed to initialize resource manager: {}",
						  ma_result_description(mares));
		}

		mares = ma_engine_init(&engineConfig, &m_engine);
		if (mares != MA_SUCCESS) {
			return Result(-1, "Failed to initialize audio engine: {}",
						  ma_result_description(mares));
		}

		m_engine.ownsResourceManager = MA_TRUE;

		// Set global volume to 0.75f by default
		set_global_volume(0.75f);

		return {};
	}

	static void cleanup() {
		stop_sounds();
		ma_engine_uninit(&m_engine);
	}

	// Handles uninitializing ended sounds, and playing next sound in sequence
	static void update() {
		for (auto &[guid, sounds] : m_sounds) {
			if (sounds.empty())
				continue;

			// Play next sound in sequence if current one has ended (ma_sound_at_end)
			if (ma_sound_at_end(sounds.front().get())) {
				// Uninitialize sound
				ma_sound_uninit(sounds.front().get());
				// Remove from deque
				sounds.pop_front();

				// Play next sound in sequence
				if (!sounds.empty())
					ma_sound_start(sounds.front().get());
			}
		}
	}

	// Stops all sounds
	static void stop_sounds() {
		for (auto &[name, sounds] : m_sounds) {
			for (auto &sound : sounds) {
				ma_sound_stop(sound.get());
				ma_sound_uninit(sound.get());
			}
		}
		m_sounds.clear();
	}

	static auto get_global_volume() -> float { return m_volume; }
	static void set_global_volume(const float volume) {
		m_volume = volume;
		ma_engine_set_volume(&m_engine, m_volume);
	}

	static void play_oneshot(const std::filesystem::path &file) {
		// Create new sound
		const auto sound = std::make_shared<ma_sound>();
		if (ma_sound_init_from_file(&m_engine, file.string().c_str(),
									MA_SOUND_FLAG_ASYNC | MA_SOUND_FLAG_NO_PITCH |
										MA_SOUND_FLAG_NO_SPATIALIZATION,
									nullptr, nullptr, sound.get()) != MA_SUCCESS) {
			return;
		}

		ma_sound_start(m_sounds[generate_guid()].emplace_back(sound).get());
	}

	// Plays given sounds in order, waiting for last one to finish before starting next
	static void play_sequential(const std::vector<std::filesystem::path> &files) {
		const auto groupGUID = generate_guid();
		// Create sounds
		for (const auto &file : files) {
			const auto sound = std::make_shared<ma_sound>();
			if (ma_sound_init_from_file(&m_engine, file.string().c_str(),
										MA_SOUND_FLAG_ASYNC | MA_SOUND_FLAG_NO_PITCH |
											MA_SOUND_FLAG_NO_SPATIALIZATION,
										nullptr, nullptr, sound.get()) != MA_SUCCESS) {
				return;
			}

			m_sounds[groupGUID].emplace_back(sound);
		}

		// Play first sound
		ma_sound_start(m_sounds[groupGUID].front().get());
	}
};