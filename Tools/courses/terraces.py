#!/usr/bin/env python3
"""Writes Content/Courses/terraces.json: the collision and rules for The Terraces.

The game simulates against these boxes, and Tools/levels/terraces.py builds matching geometry from the same file.
"""

import json
import os

LENGTH, WIDTH, DROP = 3000, 2600, 320
boxes = []


def box(name, center, size, surface="stone", look="stone", shape="box"):
    boxes.append({"name": name, "center": list(center), "size": list(size), "surface": surface, "look": look,
                  "shape": shape})


for i in range(3):
    top = -DROP * i
    x = LENGTH * i + LENGTH / 2
    lawn = ("green", "green") if i == 2 else ("grass", "grass")
    box(f"Terrace{i + 1}_Lawn", (x, 0, top - 15), (LENGTH, WIDTH, 30), *lawn)
    box(f"Terrace{i + 1}_Body", (x, 0, top - 30 - 1500), (LENGTH, WIDTH + 240, 3000))
    for side, tag in ((-1, "L"), (1, "R")):
        box(f"Terrace{i + 1}_Parapet{tag}", (x, side * (WIDTH / 2 + 60), top + 40), (LENGTH, 120, 140))
    if i < 2:
        for step in range(8):
            box(f"Terrace{i + 1}_Step{step}", (LENGTH * (i + 1) + step * 45 + 22.5, -WIDTH / 2 + 250, top - step * 40 - 20),
                (45, 300, 40))

# The great wall across the middle terrace, open only at the right-hand end.
box("Ruin_Wall", (4500, -350, -DROP + 450), (180, 1900, 900))
box("Ruin_Wall_Stub", (4500, 1150, -DROP + 110), (180, 300, 220))

# End wall behind the green keeps long shots in play.
box("Green_Backstop", (9060, 0, -2 * DROP + 60), (120, WIDTH + 240, 180))

for n, y in enumerate(range(-1100, 1101, 440)):
    height = 650 if n % 3 != 1 else 260
    box(f"Column{n}", (8850, y, -2 * DROP + height / 2), (110, 110, height), shape="cylinder")

course = {
    "id": "terraces",
    "name": "The Terraces",
    "par": 3,
    "ballRadius": 6,
    "gravity": [0, 0, -980],
    "killZ": -1400,
    "tee": {"position": [600, 0, 6], "yaw": 0},
    "cup": {"position": [7600, 500, -2 * DROP], "radius": 18},
    "attractors": [
        {"position": [2300, 700, 120], "strength": 1.35e8, "range": 1300, "movable": True},
    ],
    "boxes": boxes,
}

out = os.path.join(os.path.dirname(__file__), "..", "..", "Content", "Courses", "terraces.json")
with open(out, "w") as f:
    json.dump(course, f, indent="\t")
    f.write("\n")
print(f"wrote {os.path.normpath(out)} ({len(boxes)} boxes)")
