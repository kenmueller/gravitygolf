#!/bin/sh
# A gamepad in the web build, in Chrome (the harness's synthetic standard gamepad): the left stick walks the player
# forward and the right stick turns the view.
#   Tools/padtest.sh        (needs a Development web build; GG_WEB_OUT names its folder, as for Tools/webtest.sh)
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WEB="${GG_WEB_COPY:-$ROOT/../gravitygolf-web}"
"$ROOT/Tools/webtest.sh" gamepad >/dev/null 2>&1
LOG="$WEB/Saved/WebTest/gamepad/console.log"
XS="$(grep -h "where: me" "$LOG" 2>/dev/null | sed 's/.*where: me (\([-0-9]*\),.*/\1/' | tr '\n' ' ')"
YAWS="$(grep -h "Where: at" "$LOG" 2>/dev/null | sed 's/.*yaw \([-0-9.]*\) .*/\1/' | tr '\n' ' ')"
python3 - "$XS" "$YAWS" <<'PY'
import sys
xs = [float(v) for v in sys.argv[1].split()]
yaws = [float(v) for v in sys.argv[2].split()]
if len(xs) != 2 or len(yaws) != 2:
    print(f"FAIL  expected two positions and two facings, got {xs} and {yaws}")
    sys.exit(1)
checks = [
    ("the left stick walks the player forward", xs[1] - xs[0] > 300, f"x {xs[0]:.0f} -> {xs[1]:.0f}"),
    ("the right stick turns the view", abs(yaws[1] - yaws[0]) > 30, f"yaw {yaws[0]} -> {yaws[1]}"),
]
failed = False
for name, ok, detail in checks:
    print(("ok    " if ok else "FAIL  ") + name + f" ({detail})")
    failed = failed or not ok
print("FAIL" if failed else "PASS")
sys.exit(1 if failed else 0)
PY
