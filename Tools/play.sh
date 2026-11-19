#!/bin/sh
# Starts Play-In-Editor (or stops it with "stop") and waits for warm-up:  Tools/play.sh [seconds|stop]
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
python3 "$ROOT/Tools/mcp.py" call call_tool "{\"toolset_name\":\"EditorToolset.EditorAppToolset\",\"tool_name\":\"StartPIE\",\"arguments\":{\"options\":{\"warmupSeconds\":${1:-8}}}}" 2>&1 | head -1
