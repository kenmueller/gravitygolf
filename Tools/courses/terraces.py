#!/usr/bin/env python3
"""Writes Content/Courses/terraces.json: the collision and rules for hole 1, The Terraces.

THE IDEA: "it rolls toward the machine, so put the cup in the way." A wall hides the cup from the tee. A putt
can get round the end of the wall but then runs on into the hedge; hedges swallow any bank shot. The machine
can't stand on the paved court around the cup, so the answer is to stand it beyond the court: the ball is pulled
round the wall and rolls across the cup on its way to the machine.

The game simulates against these boxes, Tools/levels/terraces.py builds matching geometry from the same file, and
Tests/Sim/solve.sh proves the hole is impossible without the machine and solvable with it.
"""

import json
import os

LENGTH, WIDTH = 6400, 2600
WALL = 120
boxes = []


def box(name, center, size, surface="stone", look="stone", shape="box"):
    boxes.append({"name": name, "center": list(center), "size": list(size), "surface": surface, "look": look,
                  "shape": shape})


# One long lawn on a limestone body.
box("Lawn", (LENGTH / 2, 0, -15), (LENGTH, WIDTH, 30), "green", "green")
box("Body", (LENGTH / 2, 0, -30 - 1500), (LENGTH, WIDTH + 240, 3000))

# Hedges on three sides swallow the ball: no bank shots.
for side, tag in ((-1, "L"), (1, "R")):
    box(f"Hedge{tag}", (LENGTH / 2, side * (WIDTH / 2 - 60), 70), (LENGTH, 120, 140), "sand", "hedge")
box("HedgeEnd", (LENGTH - 60, 0, 70), (120, WIDTH, 140), "sand", "hedge")
box("HedgeStart", (60, 0, 70), (120, WIDTH, 140), "sand", "hedge")

# The great wall crosses the lawn from the left hedge, leaving a passage at its right-hand end.
WALL_X, PASSAGE_Y = 2400, 450
box("Great_Wall", (WALL_X + WALL / 2, (-WIDTH / 2 + 120 + PASSAGE_Y) / 2, 150), (WALL, PASSAGE_Y - (-WIDTH / 2 + 120), 300))

TEE = (600, -650)
CUP = (4000, -650)
COURT_RADIUS = 520

course = {
    "id": "terraces",
    "name": "The Terraces",
    "par": 1,
    "ballRadius": 6,
    "gravity": [0, 0, -980],
    "killZ": -1400,
    "tee": {"position": [TEE[0], TEE[1], 6], "yaw": 0},
    # A wide stone basin: finding the idea is the hard part, not threading a needle.
    "cup": {"position": [CUP[0], CUP[1], 0], "radius": 42, "captureSpeed": 650},
    # Machines can't stand on the paved court around the cup.
    "noMachine": [{"position": [CUP[0], CUP[1], 0], "radius": COURT_RADIUS}],
    # Optional checkpoints. This hole has none.
    "safeZones": [],
    # The machine waits on a dais near the tee, so the player has to carry it somewhere and think about where.
    "attractors": [
        {"position": [1100, 600, 0], "strength": 420, "range": 1900, "coreHeight": 45, "movable": True},
    ],
    "boxes": boxes,
}

root = os.path.join(os.path.dirname(__file__), "..", "..")
out = os.path.join(root, "Content", "Courses", "terraces.json")
with open(out, "w") as f:
    json.dump(course, f, indent="\t")
    f.write("\n")

# Plain-number copy for the standalone solver, which has no JSON parser.
SURFACES = {"grass": 0, "green": 1, "stone": 2, "sand": 3, "ice": 4}
flat = os.path.join(root, "Saved", "SimTest", "terraces.course")
os.makedirs(os.path.dirname(flat), exist_ok=True)
with open(flat, "w") as f:
    tee, cup = course["tee"]["position"], course["cup"]
    f.write(f"{tee[0]} {tee[1]} {tee[2]}\n")
    f.write(f"{cup['position'][0]} {cup['position'][1]} {cup['position'][2]} {cup['radius']} {cup['captureSpeed']}\n")
    f.write(f"{course['ballRadius']} {course['killZ']}\n")
    a = course["attractors"][0]
    f.write(f"{a['strength']} {a['range']} {a['coreHeight']}\n")
    # Where a machine may stand: the lawn, inside the hedges.
    f.write(f"200 {-WIDTH / 2 + 200} {LENGTH - 200} {WIDTH / 2 - 200}\n")
    f.write(f"{len(course['noMachine'])}\n")
    for z in course["noMachine"]:
        f.write(f"{z['position'][0]} {z['position'][1]} {z['radius']}\n")
    f.write(f"{len(course['safeZones'])}\n")
    for z in course["safeZones"]:
        f.write(f"{z['position'][0]} {z['position'][1]} {z['position'][2]} {z['radius']}\n")
    f.write(f"{len(boxes)}\n")
    for b in boxes:
        c, s = b["center"], b["size"]
        f.write(" ".join(str(v) for v in (c[0] - s[0] / 2, c[1] - s[1] / 2, c[2] - s[2] / 2, c[0] + s[0] / 2,
                                           c[1] + s[1] / 2, c[2] + s[2] / 2, SURFACES[b["surface"]])) + "\n")
print(f"wrote {os.path.normpath(out)} ({len(boxes)} boxes) and {os.path.normpath(flat)}")
