module;

#include <sndfile.h>

#define AL_ALEXT_PROTOTYPES
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/efx-presets.h>

#include <map>
#include <print>
#include <array>
#include <chrono>
#include <vector>
#include <memory>
#include <string>
#include <ranges>
#include <optional>
#include <filesystem>
#include <source_location>

export module audio;

import config;
import common;

// Struct of passable (memory) sound data
export struct SoundData {
	std::vector<float> data;
	std::uint32_t samplerate, channels;
	float lengthInSeconds;

	SoundData(const std::vector<float> &data, const std::uint32_t &samplerate,
			  const std::uint32_t &channels)
		: data(data), samplerate(samplerate), channels(channels) {
		lengthInSeconds = static_cast<float>(data.size()) / static_cast<float>(samplerate);
	}
};

// Struct of passable sound options
export struct SoundOptions {
	std::optional<float> volume = std::nullopt;
	std::optional<float> pitch = std::nullopt;
	std::optional<float> offset = std::nullopt;
	std::optional<Position3D> pos = std::nullopt;
	std::optional<std::vector<std::string>> effects = std::nullopt;
};

struct AudioPlayerSound {
	SoundOptions options;
	ALuint buffer = 0, SID = 0;
	float length = 0.0f, lengthOffset = 0.0f;
	std::vector<ALuint> effectSlots;
	std::chrono::time_point<std::chrono::steady_clock> endedTime;
	std::shared_ptr<AudioPlayerSound> next = nullptr;

	explicit AudioPlayerSound(const SoundOptions &opts, const ALuint &buffer, const ALuint &SID,
							  const float &length)
		: options(opts), buffer(buffer), SID(SID), length(length) {}
};

// Method for checking and printing out OpenAL errors
export auto check_al_errors(const std::source_location location = std::source_location::current())
	-> bool {
	if (const auto error = alGetError(); error != AL_NO_ERROR) {
		std::string errorStr;
		switch (error) {
		case AL_INVALID_NAME:
			errorStr = "AL_INVALID_NAME";
			break;
		case AL_INVALID_ENUM:
			errorStr = "AL_INVALID_ENUM";
			break;
		case AL_INVALID_VALUE:
			errorStr = "AL_INVALID_VALUE";
			break;
		case AL_INVALID_OPERATION:
			errorStr = "AL_INVALID_OPERATION";
			break;
		case AL_OUT_OF_MEMORY:
			errorStr = "AL_OUT_OF_MEMORY";
			break;
		default:
			errorStr = "UNKNOWN";
			break;
		}
		std::println("OpenAL Error: {} at {}:{}", errorStr, location.file_name(), location.line());
		return true;
	}
	return false;
}

// Super-duper simple audio player
export class AudioPlayer {
	static inline float m_volume;
	static inline ALCdevice *m_device = nullptr;
	static inline ALCcontext *m_context = nullptr;
	static inline std::map<std::string, ALuint> m_effects;

