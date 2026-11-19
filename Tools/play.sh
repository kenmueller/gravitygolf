#!/bin/sh
# Starts Play-In-Editor (or stops it with "stop") and waits for warm-up:  Tools/play.sh [seconds|stop]
# GG_PLAYERS=2 Tools/play.sh starts a host and one joined player, for the co-op tests.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [ "$1" = "stop" ]; then
	# Ask the editor to end play on its own next tick. The MCP StopPIE tool tears the session down from inside the
	# tool call, which asserts (and takes the editor with it) when a second player's window is open, and has hung
	# the editor with one.
	TMP="$(mktemp -t ggstop).py"
	cat > "$TMP" <<'PY'
import unreal
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
playing = level.is_in_play_in_editor()
if playing:
    level.editor_request_end_play()
"stopping" if playing else "not playing"
PY
	OUT="$("$ROOT/Tools/py.sh" "$TMP" 2>/dev/null)"
	rm -f "$TMP"
	case "$OUT" in *stopping*) sleep 3 ;; esac
	exit 0
fi
# One player, unless a co-op test asks for more (GG_PLAYERS=2). The editor remembers the last count, and a co-op test
# that failed to put it back once left every later run drawing the game twice: frame rates read 40% low for hours.
TMP="$(mktemp -t ggplayers).py"
sed "s/^PLAYERS = .*/PLAYERS = ${GG_PLAYERS:-1}/" "$ROOT/Tools/net_players.py" > "$TMP"
"$ROOT/Tools/py.sh" "$TMP" >/dev/null 2>&1
rm -f "$TMP"
python3 "$ROOT/Tools/mcp.py" call call_tool "{\"toolset_name\":\"EditorToolset.EditorAppToolset\",\"tool_name\":\"StartPIE\",\"arguments\":{\"options\":{\"warmupSeconds\":${1:-8}}}}" 2>&1 | head -1
# Automated play never touches the player's own save: it gets an empty one that is not written (GGTestSave).
"$ROOT/Tools/console.sh" "GGTestSave"
