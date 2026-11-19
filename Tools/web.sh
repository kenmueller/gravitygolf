#!/bin/sh
# Packages the game for the web (Wasm + WebGPU), following Engine/Platforms/Wasm/Docs/GettingStarted.md:
#   Tools/web.sh [Development|Shipping] [host]
#   host: static | pages | r2worker | s3cf | netlify | zip   (the engine's deploy planner; it plans, it never uploads)
# The build tools (UnrealBuildTool, AutomationTool) are the launcher install's prebuilt copies, placed in the source
# tree: compiling them needs an Intel-only helper and this Mac has no Rosetta. Hence -nocompileuat -nocompile.
# Needs an engine with the web libraries (a source build at ~/UE59; the 5.9 preview's Mac launcher build has none) and
# the Emscripten SDK 6.0.10 at ~/emsdk. Works in a copy of the project (../gravitygolf-web) so the build against the
# source engine never touches the binaries the launcher editor uses.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE="${UE_ROOT:-$HOME/UE59}"
WEB="${GG_WEB_COPY:-$ROOT/../gravitygolf-web}"
CONFIG="${1:-Development}"
HOST="${2:-static}"
export WASM_EMSDK_ROOT="${WASM_EMSDK_ROOT:-$HOME/emsdk}"
export DOTNET_CLI_TELEMETRY_OPTOUT=1
if [ ! -f "$WASM_EMSDK_ROOT/upstream/emscripten/emscripten-version.txt" ]; then
	echo "No Emscripten SDK at $WASM_EMSDK_ROOT (need 6.0.10; see docs/WEB.md)" >&2
	exit 1
fi
if [ ! -d "$UE/Engine/Platforms/Wasm/Build/Template" ]; then
	echo "No web-capable engine at $UE (need a source build; see docs/WEB.md)" >&2
	exit 1
fi

# The copy: everything but build output and history. The MCP toolset plugins are editor tools and break a web cook
# ("Asset manager settings do not include a rule for assets of type GameFeatureData"), so the copy leaves them out.
mkdir -p "$WEB"
rsync -a --delete --exclude .git --exclude Saved --exclude Intermediate --exclude Binaries --exclude DerivedDataCache \
	--exclude legacy "$ROOT/" "$WEB/"
python3 - "$WEB/GravityGolf.uproject" <<'PY'
import json, sys
path = sys.argv[1]
project = json.load(open(path))
project["Plugins"] = [p for p in project["Plugins"] if p["Name"] not in ("ModelContextProtocol", "AllToolsets")]
project.pop("EngineAssociation", None)
json.dump(project, open(path, "w"), indent="\t")
PY

MAPS="/Game/Levels/Terraces+/Game/Levels/SaltMirror+/Game/Levels/Canyon+/Game/Levels/Lantern+/Game/Levels/Returning+/Game/Levels/Brief"
OUT="$WEB/Saved/Web"
"$UE/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun -project="$WEB/GravityGolf.uproject" -platform=Wasm \
	-clientconfig="$CONFIG" -build -cook -map="$MAPS" -stage -pak -iostore -package -archive -archivedirectory="$OUT" \
	-deploy -WasmHost="$HOST" -unattended -utf8output -nop4 -nocompileuat -nocompile \
	-AdditionalCookerOptions="-unattended -nopause -nosplash -nullrhi"
"$UE/Engine/Binaries/ThirdParty/Python3/Mac/bin/python3" "$UE/Engine/Platforms/Wasm/Build/Tools/deploy/check_stage.py" "$OUT/Wasm"
echo "Site: $OUT/Wasm    Play locally: $OUT/play-gravitygolf.sh    Deploy plan: $WEB/Saved/Wasm/Deploy"
