import chatnotifier
import Deps.sherpa_onnx as sherpa
import pathlib
import random

# Dict of available voice models to use
voices_models: dict[str, sherpa.OfflineTtsConfig] = {}
# Cache dict of users and their "custom" voice
user_voices: dict[str, tuple[str, int]] = {}
# Current loaded models
loaded_models: dict[str, sherpa.OfflineTts] = {}


def on_load():
    # Get all available voice models
    tts_assets_path = pathlib.Path(chatnotifier.get_tts_assets_path())
    for subfolder in tts_assets_path.iterdir():
        if (subfolder.is_dir() and (subfolder / "model.onnx").exists() and (subfolder / "tokens.txt").exists()
            and (subfolder / "espeak-ng-data").exists()) and (subfolder / "config.json").exists():
            tts_conf = sherpa.OfflineTtsConfig(
                model=sherpa.OfflineTtsModelConfig(
                    vits=sherpa.OfflineTtsVitsModelConfig(
                        model=str(subfolder / "model.onnx"),
                        lexicon="",  # Not used
                        tokens=str(subfolder / "tokens.txt"),
                        data_dir=str(subfolder / "espeak-ng-data"),
                    ),
                    provider="cpu",
                    debug=False,
                    num_threads=4,
                ),
                max_num_sentences=1,
            )
            if not tts_conf.validate():
                print(f"Invalid config for voice model {subfolder.name}, skipping")
                continue

            voices_models[subfolder.name] = tts_conf


def load_voice_model(voice_name: str) -> sherpa.OfflineTts | None:
    if voice_name not in voices_models:
        return None

    if voice_name not in loaded_models:
        loaded_models[voice_name] = sherpa.OfflineTts(voices_models[voice_name])

    return loaded_models[voice_name]


def get_user_voice(user: str) -> tuple[sherpa.OfflineTts, int] | None:
    # Check if user has a custom voice already, if not, assign random one
    if user not in user_voices:
        user_voices[user] = (random.choice(list(voices_models.keys())), -1)

    vmodel = load_voice_model(user_voices[user][0])
    if vmodel is None:
        return None

    # If user speaker id is -1, get one from num_speakers of the model
    if user_voices[user][1] == -1:
        user_voices[user] = (user_voices[user][0], random.randint(0, vmodel.num_speakers - 1))

    return vmodel, user_voices[user][1]


def on_message(msg):
    # Get user voice
    voice = get_user_voice(msg.user)
    if voice is None:
        return

    generated = voice[0].generate(
        msg.get_message(),
        sid=voice[1],
        speed=1.0,
    )

    chatnotifier.play_oneshot_memory(generated.samples, generated.sample_rate, 1)