	// Vector of sounds
	static inline std::vector<std::shared_ptr<AudioPlayerSound>> m_sounds;

public:
	static auto initialize() -> Result {
		// Get primary output device
		const auto primaryOutput = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
		if (!primaryOutput) return {1, "Failed to get primary audio output"};

		// Open audio device and context
		m_device = alcOpenDevice(primaryOutput);
		if (!m_device) return {1, "Failed to open audio device"};

		constexpr std::array attrs{ALC_HRTF_SOFT, ALC_TRUE, 0};
		m_context = alcCreateContext(m_device, attrs.data());
		if (!m_context) return {1, "Failed to create audio context"};

		if (!alcMakeContextCurrent(m_context)) return {1, "Failed to make audio context current"};
		check_al_errors();

		if (!alIsExtensionPresent("AL_EXT_float32")) return {1, "AL_EXT_float32 not supported"};
		check_al_errors();

		// Default effects //

		// Basic reverb
		ALuint reverbEffect = 0;
		alGenEffects(1, &reverbEffect);
		constexpr EFXEAXREVERBPROPERTIES reverb = EFX_REVERB_PRESET_LIVINGROOM;
		alEffecti(reverbEffect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
		alEffectf(reverbEffect, AL_EAXREVERB_DENSITY, reverb.flDensity);
		alEffectf(reverbEffect, AL_EAXREVERB_DIFFUSION, reverb.flDiffusion);
		alEffectf(reverbEffect, AL_EAXREVERB_GAIN, reverb.flGain);
		alEffectf(reverbEffect, AL_EAXREVERB_GAINHF, reverb.flGainHF);
		alEffectf(reverbEffect, AL_EAXREVERB_GAINLF, reverb.flGainLF);
		alEffectf(reverbEffect, AL_EAXREVERB_DECAY_TIME, reverb.flDecayTime);
		alEffectf(reverbEffect, AL_EAXREVERB_DECAY_HFRATIO, reverb.flDecayHFRatio);
		alEffectf(reverbEffect, AL_EAXREVERB_DECAY_LFRATIO, reverb.flDecayLFRatio);
		alEffectf(reverbEffect, AL_EAXREVERB_REFLECTIONS_GAIN, reverb.flReflectionsGain);
		alEffectf(reverbEffect, AL_EAXREVERB_REFLECTIONS_DELAY, reverb.flReflectionsDelay);
		alEffectfv(reverbEffect, AL_EAXREVERB_REFLECTIONS_PAN, reverb.flReflectionsPan);
		alEffectf(reverbEffect, AL_EAXREVERB_LATE_REVERB_GAIN, reverb.flLateReverbGain);
		alEffectf(reverbEffect, AL_EAXREVERB_LATE_REVERB_DELAY, reverb.flLateReverbDelay);
		alEffectfv(reverbEffect, AL_EAXREVERB_LATE_REVERB_PAN, reverb.flLateReverbPan);
		alEffectf(reverbEffect, AL_EAXREVERB_ECHO_TIME, reverb.flEchoTime);
		alEffectf(reverbEffect, AL_EAXREVERB_ECHO_DEPTH, reverb.flEchoDepth);
		alEffectf(reverbEffect, AL_EAXREVERB_MODULATION_TIME, reverb.flModulationTime);
		alEffectf(reverbEffect, AL_EAXREVERB_MODULATION_DEPTH, reverb.flModulationDepth);
		alEffectf(reverbEffect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, reverb.flAirAbsorptionGainHF);
		alEffectf(reverbEffect, AL_EAXREVERB_HFREFERENCE, reverb.flHFReference);
		alEffectf(reverbEffect, AL_EAXREVERB_LFREFERENCE, reverb.flLFReference);
		alEffectf(reverbEffect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, reverb.flRoomRolloffFactor);
		alEffecti(reverbEffect, AL_EAXREVERB_DECAY_HFLIMIT, reverb.iDecayHFLimit);
		m_effects["reverb"] = reverbEffect;
		check_al_errors();

		// "Big" reverb, just EFX_REVERB_PRESET_CONCERTHALL
		ALuint bigReverbEffect = 0;
		alGenEffects(1, &bigReverbEffect);
		constexpr EFXEAXREVERBPROPERTIES bigReverb = EFX_REVERB_PRESET_CONCERTHALL;
		alEffecti(bigReverbEffect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
		alEffectf(bigReverbEffect, AL_EAXREVERB_DENSITY, bigReverb.flDensity);
		alEffectf(bigReverbEffect, AL_EAXREVERB_DIFFUSION, bigReverb.flDiffusion);
		alEffectf(bigReverbEffect, AL_EAXREVERB_GAIN, bigReverb.flGain);
		alEffectf(bigReverbEffect, AL_EAXREVERB_GAINHF, bigReverb.flGainHF);
		alEffectf(bigReverbEffect, AL_EAXREVERB_GAINLF, bigReverb.flGainLF);
		alEffectf(bigReverbEffect, AL_EAXREVERB_DECAY_TIME, bigReverb.flDecayTime);
		alEffectf(bigReverbEffect, AL_EAXREVERB_DECAY_HFRATIO, bigReverb.flDecayHFRatio);
		alEffectf(bigReverbEffect, AL_EAXREVERB_DECAY_LFRATIO, bigReverb.flDecayLFRatio);
		alEffectf(bigReverbEffect, AL_EAXREVERB_REFLECTIONS_GAIN, bigReverb.flReflectionsGain);
		alEffectf(bigReverbEffect, AL_EAXREVERB_REFLECTIONS_DELAY, bigReverb.flReflectionsDelay);
		alEffectfv(bigReverbEffect, AL_EAXREVERB_REFLECTIONS_PAN, bigReverb.flReflectionsPan);
		alEffectf(bigReverbEffect, AL_EAXREVERB_LATE_REVERB_GAIN, bigReverb.flLateReverbGain);
		alEffectf(bigReverbEffect, AL_EAXREVERB_LATE_REVERB_DELAY, bigReverb.flLateReverbDelay);
		alEffectfv(bigReverbEffect, AL_EAXREVERB_LATE_REVERB_PAN, bigReverb.flLateReverbPan);
		alEffectf(bigReverbEffect, AL_EAXREVERB_ECHO_TIME, bigReverb.flEchoTime);
		alEffectf(bigReverbEffect, AL_EAXREVERB_ECHO_DEPTH, bigReverb.flEchoDepth);
		alEffectf(bigReverbEffect, AL_EAXREVERB_MODULATION_TIME, bigReverb.flModulationTime);
		alEffectf(bigReverbEffect, AL_EAXREVERB_MODULATION_DEPTH, bigReverb.flModulationDepth);
		alEffectf(bigReverbEffect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF,
				  reverb.flAirAbsorptionGainHF);
		alEffectf(bigReverbEffect, AL_EAXREVERB_HFREFERENCE, bigReverb.flHFReference);
		alEffectf(bigReverbEffect, AL_EAXREVERB_LFREFERENCE, bigReverb.flLFReference);
		alEffectf(bigReverbEffect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, bigReverb.flRoomRolloffFactor);
		alEffecti(bigReverbEffect, AL_EAXREVERB_DECAY_HFLIMIT, bigReverb.iDecayHFLimit);
		m_effects["bigreverb"] = bigReverbEffect;
		check_al_errors();

		// Distortion
		ALuint distortionEffect = 0;
		alGenEffects(1, &distortionEffect);
		alEffecti(distortionEffect, AL_EFFECT_TYPE, AL_EFFECT_DISTORTION);
		alEffectf(distortionEffect, AL_DISTORTION_EDGE, 0.2f);
		alEffectf(distortionEffect, AL_DISTORTION_GAIN, 0.5f);
		alEffectf(distortionEffect, AL_DISTORTION_LOWPASS_CUTOFF, 8000.0f);
		alEffectf(distortionEffect, AL_DISTORTION_EQCENTER, 3600.0f);
		alEffectf(distortionEffect, AL_DISTORTION_EQBANDWIDTH, 3600.0f);
		m_effects["distortion"] = distortionEffect;
		check_al_errors();

		// Echo
		ALuint echoEffect = 0;
		alGenEffects(1, &echoEffect);
		alEffecti(echoEffect, AL_EFFECT_TYPE, AL_EFFECT_ECHO);
		alEffectf(echoEffect, AL_ECHO_DELAY, 0.1f);
		alEffectf(echoEffect, AL_ECHO_LRDELAY, 0.1f);
		alEffectf(echoEffect, AL_ECHO_DAMPING, 0.5f);
		alEffectf(echoEffect, AL_ECHO_FEEDBACK, 0.5f);
		m_effects["echo"] = echoEffect;
		check_al_errors();

		// Set global volume to 0.75f by default
		set_global_volume(0.75f);

		return Result();
	}

	static void cleanup() {
		stop_sounds();
		for (const auto &effect : m_effects | std::views::values) alDeleteEffects(1, &effect);
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(m_context);
		alcCloseDevice(m_device);
	}

	// Handles uninitializing ended sounds and playing next sound in sequence
	static void update() {
		for (const auto sound : m_sounds) {
			// Play next sound in sequence if current one has reached the point
			auto soundTime = 0.0f;
			alGetSourcef(sound->SID, AL_SEC_OFFSET, &soundTime);
			auto soundState = AL_INITIAL;
			alGetSourcei(sound->SID, AL_SOURCE_STATE, &soundState);

			if (sound->next != nullptr &&
				(soundState != AL_PLAYING || soundTime >= sound->length + sound->lengthOffset)) {
				// Start playback of next sound
				alSourcePlay(sound->next->SID);
			}
		}
		check_al_errors();

		// Clean ended sounds, 3s delay from their end to allow effects to fade out
		std::vector<std::shared_ptr<AudioPlayerSound>> soundsToRemove;
		for (const auto sound : m_sounds) {
			auto soundState = AL_INITIAL;
			alGetSourcei(sound->SID, AL_SOURCE_STATE, &soundState);
			auto soundTime = 0.0f;
			alGetSourcef(sound->SID, AL_SEC_OFFSET, &soundTime);
			check_al_errors();
			if (soundState != AL_PLAYING && soundTime >= sound->length) {
				if (sound->endedTime.time_since_epoch().count() == 0)
					sound->endedTime = std::chrono::steady_clock::now();
				else if (std::chrono::steady_clock::now() - sound->endedTime >=
						 std::chrono::seconds(3)) {
					clear_sound_oal(sound);
					soundsToRemove.push_back(sound);
				}
			}
		}
		check_al_errors();

		// Remove sounds
		for (const auto sound : soundsToRemove) m_sounds.erase(std::ranges::find(m_sounds, sound));
	}

	// Method for clearing out OpenAL sound
	static void clear_sound_oal(const std::shared_ptr<AudioPlayerSound> &sound) {
		alSourceStop(sound->SID);
		// Make effect slots unused first, before deleting them
		alSource3i(sound->SID, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
		for (const auto effectSlot : sound->effectSlots)
			alAuxiliaryEffectSloti(effectSlot, AL_EFFECTSLOT_TARGET_SOFT, AL_EFFECTSLOT_NULL);

		alDeleteAuxiliaryEffectSlots(sound->effectSlots.size(), sound->effectSlots.data());
		alDeleteSources(1, &sound->SID);
		alDeleteBuffers(1, &sound->buffer);
	}

	// Stops all sounds
	static void stop_sounds() {
		for (const auto sound : m_sounds) clear_sound_oal(sound);

		check_al_errors();
		m_sounds.clear();
	}

	static auto get_global_volume() -> float { return m_volume; }
	static void set_global_volume(const float volume) {
		m_volume = volume;
		alListenerf(AL_GAIN, m_volume);
	}

	static auto load_sound_oal(const SoundData &soundData, const SoundOptions &opts)
		-> std::shared_ptr<AudioPlayerSound> {
		const auto format =
			soundData.channels == 1 ? AL_FORMAT_MONO_FLOAT32 : AL_FORMAT_STEREO_FLOAT32;
		const auto dataSize = static_cast<ALsizei>(soundData.data.size() * sizeof(float));

		// OpenAL sound creation
		ALuint buffer;
		alGenBuffers(1, &buffer);
		alBufferData(buffer, format, soundData.data.data(), dataSize, soundData.samplerate);
		if (check_al_errors()) {
			alDeleteBuffers(1, &buffer);
			return nullptr;
		}

		// Create sound source
		ALuint SID;
		alGenSources(1, &SID);
		alSourcei(SID, AL_BUFFER, static_cast<ALint>(buffer));
		if (check_al_errors()) {
			alDeleteSources(1, &SID);
			alDeleteBuffers(1, &buffer);
			return nullptr;
		}

		// Create new sound
		const auto sound =
			std::make_shared<AudioPlayerSound>(opts, buffer, SID, soundData.lengthInSeconds);

		// Create effects if given
		if (opts.effects) {
			std::vector<ALuint> effectSlots;
			const auto wantedEffects = opts.effects.value();
			for (std::uint32_t i = 0; i < wantedEffects.size(); ++i) {
				if (const auto effect = wantedEffects[i]; m_effects.contains(effect)) {
					ALuint effectSlot = 0;
					alGenAuxiliaryEffectSlots(1, &effectSlot);
					alAuxiliaryEffectSloti(effectSlot, AL_EFFECTSLOT_EFFECT, m_effects[effect]);
					effectSlots.push_back(effectSlot);
				}
			}
			check_al_errors();
			sound->effectSlots = effectSlots;

			// Set first effect as aux send
			if (!effectSlots.empty())
				alSource3i(SID, AL_AUXILIARY_SEND_FILTER, effectSlots[0], 0, AL_FILTER_NULL);

			check_al_errors();

			// Finally, target every effect to their next effect, or the source if it's the last
			for (std::uint32_t i = 0; i < effectSlots.size(); ++i) {
				if (i + 1 < effectSlots.size())
					alAuxiliaryEffectSloti(effectSlots[i], AL_EFFECTSLOT_TARGET_SOFT,
										   effectSlots[i + 1]);
				else
					alAuxiliaryEffectSloti(effectSlots[i], AL_EFFECTSLOT_TARGET_SOFT,
										   AL_EFFECTSLOT_NULL);
			}
			check_al_errors();
		}

		// If pos is set, enable 3D sound
		if (opts.pos) {
			alSourcei(SID, AL_SOURCE_RELATIVE, AL_TRUE);
			alSourcei(SID, AL_SOURCE_SPATIALIZE_SOFT, AL_TRUE);
			const auto [x, y, z] = opts.pos.value();
			alSource3f(SID, AL_POSITION, x, y, z);
		} else {
			alSourcei(SID, AL_SOURCE_RELATIVE, AL_FALSE);
			alSourcei(SID, AL_SOURCE_SPATIALIZE_SOFT, AL_FALSE);
			alSourcei(SID, AL_DIRECT_CHANNELS_SOFT, AL_REMIX_UNMATCHED_SOFT);
		}
		check_al_errors();

		// Set settings and play audio
		if (opts.pitch) alSourcef(SID, AL_PITCH, opts.pitch.value());
		if (opts.volume) alSourcef(SID, AL_GAIN, opts.volume.value());
		check_al_errors();

		return sound;
	}

	static void play_oneshot(const std::filesystem::path &file, const SoundOptions &opts = {}) {
		// Load file using sndfile
		SF_INFO sfInfo;
		const auto sndFile = sf_open(file.string().c_str(), SFM_READ, &sfInfo);
		if (!sndFile) return;

		std::vector<float> samples(sfInfo.frames * sfInfo.channels);
		sf_readf_float(sndFile, samples.data(), sfInfo.frames);
		sf_close(sndFile);

		// Load sound
		const auto sound = load_sound_oal({samples, static_cast<std::uint32_t>(sfInfo.samplerate),
										   static_cast<std::uint32_t>(sfInfo.channels)},
										  opts);
		m_sounds.push_back(sound);
		alSourcePlay(sound->SID);
	}

	// Plays given sounds in order, waiting for last one to finish before starting next
	static void play_sequential(const std::vector<std::filesystem::path> &files,
								const std::vector<SoundOptions> &opts) {
		std::vector<std::shared_ptr<AudioPlayerSound>> sequence;
		for (const auto &[file, opt] : std::views::zip(files, opts)) {
			// Load file using sndfile
			SF_INFO sfInfo;
			const auto sndFile = sf_open(file.string().c_str(), SFM_READ, &sfInfo);
			if (!sndFile) continue;

			std::vector<float> samples(sfInfo.frames * sfInfo.channels);
			sf_readf_float(sndFile, samples.data(), sfInfo.frames);
			sf_close(sndFile);

			// Load sound
			const auto sound =
				load_sound_oal({samples, static_cast<std::uint32_t>(sfInfo.samplerate),
								static_cast<std::uint32_t>(sfInfo.channels)},
							   opt);

			// Assign next sound in sequence
			if (!sequence.empty()) sequence.back()->next = sound;
			sequence.push_back(sound);
		}

		// Add to sounds
		m_sounds.insert(m_sounds.end(), sequence.begin(), sequence.end());

		// Begin playback of first sound
		alSourcePlay(sequence.front()->SID);
	}

	// Plays from memory, returns sound length in milliseconds
	static void play_oneshot_memory(const SoundData &soundData, const SoundOptions &opts) {
		const auto sound = load_sound_oal(soundData, opts);
		m_sounds.push_back(sound);
		alSourcePlay(sound->SID);
	}

	// Plays given sounds in order from memory, waiting for last one to finish before starting next
	static void play_sequential_memory(const std::vector<SoundData> &soundDatas,
									   const std::vector<SoundOptions> &opts) {
		std::vector<std::shared_ptr<AudioPlayerSound>> sequence;
		for (const auto &[soundData, opt] : std::views::zip(soundDatas, opts)) {
			const auto sound = load_sound_oal(soundData, opt);

			// Assign next sound in sequence
			if (!sequence.empty()) sequence.back()->next = sound;
			sequence.push_back(sound);
		}

		// Add to sounds
		m_sounds.insert(m_sounds.end(), sequence.begin(), sequence.end());

		// Begin playback of first sound
		alSourcePlay(sequence.front()->SID);
	}
};
