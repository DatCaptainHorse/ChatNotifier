from piper import PiperVoice
import numpy as np
import pathlib


def on_message(msg):
    model_path = pathlib.Path(chatnotifier.get_tts_assets_path()) / "fi_FI-harri-medium.onnx"
    config_path = pathlib.Path(
        chatnotifier.get_tts_assets_path()) / "fi_fi_FI_harri_medium_fi_FI-harri-medium.onnx.json"
    if not model_path.exists() or not config_path.exists():
        print(f"Model not found: {model_path}")
        return

    print(f"Using model: {model_path}")
    voice = PiperVoice.load(model_path=str(model_path),
                            config_path=str(config_path),
                            use_cuda=False)

    print("Synthesizing audio")
    audio_stream = voice.synthesize_stream_raw(
        text=msg,
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

    print("Audio synthesized")
    chatnotifier.play_oneshot_memory(collected_audio, voice.config.sample_rate, 1)
    print("Audio played")
