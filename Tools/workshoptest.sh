#!/bin/sh
# Works the Workshop (the level editor) in the open editor, without hands, and checks what it reports:
# every tool, undo, the hole code there and back, keeping and reopening, touch, the proof both ways (a hole that
# needs its machine, and the same hole with its wall taken out), and a test play that records the answer.
#   Tools/workshoptest.sh
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
python3 - "$ROOT" <<'PY'
import json, os, re, subprocess, sys, time

root = sys.argv[1]
log = os.path.expanduser("~/Library/Logs/Unreal Engine/GravityGolfEditor/GravityGolf.log")
failures = []


def sh(*args):
    return subprocess.run(list(args), capture_output=True, text=True).stdout


def lines_from(mark):
    with open(log, errors="replace") as f:
        return f.readlines()[mark:]


def mark():
    with open(log, errors="replace") as f:
        return len(f.readlines())


def console(command):
    sh(f"{root}/Tools/console.sh", command)


def state(command=None, wait=0.4):
    """Run a Workshop command (or none) and return the state the Workshop logs after it."""
    at = mark()
    console(f"GGWorkshop {command or 'state'}")
    for _ in range(40):
        time.sleep(wait if _ == 0 else 0.25)
        found = [l for l in lines_from(at) if "LogGGMode: Display: Workshop: " in l and " squares " in l]
        if found:
            line = found[-1]
            numbers = {k: int(v) for k, v in re.findall(r"(squares|machines|plates|gates|pads|solved|necessary|proving) (-?\d+)", line)}
            numbers["mode"] = "building" if "Workshop: building" in line else "testing"
            numbers["message"] = line.split("|", 1)[-1].strip()
            numbers["log"] = lines_from(at)
            return numbers
    return {"log": lines_from(at), "message": ""}


def check(name, ok, detail=""):
    print(("ok    " if ok else "FAIL  ") + name + (f"   ({detail})" if detail and not ok else ""))
    if not ok:
        failures.append(name)


def stroke(tool, numbers):
    console(f"GGWorkshop tool {tool}")
    return state(f"stroke {numbers}")


def touch(finger, phase, x, y):
    console(f"GGTouch {finger},{phase},{x},{y}")


sh(f"{root}/Tools/play.sh", "stop")
sh("python3", f"{root}/Tools/mcp.py", "call", "call_tool", json.dumps({
    "toolset_name": "EditorToolset.LevelToolset", "tool_name": "LoadLevel",
    "arguments": {"levelPath": "/Game/Levels/Workshop", "unsavedChangesPolicy": "Discard"}}))
sh(f"{root}/Tools/play.sh", "6")

s = state("new")
check("a new hole is the starter", s.get("squares") == 1120 and s.get("machines") == 1 and s.get("mode") == "building", str(s.get("message")))

# Every tool once.
s = stroke("wall", "700,-475,740,-120")
check("wall painted (ground count unchanged)", s.get("squares") == 1120)
s = stroke("ground", "2800,-200,3190,190")
check("ground laid", s.get("squares") == 1120 + 8 * 8, str(s.get("squares")))
stroke("sand", "1800,-480,2000,480")
s = stroke("lightplate", "925,325")
check("light plate placed", s.get("plates") == 1)
s = stroke("weightplate", "1825,-375")
check("weight plate placed", s.get("plates") == 2)
s = stroke("gate", "1325,275,1375,475")
check("gate drawn", s.get("gates") == 1)
s = stroke("gate", "1340,400")
check("clicking a gate hands it to the next plate", "plate 1" in s.get("message", ""), s.get("message"))
s = stroke("safepad", "2050,25")
check("safe pad placed", s.get("pads") == 1)
s = stroke("repulsor", "525,-275")
check("repulsor placed", s.get("machines") == 2)
s = stroke("repulsor", "9000,9000")
check("nothing can stand off the ground", s.get("machines") == 2 and "ground" in s.get("message", ""), s.get("message"))
stroke("move", "2425,25,2625,225")
s = stroke("erase", "1000,-480,1100,-300")
check("ground erased", s.get("squares") == 1184 - 3 * 5, str(s.get("squares")))
s = stroke("erase", "925,325")
check("erasing a plate takes its gate", s.get("plates") == 1 and s.get("gates") == 0, f"{s.get('plates')} plates, {s.get('gates')} gates")
s = state("undo")
check("undo brings them back", s.get("plates") == 2 and s.get("gates") == 1)

