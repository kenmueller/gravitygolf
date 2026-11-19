#!/bin/sh
# Regenerates materials and a level inside the open editor, then saves:  Tools/regen.sh [level script name, default terraces] [--no-materials]
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LEVEL="${1:-terraces}"
python3 - "$ROOT" "$LEVEL" "$2" <<'PY'
import json, subprocess, sys
root, level, flag = sys.argv[1], sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else ""
code = "import runpy\n"
if flag != "--no-materials":
    code += f"runpy.run_path('{root}/Tools/build_materials.py', run_name='__main__')\n"
code += f"runpy.run_path('{root}/Tools/levels/{level}.py', run_name='__main__')\n"
code += "unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)\n'regenerated'\n"
payload = json.dumps({"toolset_name": "editor_toolset.toolsets.python_scripting.PythonScriptingTools",
                      "tool_name": "execute_python", "arguments": {"script": {"content": code}}})
out = subprocess.run(["python3", f"{root}/Tools/mcp.py", "call", "call_tool", payload], capture_output=True, text=True)
print((out.stdout + out.stderr).strip()[-700:])
PY
