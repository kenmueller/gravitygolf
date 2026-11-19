#!/bin/sh
# Starts the Unreal Editor on this project in the background and waits until its MCP server answers and the
# editor is idle. Safe to call when it's already running.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.9}"
# Start one only if no editor process exists; a starting editor has no MCP server yet.
if ! ps -axo comm | grep -q "MacOS/UnrealEditor$"; then
	(nohup "$UE/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" "$ROOT/GravityGolf.uproject" >/dev/null 2>&1 &)
fi
i=0
while [ $i -lt 170 ]; do
	R=$(python3 "$ROOT/Tools/mcp.py" call call_tool '{"toolset_name":"EditorToolset.EditorAppToolset","tool_name":"IsPIERunning","arguments":{}}' 2>&1)
	case "$R" in
	*returnValue*) echo "editor ready after about $((i * 3))s"; exit 0 ;;
	esac
	sleep 3
	i=$((i + 1))
done
echo "editor not ready: $R" | cut -c1-300
exit 1
