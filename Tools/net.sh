#!/bin/sh
# Runs a console command on one player's machine in a Play-In-Editor session with several players:
#   Tools/net.sh <player|all> "<console command>"     player 0 is the server, 1 the first client
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WHO="$1"
[ "$WHO" = "all" ] && WHO=-1
TMP="$(mktemp -t ggnet).py"
cat > "$TMP" <<PY
import unreal
worlds = unreal.EditorLevelLibrary.get_pie_worlds(False)
ran = 0
for index, world in enumerate(worlds):
    if $WHO >= 0 and index != $WHO:
        continue
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    if pc:
        unreal.SystemLibrary.execute_console_command(pc, """$2""", pc)
        ran += 1
f"ran on {ran} of {len(worlds)} worlds"
PY
"$ROOT/Tools/py.sh" "$TMP" >/dev/null 2>&1
rm -f "$TMP"
