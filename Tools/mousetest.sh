#!/bin/sh
# Mouse look in the web build, in Chrome: small movements turn the view, one enormous jump (what a window reports when
# it first takes hold of the pointer) does not, and small movements still work after it.
#   Tools/mousetest.sh        (needs a Development web build; GG_WEB_OUT names its folder, as for Tools/webtest.sh)
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WEB="${GG_WEB_COPY:-$ROOT/../gravitygolf-web}"
"$ROOT/Tools/webtest.sh" mouse-look >/dev/null 2>&1
YAWS="$(grep -h "Where: at" "$WEB/Saved/WebTest/mouse-look/console.log" 2>/dev/null | sed 's/.*yaw \([-0-9.]*\) .*/\1/' | tr '\n' ' ')"
python3 - $YAWS <<'PY'
import sys
yaws = [float(v) for v in sys.argv[1:]]
if len(yaws) != 4:
    print(f"FAIL  expected four readings of the facing, got {len(yaws)}: {yaws}")
    sys.exit(1)
start, turned, jumped, after = yaws
checks = [
    ("small movements to the right turn the view right", turned - start > 20, f"{start} -> {turned}"),
    ("one jump of 1400 pixels is not taken for a look", abs(jumped - turned) < 1, f"{turned} -> {jumped}"),
    ("small movements still turn the view afterwards", after - jumped > 20, f"{jumped} -> {after}"),
]
failed = False
for name, ok, detail in checks:
    print(("ok    " if ok else "FAIL  ") + name + f" (yaw {detail})")
    failed = failed or not ok
print("FAIL" if failed else "PASS")
sys.exit(1 if failed else 0)
PY
