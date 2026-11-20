#!/bin/sh
# Works the Workshop (the level editor) in the open editor, without hands, and checks what it reports:
# every tool, undo, the hole code there and back, keeping and reopening, touch, the proof both ways (a hole that
# needs its machine, and the same hole with its wall taken out), a test play that records the answer, and a hole
# with a grille and plinths played through: the carrying rules, its proof, and its answer replayed.
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
            numbers = {k: int(v) for k, v in re.findall(r"(squares|machines|plates|gates|rails|plinths|stations|pads|solved|necessary|luck|proving) (-?\d+)", line)}
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
s = stroke("mirror", "1525,-425")
check("mirror placed", s.get("machines") == 3)
code_lines = [l for l in state("code")["log"] if "Workshop: code " in l]
s = stroke("mirror", "1525,-425")
turned = [l for l in state("code")["log"] if "Workshop: code " in l]
check("clicking a mirror turns it", s.get("machines") == 3 and code_lines[-1:] != turned[-1:] and ";m" in turned[-1], str(turned[-1:]))
state("undo")
state("undo")
s = stroke("repulsor", "9000,9000")
check("nothing can stand off the ground", s.get("machines") == 2 and "ground" in s.get("message", ""), s.get("message"))
stroke("move", "2425,25,2625,225")
s = stroke("erase", "1000,-480,1100,-300")
check("ground erased", s.get("squares") == 1184 - 3 * 5, str(s.get("squares")))
s = stroke("erase", "925,325")
check("erasing a plate takes its gate", s.get("plates") == 1 and s.get("gates") == 0, f"{s.get('plates')} plates, {s.get('gates')} gates")
s = state("undo")
check("undo brings them back", s.get("plates") == 2 and s.get("gates") == 1)

# Plinths, and rails on gates.
s = stroke("plinth", "2225,-325")
check("plinth placed, and the Workshop says what that changes", s.get("plinths") == 1 and "only on a plinth" in s.get("message", ""), s.get("message"))
check("the machines' starts and the weight plates count as plinths too", s.get("stations") == 1 + s.get("machines", 0) + 1, str(s.get("stations")))
s = stroke("plinth", "2225,-325")
check("no second plinth on the same square", s.get("plinths") == 1 and "already" in s.get("message", ""), s.get("message"))
s = stroke("grille", "1340,400")
check("clicking a gate with the grille tool gives it rails", s.get("rails") == 1 and s.get("gates") == 1, s.get("message"))
s = stroke("grille", "1340,400")
check("and clicking again takes them off", s.get("rails") == 0 and s.get("gates") == 1, s.get("message"))
s = stroke("grille", "1325,-475,1375,-325")
check("a grille drawn across a gap", s.get("rails") == 1 and s.get("gates") == 2, s.get("message"))
s = stroke("move", "2225,-325,2325,-325")
s = stroke("erase", "2325,-325")
check("a plinth is moved, then erased", s.get("plinths") == 0, str(s.get("plinths")))
s = state("undo")
check("undo brings it back", s.get("plinths") == 1)

# The hole code, there and back.
before = state()
code_lines = [l for l in state("code")["log"] if "Workshop: code " in l]
code = code_lines[-1].split("Workshop: code ", 1)[1].strip() if code_lines else ""
check("the hole has a code", code.startswith("GG1;"), code)
state("new")
after = state(f"code {code}")
same = all(before.get(k) == after.get(k) for k in ("squares", "machines", "plates", "gates", "rails", "plinths", "stations", "pads"))
check("the code carries the plinth and the rails", ";o" in code and after.get("rails") == 1 and after.get("plinths") == 1, code)
check("the code opens as the same hole", same, f"{before} vs {after}")
print(f"      code is {len(code)} characters")

# Keeping and reopening.
state("name Test Bench")
state("keep")
state("new")
s = state("open Test Bench")
check("a kept hole reopens", s.get("plates") == 2 and s.get("pads") == 1 and s.get("plinths") == 1 and s.get("rails") == 1
      and "Opened" in s.get("message", ""), s.get("message"))

# Touch: the first tap switches to touch controls; then a tool tile and a drag on the ground.
s = state("new")
touch(0, 0, 0.9, 0.5); touch(0, 2, 0.9, 0.5)
# (The row of tools is as wide as the screen allows, so where a tile lies depends on the window: the sand tile is
# the third of fifteen, somewhere in the left quarter. Tap across until it answers.)
for step in range(26):
    x = 0.10 + 0.01 * step
    touch(0, 0, x, 0.905); touch(0, 2, x, 0.905)
    s = state()
    if "sand" in s.get("message", "").lower():
        break
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
check("and is rated (it is an easy one)", s.get("luck", -1) > 15, f"luck {s.get('luck')}: {s.get('message')}")
print(f"      {s.get('luck')} in 100 random placements work")
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

