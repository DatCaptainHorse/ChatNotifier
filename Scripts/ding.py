import pathlib


def on_message(msg):
    # Get sound assets path
    sound_assets_path = pathlib.Path(chatnotifier.get_sound_assets_path())
    # Check if ding.opus exists, if so, play it when a message is received
    if (sound_assets_path / "ding.opus").exists():
        # Convert path to string
        chatnotifier.play_oneshot_file((sound_assets_path / "ding.opus").as_posix())