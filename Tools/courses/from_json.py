#!/usr/bin/env python3
"""Turns a course JSON (a Workshop design, written by the console command `GGWorkshop json <file>`) into the plain
numbers the standalone solver reads, so a player-made hole can be put through the same proof as the game's own.

  python3 Tools/courses/from_json.py <design.json> [id]     writes Saved/SimTest/<id>.course   (id defaults to "design")
"""

import json
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from coursekit import ROOT, write_flat  # noqa: E402

with open(sys.argv[1]) as f:
    data = json.load(f)
# Machines may be tried anywhere on the ground: the boxes whose tops are where the ball rolls.
ground = [b for b in data["boxes"] if b["center"][2] + b["size"][2] / 2 < 20]
placement = [min(b["center"][0] - b["size"][0] / 2 for b in ground), min(b["center"][1] - b["size"][1] / 2 for b in ground),
             max(b["center"][0] + b["size"][0] / 2 for b in ground), max(b["center"][1] + b["size"][1] / 2 for b in ground)]
out = os.path.join(ROOT, "Saved", "SimTest", f"{sys.argv[2] if len(sys.argv) > 2 else 'design'}.course")
write_flat(data, placement, out)
print(f"wrote {out}")
