#!/bin/sh
# Packages the game for the web (Wasm + WebGPU) and plans a deploy:  Tools/web.sh [Development|Shipping] [host]
#   host: static | pages | r2worker | s3cf | netlify | zip   (the engine's deploy planner; it never uploads)
# Needs: an engine that supports the Wasm platform (the 5.9 preview's Mac launcher build does NOT; see docs/WEB.md)
# and the Emscripten SDK 6.0.10 at ~/emsdk (or WASM_EMSDK_ROOT).
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.9}"
CONFIG="${1:-Development}"
HOST="${2:-static}"
export WASM_EMSDK_ROOT="${WASM_EMSDK_ROOT:-$HOME/emsdk}"
if [ ! -f "$WASM_EMSDK_ROOT/upstream/emscripten/emscripten-version.txt" ]; then
	echo "No Emscripten SDK at $WASM_EMSDK_ROOT. Install 6.0.10: git clone https://github.com/emscripten-core/emsdk ~/emsdk && cd ~/emsdk && ./emsdk install 6.0.10 && ./emsdk activate 6.0.10" >&2
	exit 1
fi
MAPS="/Game/Levels/Terraces+/Game/Levels/SaltMirror+/Game/Levels/Canyon+/Game/Levels/Lantern+/Game/Levels/Returning"
"$UE/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun -project="$ROOT/GravityGolf.uproject" -platform=Wasm \
	-clientconfig="$CONFIG" -build -cook -map="$MAPS" -stage -pak -iostore -package -archive \
	-archivedirectory="$ROOT/Saved/Web" -deploy -WasmHost="$HOST" -unattended -utf8output -nop4
echo "Packaged to $ROOT/Saved/Web. Deploy plan: $ROOT/Saved/Wasm/Deploy"
