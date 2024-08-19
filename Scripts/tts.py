from Deps.piper import PiperVoice
import numpy as np
import pathlib
import random

# List of available voices
voices: list[str] = []
# Cache dict of users and their "custom" voices
user_voices: dict[str, str] = {}


def on_load():
    # Get all available voices
    voices_path = pathlib.Path(chatnotifier.get_tts_assets_path())
    for voice in voices_path.glob("*.onnx"):
        voices.append(voice.stem)


def load_voice(name: str) -> tuple[str, PiperVoice] | None:
    # Load given voice by name
    voices_path = pathlib.Path(chatnotifier.get_tts_assets_path())
    for voice in voices_path.glob("*.onnx"):
        voice_name = voice.stem
        # Check if file name contains name
        if name.lower() not in voice_name.lower():
            continue

        # Config always ends with "onnx.json"
        config = voices_path / f"{voice_name}.onnx.json"
        if config.exists():
            return voice_name, PiperVoice.load(model_path=str(voice),
                                               config_path=str(config),
                                               use_cuda=False)
        else:
            print(f"Voice config not found: {str(config)}")
    return None


def get_user_voice(user: str) -> PiperVoice | None:
    # Check if user has a custom voice already, if not, assign random one from loaded voices
    if user not in user_voices:
        voice_name = random.choice(voices)
        user_voices[user] = voice_name

    return load_voice(user_voices[user])[1]


def on_message(msg):
    # Get user voice
    voice = get_user_voice(msg.user)
    if voice is None:
        return

    audio_stream = voice.synthesize_stream_raw(
        text=msg.message,
        length_scale=1.0,
        noise_scale=0.667,
        noise_w=0.8,
    )

    # Collect int16 byte audio
    collected_audio = b""
    for audio in audio_stream:
        collected_audio += audio

    # Convert to float32, piper works in mysterious ways
    collected_audio = np.frombuffer(collected_audio, dtype=np.int16).astype(np.float32) / 32768.0

    # Convert audio to list of floats
    collected_audio = collected_audio.tolist()

    chatnotifier.play_oneshot_memory(collected_audio, voice.config.sample_rate, 1)
