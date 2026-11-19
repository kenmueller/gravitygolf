#!/usr/bin/env python3
"""Writes Content/Courses/terraces.json: the collision and rules for hole 1, The Terraces.

THE IDEA: "pull it round the corner." The cup sits in a walled court. The only gap is in the court's right-hand
wall, hidden from the tee by a wing wall. A putt can pass the wing but can never turn in; hedges swallow any bank
shot. Standing the gravity machine in the right place bends the ball through the gap and over the cup.

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

# The court: four limestone walls around the cup. The only way in is a gap in the right-hand wall, hidden from
# the tee by a wing wall that juts out from the court's near corner.
COURT_X0, COURT_X1, COURT_HALF = 2900, 4300, 420
GAP_X0, GAP_X1 = 3400, 3800
box("Court_Back", (COURT_X0 + WALL / 2, 0, 110), (WALL, 2 * COURT_HALF + 2 * WALL, 220))
box("Court_Front", (COURT_X1 - WALL / 2, 0, 110), (WALL, 2 * COURT_HALF + 2 * WALL, 220))
box("Court_SideL", ((COURT_X0 + COURT_X1) / 2, -(COURT_HALF + WALL / 2), 110), (COURT_X1 - COURT_X0, WALL, 220))
box("Court_SideR_Near", ((COURT_X0 + GAP_X0) / 2, COURT_HALF + WALL / 2, 110), (GAP_X0 - COURT_X0, WALL, 220))
box("Court_SideR_Far", ((GAP_X1 + COURT_X1) / 2, COURT_HALF + WALL / 2, 110), (COURT_X1 - GAP_X1, WALL, 220))
WING_END = 900
box("Wing", (COURT_X0 + WALL / 2, (COURT_HALF + WALL + WING_END) / 2, 110), (WALL, WING_END - COURT_HALF - WALL, 220))

PLAZA = (3600, -900)

course = {
    "id": "terraces",
    "name": "The Terraces",
    "par": 1,
    "ballRadius": 6,
    "gravity": [0, 0, -980],
    "killZ": -1400,
    "tee": {"position": [600, 0, 6], "yaw": 0},
    "cup": {"position": [3600, 0, 0], "radius": 18},
    "plaza": {"position": [PLAZA[0], PLAZA[1], 0], "radius": 700},
    # The machine starts on a dais beside the tee, so the player has to carry it somewhere and think about where.
    "attractors": [
        {"position": [1100, 800, 0], "strength": 1.35e8, "range": 1500, "coreHeight": 45, "movable": True},
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
    tee, cup = course["tee"]["position"], course["cup"]["position"]
    f.write(f"{tee[0]} {tee[1]} {tee[2]}\n{cup[0]} {cup[1]} {cup[2]} {course['cup']['radius']}\n")
    f.write(f"{course['ballRadius']} {course['killZ']}\n")
    a = course["attractors"][0]
    f.write(f"{a['strength']} {a['range']} {a['coreHeight']}\n")
    # Where a machine may stand: the lawn, inside the hedges.
    f.write(f"200 {-WIDTH / 2 + 200} {LENGTH - 200} {WIDTH / 2 - 200}\n")
    f.write(f"{len(boxes)}\n")
    for b in boxes:
        c, s = b["center"], b["size"]
        f.write(" ".join(str(v) for v in (c[0] - s[0] / 2, c[1] - s[1] / 2, c[2] - s[2] / 2, c[0] + s[0] / 2,
                                           c[1] + s[1] / 2, c[2] + s[2] / 2, SURFACES[b["surface"]])) + "\n")
print(f"wrote {os.path.normpath(out)} ({len(boxes)} boxes) and {os.path.normpath(flat)}")
