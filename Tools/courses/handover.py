#!/usr/bin/env python3
"""Hole 14, Hand Over Hand.

THE IDEA: "the one who got through holds the door for the one behind." A walled court cut in three by two walls. The
ball's way to the cup is one straight line through a grille in each wall, and both of those grilles open from plates
in the last yard, beyond both walls. So both machines have to end up there, and neither can get there alone.

Each wall has a second grille, to the side, opened from a plate back by the tee. Those get one machine through, as
on Wait Here: hold the first, carry the other to the middle, change plates, carry it on. That is half. The machine
still by the tee cannot follow that way, because nobody is left to hold a plate for it. What lets it in is the other
machine, from the far end: the first wall's own grille opens from a plate in the last yard. So the one who got
through stands on that plate, the one behind walks in to the middle, and then the first changes plates once more to
open the second wall. Seven carries, each of which shuts something.

It was found by trying every layout of this size (Tests/Sim/rooms.sh): of 360 with four plates, this and its three
mirror images need the most carries. Nothing in it is a matter of aim: both machines end on the ball's line.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from coursekit import Course  # noqa: E402

c = Course("handover", "Hand Over Hand", "HandOver", next_level="Wires")
X0, X1, HALF = 0, 4400, 700
c.box("Floor", ((X0 + X1) / 2, 0, -15), (X1 - X0 + 400, 2 * HALF + 400, 30), "green", "paving")
# Low walls: this hole is a plan made before the first carry, so all of it can be seen from the tee. (A wall knee
# high stops a carried machine as well as a tall one does.)
H = 110


def wall(name, x0, y0, x1, y1):
    c.box(name, ((x0 + x1) / 2, (y0 + y1) / 2, H / 2), (x1 - x0, y1 - y0, H), "sand", "wall")


# The court is walled all round: nothing is carried except through the grilles.
wall("Outer_S", X0 - 200, -HALF - 200, X1 + 200, -HALF)
wall("Outer_N", X0 - 200, HALF, X1 + 200, HALF + 200)
wall("Outer_W", X0 - 200, -HALF, X0, HALF)
wall("Outer_E", X1, -HALF, X1 + 200, HALF)

c.set_tee(1200, 0)

# ---- The plates. Two by the tee, which open the side grilles; two in the last yard, on the ball's line, which open
# the ball's own grilles. (In this order they get one, two, three and four dots.)
NEAR_S = (900, -400)
NEAR_N = (900, 400)
FAR_1 = (2700, 0)
FAR_2 = (3250, 0)
near_s = c.switch(*NEAR_S, radius=75, weight=True)
near_n = c.switch(*NEAR_N, radius=75, weight=True)
far_1 = c.switch(*FAR_1, radius=75, weight=True)
far_2 = c.switch(*FAR_2, radius=75, weight=True)

# ---- The first wall: a side grille to the north, opened from the plate beside it; the ball's grille on the line,
# opened from the far plate of the last yard.
W1 = 1500
wall("First_S", W1 - 50, -HALF, W1 + 50, -100)
wall("First_M", W1 - 50, 100, W1 + 50, 350)
wall("First_N", W1 - 50, 550, W1 + 50, HALF)
c.gate("First_Side", (W1, 450, 110), (60, 200, 220), near_n, grille=True)
c.gate("First_Ball", (W1, 0, 110), (60, 200, 220), far_2, grille=True)

# ---- The second wall: its side grille to the south, opened from the other plate by the tee; the ball's grille on
# the line, opened from the near plate of the last yard. The side grilles are on opposite sides, so no ball threads both.
W2 = 2300
wall("Second_S", W2 - 50, -HALF, W2 + 50, -550)
wall("Second_M", W2 - 50, -350, W2 + 50, -100)
wall("Second_N", W2 - 50, 100, W2 + 50, HALF)
c.gate("Second_Side", (W2, -450, 110), (60, 200, 220), near_s, grille=True)
c.gate("Second_Ball", (W2, 0, 110), (60, 200, 220), far_1, grille=True)

c.set_cup(3850, 0, court=200)

# The machines wait ahead of the tee, one each side. One plinth in the middle yard: somewhere to wait.
STARTS = [(1330, 300), (1330, -300)]
MIDDLE = (1900, 420)
for spot in STARTS + [NEAR_S, NEAR_N, FAR_1, FAR_2, MIDDLE]:
    c.plinth(*spot, radius=65)
# Small fields: these two are weights first. On the ball's line they hand it on, one to the next.
c.machine(*STARTS[0], strength=220, reach=700)
c.machine(*STARTS[1], strength=220, reach=700)
c.placement_area(100, -HALF + 100, X1 - 100, HALF - 100)
# Any gentle putt does it: straight within four degrees, at a sixth to a quarter of full strength.
c.solution(0, [FAR_2, FAR_1], yaw=0, power=0.2)
# The air of a great machine at rest, and the night's slow chords.
c.extra["ambience"] = "hum"
c.extra["music"] = "night"
c.write()
