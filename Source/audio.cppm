module;

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_WAV
#define MA_NO_MP3
#define MA_NO_FLAC
#include <miniaudio.h>

#include <array>
#include <vector>
#include <memory>
#include <string>
#include <ranges>
#include <filesystem>

export module audio;

import config;
import common;
import opus_decoder;

enum class SoundType {
	eOneshot,
	eSequential,
	eMemory,
};

struct AudioPlayerSound {
	SoundType type = SoundType::eOneshot;
	std::shared_ptr<ma_sound> sound = nullptr;
	std::shared_ptr<ma_audio_buffer> buffer = nullptr;
	std::shared_ptr<AudioPlayerSound> next = nullptr;

	explicit AudioPlayerSound(const SoundType &type,
							  const std::shared_ptr<ma_audio_buffer> &buffer = nullptr)
		: type(type), sound(std::make_shared<ma_sound>()), buffer(buffer) {}
};

// Super-duper simple audio player
export class AudioPlayer {
	static inline ma_engine m_engine;
	static inline float m_volume;

	// Vector of sounds
	static inline std::vector<std::shared_ptr<AudioPlayerSound>> m_sounds;

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

	// Handles uninitializing ended sounds and playing next sound in sequence
	static void update() {
		for (const auto sound : m_sounds) {
			// Play next sound in sequence if current one has reached the point
			const auto &soundTime =
				static_cast<float>(ma_sound_get_time_in_milliseconds(sound->sound.get())) / 1000.0f;
			float soundLength = 0;
			ma_sound_get_length_in_seconds(sound->sound.get(), &soundLength);

			if (sound->type == SoundType::eSequential &&
				(ma_sound_at_end(sound->sound.get()) ||
				 soundTime >= soundLength + global_config.audioSequenceOffset)) {
				// Start playback of next sound
				if (sound->next != nullptr) ma_sound_start(sound->next->sound.get());
			}
		}

		// Clean ended sounds
		std::vector<std::shared_ptr<AudioPlayerSound>> soundsToRemove;
		for (const auto sound : m_sounds) {
			if (ma_sound_at_end(sound->sound.get())) {
				ma_sound_stop(sound->sound.get());
				ma_sound_uninit(sound->sound.get());
				if (sound->buffer != nullptr) ma_audio_buffer_uninit(sound->buffer.get());

				soundsToRemove.push_back(sound);
			}
		}

		// Remove sounds
		for (const auto sound : soundsToRemove) m_sounds.erase(std::ranges::find(m_sounds, sound));
	}

	// Stops all sounds
	static void stop_sounds() {
		for (const auto sound : m_sounds) {
			ma_sound_stop(sound->sound.get());
			ma_sound_uninit(sound->sound.get());
			if (sound->buffer != nullptr) ma_audio_buffer_uninit(sound->buffer.get());
		}
		m_sounds.clear();
	}

	static auto get_global_volume() -> float { return m_volume; }
	static void set_global_volume(const float volume) {
		m_volume = volume;
		ma_engine_set_volume(&m_engine, m_volume);
	}

	static void play_oneshot(const std::filesystem::path &file, const float volume = 1.0f,
							 const float pitch = 1.0f) {
		// Create new sound
		const auto sound =
			m_sounds.emplace_back(std::make_shared<AudioPlayerSound>(SoundType::eOneshot));
		if (ma_sound_init_from_file(&m_engine, file.string().c_str(),
									MA_SOUND_FLAG_ASYNC | MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr,
									nullptr, sound->sound.get()) != MA_SUCCESS) {
			return;
		}

		ma_sound_set_volume(sound->sound.get(), volume);
		ma_sound_set_pitch(sound->sound.get(), pitch);
		ma_sound_start(sound->sound.get());
	}

	// Plays given sounds in order, waiting for last one to finish before starting next
	static void play_sequential(const std::vector<std::filesystem::path> &files,
								const float volume = 1.0f, const float pitch = 1.0f) {
		std::vector<std::shared_ptr<AudioPlayerSound>> sequence;
		for (const auto &file : files) {
			const auto sound = std::make_shared<AudioPlayerSound>(SoundType::eSequential);
			if (ma_sound_init_from_file(&m_engine, file.string().c_str(),
										MA_SOUND_FLAG_ASYNC | MA_SOUND_FLAG_NO_SPATIALIZATION,
										nullptr, nullptr, sound->sound.get()) != MA_SUCCESS) {
				return;
			}

			ma_sound_set_volume(sound->sound.get(), volume);
			ma_sound_set_pitch(sound->sound.get(), pitch);
			// Assign next sound in sequence
			if (!sequence.empty()) sequence.back()->next = sound;

			sequence.push_back(sound);
		}

		// Add to sounds
		m_sounds.insert(m_sounds.end(), sequence.begin(), sequence.end());

		// Begin playback of first sound
		ma_sound_start(sequence.front()->sound.get());
	}

	// Plays from memory
	static void play_oneshot_memory(const std::vector<float> &data, const std::uint32_t &samplerate,
									const float volume = 1.0f, const float pitch = 1.0f) {
		// Have local copy of data
		std::vector<float> localData(data.size());
		std::ranges::copy(data, localData.begin());

		ma_audio_buffer_config bufferConfig = ma_audio_buffer_config_init(
			ma_format_f32, 1, localData.size(), localData.data(), nullptr);
		bufferConfig.sampleRate = samplerate;
		const auto buffer = std::make_shared<ma_audio_buffer>();
		ma_audio_buffer_init(&bufferConfig, buffer.get());

		const auto sound =
			m_sounds.emplace_back(std::make_shared<AudioPlayerSound>(SoundType::eMemory, buffer));
		if (ma_sound_init_from_data_source(&m_engine, buffer.get(),
										   MA_SOUND_FLAG_ASYNC | MA_SOUND_FLAG_NO_SPATIALIZATION,
										   nullptr, sound->sound.get()) != MA_SUCCESS) {
			return;
		}

		ma_sound_set_volume(sound->sound.get(), volume);
		ma_sound_set_pitch(sound->sound.get(), pitch);
		ma_sound_start(sound->sound.get());
	}
};