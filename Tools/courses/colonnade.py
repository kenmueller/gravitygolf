#!/usr/bin/env python3
"""Hole 7, Dead Weight.

THE IDEA: "a machine can be a weight, and you have to choose which one to spend." The gate to the cup's chamber is
held open by a heavy plate in an alcove: it stays down only while a machine stands on it. So one of your two
machines is left behind. Leave the attractor and nothing can carry the ball round the baffle and through the gate;
leave the repulsor and the attractor can do the whole job alone. (Checked: in 16 million random tries, no answer
leaves the attractor on the plate.)
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from coursekit import Course  # noqa: E402

c = Course("colonnade", "Dead Weight", "Colonnade", next_level="Stones")
WALL_H = 110  # low enough to see the water and the columns over

# The hall runs east. An alcove opens off its north side; a cross wall with a gate closes off the east chamber.
c.box("Floor", (3000, 250, -15), (6400, 2300, 30), "green", "floor")


def wall(name, x0, y0, x1, y1):
    c.box(name, ((x0 + x1) / 2, (y0 + y1) / 2, WALL_H / 2), (x1 - x0, y1 - y0, WALL_H), "sand", "wall")


wall("Wall_South", -200, -900, 6200, -700)
wall("Wall_West", -200, -700, 0, 700)
wall("Wall_East", 6000, -700, 6200, 700)
# North wall, with the alcove let into it.
ALCOVE_W, ALCOVE_E, ALCOVE_N = 2700, 3300, 1650
wall("Wall_North_W", -200, 700, ALCOVE_W, 900)
wall("Wall_North_E", ALCOVE_E, 700, 6200, 900)
wall("Alcove_W", ALCOVE_W - 200, 900, ALCOVE_W, ALCOVE_N)
wall("Alcove_E", ALCOVE_E, 900, ALCOVE_E + 200, ALCOVE_N)
wall("Alcove_N", ALCOVE_W - 200, ALCOVE_N, ALCOVE_E + 200, ALCOVE_N + 200)
# A baffle from the south wall hides the gate from the tee.
wall("Baffle", 2340, -700, 2460, 250)
# The cross wall: solid to the north, the gate to the south.
CROSS_X = 4200
wall("Cross", CROSS_X - 60, -100, CROSS_X + 60, 700)

plate = c.switch((ALCOVE_W + ALCOVE_E) / 2, 1350, radius=130, weight=True)
c.gate("Gate", (CROSS_X, -400, 110), (60, 600, 220), plate)

c.set_tee(500, 350)
c.set_cup(5400, -250, court=380)
c.machine(800, -350, strength=420, reach=1900)    # attractor
c.machine(1050, -350, strength=-420, reach=1500)  # repulsor
c.placement_area(150, -600, 5850, ALCOVE_N - 100)
# The repulsor stands on the plate and pushes from there; the attractor waits in the hall below it.
c.solution(0, [(3250, 150), (3000, 1350)], yaw=14, power=0.325)
c.write()
