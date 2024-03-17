module;

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_WAV
#define MA_NO_MP3
#define MA_NO_FLAC
#include <miniaudio.h>

#include <array>
#include <filesystem>
#include <fmt/format.h>

export module audio;

import opus_decoder;

// Super-duper simple audio player
export class AudioPlayer {
	static inline ma_engine m_engine;
	static inline float m_volume;

	static inline ma_decoding_backend_vtable decoding_backend_opus{
		ma_decoding_backend_init_libopus, ma_decoding_backend_init_file_libopus,
		nullptr, /* onInitFileW() */
		nullptr, /* onInitMemory() */
		ma_decoding_backend_uninit_libopus};

public:
	static void initialize() {
		auto engineConfig = ma_engine_config_init();
		engineConfig.pResourceManager = new ma_resource_manager();
		if (engineConfig.pResourceManager == nullptr) {
			fmt::println(stderr, "Failed to allocate memory for resource manager");
			return;
		}

		std::array pCustomBackendVTables = {&decoding_backend_opus};

		auto resourceManagerConfig = ma_resource_manager_config_init();
		resourceManagerConfig.pLog = engineConfig.pLog;
		resourceManagerConfig.decodedFormat = ma_format_f32;
		resourceManagerConfig.decodedChannels = 0;
		resourceManagerConfig.decodedSampleRate = engineConfig.sampleRate;
		ma_allocation_callbacks_init_copy(&resourceManagerConfig.allocationCallbacks,
										  &engineConfig.allocationCallbacks);
		resourceManagerConfig.pVFS = engineConfig.pResourceManagerVFS;
		resourceManagerConfig.ppCustomDecodingBackendVTables = pCustomBackendVTables.data();
		resourceManagerConfig.customDecodingBackendCount = pCustomBackendVTables.size();

		auto mares =
			ma_resource_manager_init(&resourceManagerConfig, engineConfig.pResourceManager);
		if (mares != MA_SUCCESS) {
			fmt::println(stderr, "Failed to initialize resource manager: {}",
						 ma_result_description(mares));
			return;
		}

		mares = ma_engine_init(&engineConfig, &m_engine);
		if (mares != MA_SUCCESS) {
			fmt::println(stderr, "Failed to initialize miniaudio engine: {}",
						 ma_result_description(mares));
			return;
		}

		m_engine.ownsResourceManager = MA_TRUE;

		// Set global volume to 0.75f by default
		set_global_volume(0.75f);
	}

	static void cleanup() { ma_engine_uninit(&m_engine); }

	static auto get_global_volume() -> float { return m_volume; }
	static void set_global_volume(const float volume) {
		m_volume = volume;
		ma_engine_set_volume(&m_engine, m_volume);
	}

	static void play_oneshot(const std::filesystem::path &file) {
		// Make sure the file exists, then play it
		if (std::filesystem::exists(file))
			ma_engine_play_sound(&m_engine, file.c_str(), nullptr);
	}
};