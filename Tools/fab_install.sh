#!/bin/sh
# Installs a free Fab listing into the open editor:  Tools/fab_install.sh <listingId> "<title>" [gltf|unreal-engine]
# glTF packs with several quality tiers install the "high" file.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
python3 - "$ROOT" "$1" "$2" "${3:-gltf}" <<'PY'
import json, subprocess, sys
root, listing, title, fmt = sys.argv[1:5]
def call(args):
    payload = json.dumps({"toolset_name": "FabToolset.FabToolset", "tool_name": "InstallFabListing", "arguments": args})
    out = subprocess.run(["python3", f"{root}/Tools/mcp.py", "call", "call_tool", payload], capture_output=True, text=True).stdout
    try:
        return json.loads(json.loads(out)["returnValue"])
    except Exception:
        print(out[:600]); sys.exit(1)
args = {"listingId": listing, "assetFormat": fmt, "title": title}
if fmt == "unreal-engine":
    args.update({"engineVersion": "UE_5.8", "distributionMethod": "asset_pack"})
r = call(args)
if r.get("status") == "choose-file":
    pick = next((f for f in r["files"] if "high" in f["fileName"]), r["files"][0])
    args["fileId"] = pick["fileId"]
    r = call(args)
print(r.get("status"), "-", title)
PY
