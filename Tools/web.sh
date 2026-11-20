#!/bin/sh
# Packages the game for the web (Wasm + WebGPU), following Engine/Platforms/Wasm/Docs/GettingStarted.md:
#   Tools/web.sh [Development|Shipping] [host]
#   host (optional): static | pages | r2worker | s3cf | netlify | zip. Names a host to also write a deploy plan for;
#   the engine's planner never uploads. Without it the build is for local testing only.
# The build tools (UnrealBuildTool, AutomationTool) are the launcher install's prebuilt copies, placed in the source
# tree: compiling them needs an Intel-only helper and this Mac has no Rosetta. Hence -nocompileuat -nocompile.
# Needs an engine with the web libraries (a source build at ~/UE59; the 5.9 preview's Mac launcher build has none) and
# the Emscripten SDK 6.0.10 at ~/emsdk. Works in a copy of the project (../gravitygolf-web) so the build against the
# source engine never touches the binaries the launcher editor uses.
# The whole script is one function, called on its last line: the shell reads all of it before running any of it, so
# editing this file while it runs cannot change or break the run in progress.
main() {
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE="${UE_ROOT:-$HOME/UE59}"
WEB="${GG_WEB_COPY:-$ROOT/../gravitygolf-web}"
CONFIG="${1:-Development}"
HOST="${2:-}"
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
# Config/Mac holds the rule that makes desktop builds cook the scanned plants. The cooker reads the host's settings,
# so on a Mac that rule would pull the scans into the web build as well: the web copy goes without it.
rsync -a --delete --exclude .git --exclude Saved --exclude Intermediate --exclude Binaries \
	--exclude DerivedDataCache --exclude legacy --exclude Config/Mac --exclude Source "$ROOT/" "$WEB/"
rm -rf "$WEB/Config/Mac"
# Source files are copied by content and never given the original's time: the build tool trusts times, and a file
# edited before the last web build but copied after it would otherwise look already built, and stale code would be
# linked. A changed file lands with the time of the copy, so it is always newer than what was built from it.
rsync -rlp --checksum --delete "$ROOT/Source/" "$WEB/Source/"
# The copy keeps file times, and the build tool compares times: a build rule edited before the last build would look
# unchanged and the link would go stale. Touch the rules so they are always re-read.
find "$WEB/Source" \( -name "*.Build.cs" -o -name "*.Target.cs" \) -exec touch {} +
python3 - "$WEB/GravityGolf.uproject" <<'PY'
import json, sys
path = sys.argv[1]
project = json.load(open(path))
project["Plugins"] = [p for p in project["Plugins"] if p["Name"] not in ("ModelContextProtocol", "AllToolsets")]
project.pop("EngineAssociation", None)
json.dump(project, open(path, "w"), indent="\t")
PY

# A deploy plan only when a host is named (and only a Shipping build passes the planner's release scan).
DEPLOY=""
if [ -n "$HOST" ]; then
	DEPLOY="-deploy -WasmHost=$HOST"
fi
MAPS="/Game/Levels/Terraces+/Game/Levels/SaltMirror+/Game/Levels/Canyon+/Game/Levels/Lantern+/Game/Levels/Returning+/Game/Levels/Brief+/Game/Levels/Colonnade+/Game/Levels/Stones+/Game/Levels/Glass+/Game/Levels/Periscope+/Game/Levels/Leapfrog+/Game/Levels/AfterYou+/Game/Levels/WaitHere+/Game/Levels/Wires+/Game/Levels/Workshop"
# GG_WEB_OUT puts the finished build somewhere else: a Shipping build can be made beside the Development one without
# touching the folder a local server (or a friend's link) is serving from.
OUT="${GG_WEB_OUT:-$WEB/Saved/Web}"
"$UE/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun -project="$WEB/GravityGolf.uproject" -platform=Wasm \
	-clientconfig="$CONFIG" -build -cook -map="$MAPS" -stage -pak -iostore -package -archive -archivedirectory="$OUT" \
	$DEPLOY -unattended -utf8output -nop4 -nocompileuat -nocompile \
	-AdditionalCookerOptions="-unattended -nopause -nosplash -nullrhi"
"$UE/Engine/Binaries/ThirdParty/Python3/Mac/bin/python3" "$UE/Engine/Platforms/Wasm/Build/Tools/deploy/check_stage.py" "$OUT/Wasm"
echo "Site: $OUT/Wasm    Play locally: $OUT/play-gravitygolf.sh"
}
main "$@"
