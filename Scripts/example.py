import pathlib

def on_load():
    # Look for tutturuu.opus in chatnotifier.get_sound_assets_path()
    sound_path = pathlib.Path(chatnotifier.get_sound_assets_path()) / "tutturuu.opus"
    if sound_path.exists():
        chatnotifier.play_oneshot_file(str(sound_path))

def on_message(msg):
    print("Hello from Python: {} - {}".format(msg.user, msg.message))
