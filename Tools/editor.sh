#!/bin/sh
# Starts exactly one Unreal Editor on this project and waits until its MCP server answers and the editor is idle.
# Safe to call when it's already running. Never run two copies of this script at once.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.9}"
LOG="$HOME/Library/Logs/Unreal Engine/GravityGolfEditor/GravityGolf.log"
EDITOR="$UE/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"

running() { ps -axo comm | grep -q "MacOS/UnrealEditor$"; }
ready() {
	python3 "$ROOT/Tools/mcp.py" call call_tool \
		'{"toolset_name":"EditorToolset.EditorAppToolset","tool_name":"IsPIERunning","arguments":{}}' 2>&1 | grep -q returnValue
}

attempt=1
while [ $attempt -le 4 ]; do
	if ! running; then
		# A crashed editor leaves its crash-report window open, and that window keeps the editor's MCP port. Close
		# any such window first, or the new editor starts without MCP.
		pkill -f CrashReportClient 2>/dev/null && sleep 2
		(nohup "$EDITOR" "$ROOT/GravityGolf.uproject" >/dev/null 2>&1 &)
		sleep 8
	fi
	i=0
	while [ $i -lt 170 ]; do
		if ready; then
			# Answering is not yet idle: a fresh editor goes on to load its first map and build meshes, and for those
			# seconds it queues what it is sent, or loses it (a test begun then sent its first commands to a game that
			# did not exist yet, and once play started in that window crashed the editor). Wait until the log has been
			# still for six seconds and the editor still answers.
			quiet=0
			waited=0
			size=$(wc -c <"$LOG" 2>/dev/null || echo 0)
			while [ $quiet -lt 3 ] && [ $waited -lt 90 ]; do
				sleep 2
				waited=$((waited + 1))
				now=$(wc -c <"$LOG" 2>/dev/null || echo 0)
				if [ "$now" = "$size" ] && ready; then quiet=$((quiet + 1)); else quiet=0; fi
				size=$now
			done
			echo "editor ready (launch attempt $attempt)"
			exit 0
		fi
		# After an editor closes, the OS can hold its port for up to a minute, and a new editor then starts
		# without MCP. Detect that, close it, wait, and try again.
		if grep -q "MCP server listener could not start" "$LOG" 2>/dev/null; then
			pkill -f "UnrealEditor.app/Contents/MacOS/UnrealEditor" 2>/dev/null
			sleep 5
			pkill -9 -f "UnrealEditor.app/Contents/MacOS/UnrealEditor" 2>/dev/null
			: >"$LOG"
			sleep 35
			break
		fi
		sleep 3
		i=$((i + 1))
	done
	attempt=$((attempt + 1))
done
echo "editor not ready after $((attempt - 1)) attempts"
exit 1
