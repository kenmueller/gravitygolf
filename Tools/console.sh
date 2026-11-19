#!/bin/sh
# Runs a console command in the running editor/PIE session:  Tools/console.sh "GGSwing 44 0.275"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
python3 "$ROOT/Tools/mcp.py" call call_tool "$(python3 -c 'import json,sys;print(json.dumps({"toolset_name":"ConsoleToolset.ConsoleToolset","tool_name":"RunConsoleCommand","arguments":{"command":sys.argv[1]}}))' "$1")" >/dev/null 2>&1
