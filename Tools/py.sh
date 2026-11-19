#!/bin/sh
# Runs a Python file inside the open editor and prints its output:  Tools/py.sh <file.py>
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
python3 "$ROOT/Tools/mcp.py" call call_tool "$(python3 -c 'import json,sys;print(json.dumps({"toolset_name":"editor_toolset.toolsets.python_scripting.PythonScriptingTools","tool_name":"execute_python","arguments":{"script":{"content":open(sys.argv[1]).read()}}}))' "$1")" 2>&1