# The hole code, there and back.
before = state()
code_lines = [l for l in state("code")["log"] if "Workshop: code " in l]
code = code_lines[-1].split("Workshop: code ", 1)[1].strip() if code_lines else ""
check("the hole has a code", code.startswith("GG1;"), code)
state("new")
after = state(f"code {code}")
same = all(before.get(k) == after.get(k) for k in ("squares", "machines", "plates", "gates", "pads"))
check("the code opens as the same hole", same, f"{before} vs {after}")
print(f"      code is {len(code)} characters")

# Keeping and reopening.
state("name Test Bench")
state("keep")
state("new")
s = state("open Test Bench")
check("a kept hole reopens", s.get("plates") == 2 and s.get("pads") == 1 and "Opened" in s.get("message", ""), s.get("message"))

# Touch: the first tap switches to touch controls; then a tool tile and a drag on the ground.
s = state("new")
touch(0, 0, 0.9, 0.5); touch(0, 2, 0.9, 0.5)
touch(0, 0, 0.235, 0.905); touch(0, 2, 0.235, 0.905)
s = state()
check("a tap on a tool tile chooses it", "sand" in s.get("message", "").lower(), s.get("message"))
touch(0, 0, 0.60, 0.30); touch(0, 1, 0.62, 0.45); touch(0, 1, 0.64, 0.62); touch(0, 2, 0.64, 0.62)
code_lines = [l for l in state("code")["log"] if "Workshop: code " in l]
check("a finger's drag paints", any(";s" in l for l in code_lines), str(code_lines[-1:]))
painted = [l for l in state("code")["log"] if "Workshop: code " in l][-1].split("Workshop: code ", 1)[1]
touch(0, 0, 0.45, 0.5); touch(1, 0, 0.55, 0.5); touch(0, 1, 0.40, 0.5); touch(1, 1, 0.60, 0.5); touch(0, 2, 0.40, 0.5); touch(1, 2, 0.60, 0.5)
state("frame")
after = [l for l in state("code")["log"] if "Workshop: code " in l][-1].split("Workshop: code ", 1)[1]
check("two fingers move the view without painting", painted == after, f"{painted} vs {after}")

# The proof, both ways, and a test play that records the answer.
state("new")
state("check")
for _ in range(60):
    s = state(wait=1.0)
    if not s.get("proving"):
        break
check("the starter needs its machine", s.get("necessary") == 1, s.get("message"))
s = state("test")
check("the test starts", s.get("mode") == "testing")
console("GGMachine 1650,-50,0")
console("GGShot 44,0.15")
time.sleep(9)
s = state()
check("solving it in the test records the answer", s.get("solved") == 1)
s = state("build")
check("back at the table the proof stands", s.get("mode") == "building" and s.get("solved") == 1 and s.get("necessary") == 1)
s = stroke("ground", "1300,-250,1399,249")
check("changing the hole drops its proof", s.get("solved") == 0 and s.get("necessary") == 0)
state("check")
for _ in range(60):
    s = state(wait=1.0)
    if not s.get("proving"):
        break
check("without its wall a plain shot does it", s.get("necessary") == -1 and "Not a puzzle" in s.get("message", ""), s.get("message"))
state("new")
sh(f"{root}/Tools/play.sh", "stop")
print("PASS" if not failures else f"FAIL: {len(failures)} checks")
sys.exit(1 if failures else 0)
PY
