#!/usr/bin/env python3
"""Hole 5, The Long Way Round.

THE IDEA: "plan the whole trip before the first swing." The cup is a few steps from the tee, straight ahead, behind
a gate of light. The plate that opens it is at the far end of the canyon. Swing one: all the way down the upper
gallery, over the plate, round the end of the rock and onto the pad in the lower gallery, which banks the open
gate. Swing two: back along the lower gallery, up through the doorway and through the gate from behind. Both
machines, both swings, set up differently each time.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from coursekit import Course  # noqa: E402

c = Course("returning", "The Long Way Round", "Returning", next_level="Brief")
FLOOR_Z = -15
WALL_H = 400

c.box("Floor", (3000, 450, FLOOR_Z), (6400, 2300, 30), "green", "floor")


def wall(name, x0, y0, x1, y1):
    c.box(name, ((x0 + x1) / 2, (y0 + y1) / 2, WALL_H / 2), (x1 - x0, y1 - y0, WALL_H), "sand", "rock")


# Lower gallery: y from -400 to 400. Upper gallery: y from 500 to 1300.
wall("Wall_South", -200, -700, 6200, -400)
wall("Wall_North", -200, 1300, 6200, 1600)
wall("Wall_West", -200, -400, 0, 1300)
wall("Wall_East", 6000, -400, 6200, 1300)
# The rock between the galleries has one doorway.
DOOR_W, DOOR_E = 3100, 3900
wall("Divide_West", 0, 400, DOOR_W, 500)
wall("Divide_East", DOOR_E, 400, 6000, 500)
# A fin of rock hides the doorway from the tee; the way past it is along the south wall.
wall("Fin", 2100, -60, 2220, 400)
# The lower gallery ends past the doorway.
wall("Lower_End", 4400, -400, 6000, 400)
# The cup's chamber is the west end of the lower gallery, closed by the gate.
GATE_X = 850

plate = c.switch(4700, 1000, radius=150)
c.gate("Gate", (GATE_X, 0, 110), (60, 800, 220), plate)

c.set_tee(1250, 80)
c.safe_zone(5650, 700, radius=250, needs_switch=plate)
c.set_cup(400, -230, court=380)
c.machine(1450, -250, strength=420, reach=1900)    # attractor
c.machine(1700, -250, strength=-420, reach=1500)   # repulsor
c.placement_area(150, -300, 5850, 1200)
# Verified answers, one per leg.
c.solution(0, [(3200, 650), (3100, 1150)], yaw=-18, power=0.55)
c.solution(1, [(1150, -200), (3700, 950)], yaw=-175, power=0.675)
c.extra["ambience"] = "canyon"
c.write()
