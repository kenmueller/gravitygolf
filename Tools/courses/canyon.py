#!/usr/bin/env python3
"""Hole 3, The Narrows.

THE IDEA: "pull, then push." Two parallel galleries in a slot canyon, joined by one doorway in the rock between
them. A fin of rock hides the doorway from the tee, and soft sand drifts line every wall, so nothing banks. The
ball has to swerve one way through the doorway and then straighten the other way to reach the cup. One attractor
and one repulsor: neither can make both turns alone.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from coursekit import Course  # noqa: E402

c = Course("canyon", "The Narrows", "Canyon", next_level="Lantern")
FLOOR_Z = -15
WALL_H = 400

# Floor: one slab under everything.
c.box("Floor", (3000, 450, FLOOR_Z), (6400, 2300, 30), "green", "floor")


def wall(name, x0, y0, x1, y1, drift=True):
    """A rock wall with a soft sand drift along it, so the ball never bounces back into play."""
    c.box(name, ((x0 + x1) / 2, (y0 + y1) / 2, WALL_H / 2), (x1 - x0, y1 - y0, WALL_H), "sand", "rock")


# Lower gallery (where the tee is): y from -400 to 400. Upper gallery: y from 500 to 1300.
wall("Wall_South", -200, -700, 6200, -400)
wall("Wall_North", -200, 1300, 6200, 1600)
wall("Wall_West", -200, -400, 0, 1300)
wall("Wall_East", 6000, -400, 6200, 1300)
# The rock between the galleries, with a doorway from x = 2300 to x = 3100.
wall("Divide_Near", 0, 400, 2300, 500)
wall("Divide_Far", 3100, 400, 6000, 500)
# A fin of rock juts into the lower gallery and hides the doorway from the tee.
wall("Fin", 1300, -60, 1420, 400)
# The lower gallery ends just past the doorway, so there is nowhere else to go.
wall("Lower_End", 3600, -400, 6000, 400)

c.set_tee(450, 80)
c.set_cup(5300, 700, court=420)
c.machine(800, -250, strength=420, reach=1900)    # attractor
c.machine(1050, -250, strength=-420, reach=1500)  # repulsor
c.placement_area(150, -300, 5850, 1200)
# Intended answer: the attractor just inside the doorway, the repulsor against the far wall above it.
c.solution(0, [(2900, 700), (2850, 1200)], yaw=-20, power=0.575)
c.extra["ambience"] = "canyon"
c.write()
