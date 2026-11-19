#!/usr/bin/env python3
"""Hole 13, Wait Here.

THE IDEA: "a machine can be left halfway." A canyon floor cut in three. The ball's line runs straight through a
grille in the first rock and another in the second, and each opens only while a machine stands on a plate on the
ball's line: one before the first rock, one inside the last chamber.

The last chamber is shut by a grille of its own, and what opens that is a third plate back at the start. One machine
cannot hold the first grille and the chamber's grille at once. So the second machine is carried through the first
grille and set down in the middle to wait; the first machine changes plates, which shuts the way back and opens the
chamber; the second is carried in to its plate; and the first goes back to where it began. Five carries, and none
of them can be done in another order.

The swing is a tap along the line, handed from one machine to the next.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from coursekit import Course  # noqa: E402

c = Course("waithere", "Wait Here", "WaitHere", next_level="Wires")
X0, X1, HALF = 0, 4200, 800
c.box("Floor", ((X0 + X1) / 2, 0, -15), (X1 - X0 + 600, 2 * HALF + 600, 30), "green", "floor")
WALL_H = 400


def wall(name, x0, y0, x1, y1):
    c.box(name, ((x0 + x1) / 2, (y0 + y1) / 2, WALL_H / 2), (x1 - x0, y1 - y0, WALL_H), "sand", "rock")


# The canyon's own walls: nothing is carried except through the grilles.
wall("Wall_South", X0 - 300, -HALF - 300, X1 + 300, -HALF)
wall("Wall_North", X0 - 300, HALF, X1 + 300, HALF + 300)
wall("Wall_West", X0 - 300, -HALF, X0, HALF)
wall("Wall_East", X1, -HALF, X1 + 300, HALF)

c.set_tee(300, 0)

# ---- Plates. Two at the start: one on the ball's line (the first grille), one off it (the chamber's grille).
# One inside the chamber, on the ball's line (the cup's gate).
LINE = (900, 0)
SIDE = (800, 520)
INNER = (3250, 0)
line = c.switch(*LINE, radius=75, weight=True)
side = c.switch(*SIDE, radius=75, weight=True)
inner = c.switch(*INNER, radius=75, weight=True)

# ---- The first rock: one grille, on the ball's line.
W1 = 1500
wall("First_S", W1 - 60, -HALF, W1 + 60, -100)
wall("First_N", W1 - 60, 100, W1 + 60, HALF)
c.gate("FirstGrille", (W1, 0, 110), (70, 200, 220), line, grille=True)

# ---- The second rock, which closes the chamber: the chamber's grille to the north, the cup's gate on the line.
W2 = 2750
wall("Second_S", W2 - 60, -HALF, W2 + 60, -90)
wall("Second_M", W2 - 60, 90, W2 + 60, 380)
wall("Second_N", W2 - 60, 580, W2 + 60, HALF)
c.gate("ChamberGrille", (W2, 480, 110), (70, 200, 220), side, grille=True)
# (A grille too: a plain gate here would let the second machine simply be carried in along the ball's line.)
c.gate("CupGrille", (W2, 0, 110), (70, 180, 220), inner, grille=True)

c.set_cup(3700, 0, court=180)

STARTS = [(500, -420), (1100, -420)]
SPARE = [(2100, 450), (2100, -450), (3400, 480)]
for spot in STARTS + [LINE, SIDE, INNER] + SPARE:
    c.plinth(*spot, radius=65)
# Small fields: weights first. The two that end on the ball's line hand it on.
c.machine(*STARTS[0], strength=220, reach=800)
c.machine(*STARTS[1], strength=220, reach=800)
c.placement_area(100, -HALF + 100, X1 - 100, HALF - 100)
c.solution(0, [LINE, INNER], yaw=0, power=0.25)
c.extra["ambience"] = "canyon"
c.write()