# A hole with a grille and plinths, played through. A strip of ground cut in two by a wall with one grille in it; the
# grille's weight plate is behind the tee, where a machine standing on it never bends the ball.
RAILS = "GG1;nRails;f0,-10,56,20;w26,-10,2,9;w26,1,2,9;t6,0;c48,0;k2,0;g26,-1,2,2,0,1;o28,4;o40,-4;a2,6;a2,-6"
s = state(f"code {RAILS}")
check("a hole with a grille opens from its code", s.get("rails") == 1 and s.get("plinths") == 2 and s.get("stations") == 5, s.get("message"))
s = state("test")


def where():
    at = mark()
    console("GGWhere")
    time.sleep(0.8)
    found = [l for l in lines_from(at) if "Where: at" in l]
    m = re.search(r"Where: at (-?\d+),(-?\d+),.* carrying (\d)", found[-1]) if found else None
    return (int(m.group(1)), int(m.group(2)), int(m.group(3))) if m else (None, None, None)


def go(x, y):
    # A straight step, which the game's own rule allows or puts back.
    console(f"GGView {x},{y},100,0,0")
    time.sleep(0.6)


def net(command):
    at = mark()
    console(f"GGNet {command}")
    time.sleep(0.8)
    found = [l for l in lines_from(at) if "Net[server]" in l]
    return found[-1].split("Net[server]", 1)[1].strip() if found else ""


net("carry 0")
go(1200, 0); go(1500, 0)
check("a shut grille stops a carried machine", where()[0] == 1200, str(where()))
go(1200, -300); go(1200, -700)
check("a carried machine stays on the hole's ground", where()[1] == -300, str(where()))
line = net("place 1000,0,0")
check("on a hole with plinths a machine stands only on one", "carrying 0" in line, line)
go(200, 100)
line = net("place 125,25,0")
check("a weight plate is a plinth", "carrying -1" in line and "(125,25)" in line, line)
net("carry 1")
go(1200, 0); go(1500, 0)
check("with the plate held, the grille lets a carried machine through", where()[0] == 1500, str(where()))
go(2000, -100)
line = net("place 2025,-175,0")
check("it stands on the plinth beyond", "carrying -1" in line and "(2025,-175)" in line, line)
go(1500, 0); go(1200, 0); go(1250, 225)
line = net("pick 1")
check("no reaching through a wall for a machine", "carrying -1" in line, line)
go(1200, 0); go(1500, 0); go(1900, -150)
line = net("pick 1")
check("from its own side of the wall it is picked up", "carrying 1" in line, line)
go(1500, 225)
line = net("place 1425,225,0")
check("a plinth hard against a wall takes a machine", "carrying -1" in line and "(1425,225)" in line, line)
go(1500, 0); go(1200, 0); go(1250, 225)
line = net("pick 1")
check("and it is not taken through the wall it stands against", "carrying -1" in line, line)
go(1200, 0); go(1500, 0); go(1500, 225)
net("pick 1")
go(1500, 0); go(1200, 0); go(200, -200)
line = net("place 125,-275,0")
check("the machine is carried back to where it began", "carrying -1" in line and "(125,-275)" in line, line)
net("goto")
console("GGShot 0,0.25")
time.sleep(8)
s = state()
check("the straight putt through the open grille solves it", s.get("solved") == 1, s.get("message"))
state("build")
state("check")
for _ in range(60):
    s = state(wait=1.0)
    if not s.get("proving"):
        break
check("the grille hole needs its machines, and is rated", s.get("necessary") == 1 and s.get("luck", -1) >= 0, s.get("message"))
print(f"      {s.get('luck')} in 100 random ways of standing the machines on its plinths work")
state("test")
at = mark()
console("GGDemo")
verdict = []
for _ in range(40):
    time.sleep(0.5)
    verdict = [l for l in lines_from(at) if "Demo: PASS" in l or "Demo: FAIL" in l]
    if verdict:
        break
carried = [l for l in lines_from(at) if "Demo: carried machine" in l]
check("its answer replays, the machines carried in an order the grille allows", bool(verdict) and "PASS" in verdict[-1] and len(carried) >= 1,
      (verdict[-1:] or ["no verdict"])[0].strip())
state("build")
state("new")
sh(f"{root}/Tools/play.sh", "stop")
print("PASS" if not failures else f"FAIL: {len(failures)} checks")
sys.exit(1 if failures else 0)
PY
