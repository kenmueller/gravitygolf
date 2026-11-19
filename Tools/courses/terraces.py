#!/usr/bin/env python3
"""Hole 1, The Terraces.

THE IDEA: "the machine bends the ball as it passes." A wall hides the cup from the tee. A putt can get round the
end of the wall but then runs on into the hedge; hedges swallow any bank shot. The machine can't stand on the paved
court around the cup, so the answer is to stand it past the wall on the inside of the turn: the ball is pulled
round the corner, released, and rolls on across the cup.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from coursekit import Course  # noqa: E402

LENGTH, WIDTH, WALL = 6400, 2600, 120
c = Course("terraces", "The Terraces", "Terraces", next_level="SaltMirror")

# One long lawn on a limestone body.
c.box("Lawn", (LENGTH / 2, 0, -15), (LENGTH, WIDTH, 30), "green", "green")
c.box("Body", (LENGTH / 2, 0, -30 - 1500), (LENGTH, WIDTH + 240, 3000))

# Hedges on all sides swallow the ball: no bank shots.
for side, tag in ((-1, "L"), (1, "R")):
    c.box(f"Hedge{tag}", (LENGTH / 2, side * (WIDTH / 2 - 60), 70), (LENGTH, 120, 140), "sand", "hedge")
c.box("HedgeEnd", (LENGTH - 60, 0, 70), (120, WIDTH, 140), "sand", "hedge")
c.box("HedgeStart", (60, 0, 70), (120, WIDTH, 140), "sand", "hedge")

# The great wall crosses the lawn from the left hedge, leaving a passage at its right-hand end.
WALL_X, PASSAGE_Y = 2400, 450
c.box("Great_Wall", (WALL_X + WALL / 2, (-WIDTH / 2 + 120 + PASSAGE_Y) / 2, 150),
      (WALL, PASSAGE_Y - (-WIDTH / 2 + 120), 300), look="wall")

c.set_tee(600, -650)
c.set_cup(4000, -650)
# The machine waits on a dais near the tee, so the player has to carry it somewhere and think about where.
c.machine(1100, 600)
c.placement_area(200, -WIDTH / 2 + 200, LENGTH - 200, WIDTH / 2 - 200)
c.write()
