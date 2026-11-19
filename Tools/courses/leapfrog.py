#!/usr/bin/env python3
"""Hole 11, Leapfrog.

THE IDEA: "a grille stops what you carry, and the weight that opens it is a machine too." A walled court cut in
three by two walls. The ball's way to the cup is one straight line through a gate in each wall, and each gate opens
only while a machine stands on a plate in the middle yard. So both machines have to get into the middle yard.

The first wall has two grilles: gates with rails across them, which stop a machine in your arms as well as the ball.
One opens while a machine stands on a plate on the near side; the other is the ball's own gate, opened from the far
side. One machine holds the near plate while the other is carried through; that one, set on its plate beyond, holds
the second grille open for the first to follow. Lift either at the wrong moment and the way shuts.

The swing is the easy part, on purpose: both machines end up on the ball's line, and a ball tapped at an attractor is
drawn through it. Everything here is the order of the carries.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from coursekit import Course  # noqa: E402

c = Course("leapfrog", "Leapfrog", "Leapfrog", next_level="AfterYou")
X0, X1, HALF = 0, 4200, 700
c.box("Floor", ((X0 + X1) / 2, 0, -15), (X1 - X0 + 400, 2 * HALF + 400, 30), "green", "paving")
H = 300


def wall(name, x0, y0, x1, y1):
    c.box(name, ((x0 + x1) / 2, (y0 + y1) / 2, H / 2), (x1 - x0, y1 - y0, H), "sand", "wall")


# The court is walled all round: nothing is carried except through the grilles.
wall("Outer_S", X0 - 200, -HALF - 200, X1 + 200, -HALF)
wall("Outer_N", X0 - 200, HALF, X1 + 200, HALF + 200)
wall("Outer_W", X0 - 200, -HALF, X0, HALF)
wall("Outer_E", X1, -HALF, X1 + 200, HALF)

c.set_tee(300, 0)

# ---- The plates. Near: holds the side grille. Middle yard: one holds the ball's grille, one the ball's gate.
NEAR = (900, -400)
FIRST = (2000, 0)
SECOND = (2550, 0)
near = c.switch(*NEAR, radius=75, weight=True)
first = c.switch(*FIRST, radius=75, weight=True)
second = c.switch(*SECOND, radius=75, weight=True)

# ---- The first wall: the side grille to the north, the ball's grille on the line.
W1 = 1500
wall("First_S", W1 - 50, -HALF, W1 + 50, -100)
wall("First_M", W1 - 50, 100, W1 + 50, 350)
wall("First_N", W1 - 50, 550, W1 + 50, HALF)
c.gate("SideGrille", (W1, 450, 110), (60, 200, 220), near, grille=True)
c.gate("BallGrille", (W1, 0, 110), (60, 200, 220), first, grille=True)

# ---- The second wall: a plain gate for the ball.
W2 = 3000
wall("Second_S", W2 - 50, -HALF, W2 + 50, -90)
wall("Second_N", W2 - 50, 90, W2 + 50, HALF)
c.gate("BallGate", (W2, 0, 110), (60, 180, 220), second)

c.set_cup(3700, 0, court=200)

STARTS = [(550, 400), (550, -150)]
SPARE = [(1100, 350), (2300, -450), (2300, 450), (3500, 450)]
for spot in STARTS + [NEAR, FIRST, SECOND] + SPARE:
    c.plinth(*spot, radius=65)
# Small fields: these two are weights first. On the ball's line they hand it on, one to the next.
c.machine(*STARTS[0], strength=220, reach=700)
c.machine(*STARTS[1], strength=220, reach=700)
c.placement_area(100, -HALF + 100, X1 - 100, HALF - 100)
c.solution(0, [SECOND, FIRST], yaw=0, power=0.2)
c.extra["ambience"] = "night"
c.write()
