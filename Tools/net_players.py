"""Sets how many players Play-In-Editor starts. Tools/nettest.sh runs this with PLAYERS replaced.
One player is ordinary single-player; more start a listen server with the rest as clients, all in this editor."""
import unreal

PLAYERS = 2  # replaced by Tools/nettest.sh
settings = unreal.get_default_object(unreal.LevelEditorPlaySettings)
settings.set_editor_property("play_number_of_clients", PLAYERS)
settings.set_editor_property("play_net_mode", unreal.PlayNetMode.PIE_LISTEN_SERVER if PLAYERS > 1 else unreal.PlayNetMode.PIE_STANDALONE)
settings.set_editor_property("run_under_one_process", True)
f"play settings: {PLAYERS} player(s)"
