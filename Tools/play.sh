#!/bin/sh
# Starts Play-In-Editor (or stops it with "stop") and waits for warm-up:  Tools/play.sh [seconds|stop]
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [ "$1" = "stop" ]; then
	python3 "$ROOT/Tools/mcp.py" call call_tool '{"toolset_name":"EditorToolset.EditorAppToolset","tool_name":"StopPIE","arguments":{}}' >/dev/null 2>&1
	exit 0
fi
python3 "$ROOT/Tools/mcp.py" call call_tool "{\"toolset_name\":\"EditorToolset.EditorAppToolset\",\"tool_name\":\"StartPIE\",\"arguments\":{\"options\":{\"warmupSeconds\":${1:-8}}}}" 2>&1 | head -1
