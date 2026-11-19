#!/usr/bin/env python3
"""Hole 12, After You.

THE IDEA: "the ball can open a door for a machine." A walled hall cut in two. This time one machine is a mirror, and
a mirror is too light to hold a plate down. So there is no leapfrog: the weight can let the mirror through the first
grille, but nothing the mirror stands on will ever let the weight follow.

What can is the ball. Beyond the wall lies a plate the ball presses, wired to a second grille, and a pad that keeps
it pressed. So the first swing is not toward the cup at all: weight on the near plate, mirror carried through and
turned to send the ball over that plate onto the pad. The second grille drops, the weight walks through it, and only
then can the last gate be held open while the mirror turns the ball into the cup.

Two machines, two legs, four carries, and the order is everything: mirror first, ball second, weight last.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from coursekit import Course  # noqa: E402

c = Course("afteryou", "After You", "AfterYou", next_level="WaitHere")
X0, X1, HALF = 0, 3700, 800
c.box("Floor", ((X0 + X1) / 2, 0, -15), (X1 - X0 + 400, 2 * HALF + 400, 30), "green", "floor")
WALL_H = 300


def wall(name, x0, y0, x1, y1):
    c.box(name, ((x0 + x1) / 2, (y0 + y1) / 2, WALL_H / 2), (x1 - x0, y1 - y0, WALL_H), "sand", "wall")


# Walled all round: nothing is carried except through the grilles.
wall("Outer_S", X0 - 200, -HALF - 200, X1 + 200, -HALF)
wall("Outer_N", X0 - 200, HALF, X1 + 200, HALF + 200)
wall("Outer_W", X0 - 200, -HALF, X0, HALF)
wall("Outer_E", X1, -HALF, X1 + 200, HALF)

c.set_tee(300, 0)

# ---- The near plate (a weight), the plate the ball presses, and the far plate (a weight) that holds the cup's gate.
NEAR = (800, -550)
FAR = (2700, 450)
near = c.switch(*NEAR, radius=75, weight=True)
ball_plate = c.switch(2300, -330, radius=70)
far = c.switch(*FAR, radius=75, weight=True)

# ---- The wall across the hall: the ball's grille on the tee's line, and a second grille that only the ball opens.
W1 = 1500
wall("Cross_S", W1 - 50, -HALF, W1 + 50, -100)
wall("Cross_M", W1 - 50, 100, W1 + 50, 400)
wall("Cross_N", W1 - 50, 600, W1 + 50, HALF)
c.gate("FirstGrille", (W1, 0, 110), (60, 200, 220), near, grille=True)
c.gate("SecondGrille", (W1, 500, 110), (60, 200, 220), ball_plate, grille=True)

# ---- The pad the first swing ends on, south of the ball's plate, with sand behind it against the outer wall.
PAD = (2300, -600)
c.safe_zone(*PAD, radius=170, needs_switch=ball_plate)
c.soft_patch("Backstop", (PAD[0], -745, -15), (440, 110, 30), look="sand")

# ---- The cup's own wall, with a plain gate in it held by the far plate.
wall("Inner_W", 2620, -250, 3110, -150)
wall("Inner_E", 3290, -250, X1, -150)
c.gate("CupGate", (3200, -200, 110), (180, 60, 220), far)
c.set_cup(3200, 250, court=180)

# ---- Plinths. The mirror's two that matter: where the tee's line turns south, and where the pad's line turns north.
TURN_ONE = (2300, 0)
TURN_TWO = (3200, -600)
STARTS = [(500, 450), (500, -250)]
SPARE = [(2900, 0), (2800, -600), (1900, 450)]
for spot in STARTS + [NEAR, FAR, TURN_ONE, TURN_TWO] + SPARE:
    c.plinth(*spot, radius=65)
# The attractor is the weight. Its field is small and both its plates are out of the ball's way: here it only holds.
c.machine(*STARTS[0], strength=220, reach=450)
c.mirror(*STARTS[1])
c.placement_area(100, -HALF + 100, X1 - 100, HALF - 100)
c.solution(0, [NEAR, TURN_ONE], yaw=0, power=0.35, turns=[0, 1])
c.solution(1, [FAR, TURN_TWO], yaw=0, power=0.3, turns=[0, 0])
c.extra["ambience"] = "water"
c.write()
