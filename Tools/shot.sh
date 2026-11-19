#!/bin/sh
# Saves a picture of the running game (Play-In-Editor):  Tools/shot.sh <Level> <out.png> ["x,y,z,pitch,yaw"]
# With a view, the player is moved there first. Play must already be running (Tools/play.sh).
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [ -n "$3" ]; then
	"$ROOT/Tools/console.sh" "GGView $3"
	sleep 2
fi
MCP_SAVE_IMAGE="$2" python3 "$ROOT/Tools/mcp.py" call call_tool "{\"toolset_name\":\"SceneCaptureToolset.SceneCaptureToolset\",\"tool_name\":\"CaptureGameViewport\",\"arguments\":{\"world\":{\"refPath\":\"/Game/Levels/UEDPIE_0_$1.$1\"},\"maxResolution\":1600}}" 2>&1 | tail -2
