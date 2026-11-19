#!/bin/sh
# Regenerates levels inside the open editor, then saves:  Tools/regen.sh <level|all> [--no-materials]
# Without --no-materials the shared materials are rebuilt first and then every level, because rebuilding the
# materials resets the levels' material instances.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LEVEL="${1:-terraces}"
python3 - "$ROOT" "$LEVEL" "$2" <<'PY'
import json, subprocess, sys
root, level, flag = sys.argv[1], sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else ""
# Rebuilding the materials resets every level's material instances, so it is always followed by every level.
EVERY = ["terraces", "saltmirror", "canyon", "lantern", "returning"]
levels = EVERY if level == "all" or flag != "--no-materials" else [level]
code = "import runpy\n"
if flag != "--no-materials":
    code += f"runpy.run_path('{root}/Tools/build_materials.py', run_name='__main__')\n"
for name in levels:
    code += f"runpy.run_path('{root}/Tools/levels/{name}.py', run_name='__main__')\n"
code += "unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)\n'regenerated'\n"
payload = json.dumps({"toolset_name": "editor_toolset.toolsets.python_scripting.PythonScriptingTools",
                      "tool_name": "execute_python", "arguments": {"script": {"content": code}}})
out = subprocess.run(["python3", f"{root}/Tools/mcp.py", "call", "call_tool", payload], capture_output=True, text=True)
print((out.stdout + out.stderr).strip()[-700:])
PY
