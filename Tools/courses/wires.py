#!/usr/bin/env python3
"""Hole 9, Crossed Wires. The last hole.

THE IDEA: "a weight opens a gate, a machine pulls along a line: find the plate whose line runs through its own
gate." Two walls cross the hall, each with three gates of light, each gate a different colour. In front of each wall
lie three heavy plates in the same three colours: a plate holds open the gate of its colour for as long as a machine
stands on it. There is one machine. Standing on a plate it does two things at once: it opens that plate's gate, and it
draws the ball in and lets it go along the line from the ball through itself. The wiring is crossed, so for two of
the three plates that line ends at a gate the plate does not open. One plate's line runs through its own gate.

Everything the course has taught is in it: the pull (1), the pad (2), the plate and gate (4), the weight (7), the
plinths and the line (8). And nothing here is a matter of aim: any soft swing toward the right plate goes in.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from coursekit import Course  # noqa: E402

c = Course("wires", "Crossed Wires", "Wires")
WALL_H = 110
LENGTH = 5400
c.box("Floor", (LENGTH / 2, 0, -15), (LENGTH + 400, 2000, 30), "green", "floor")


def wall(name, x0, y0, x1, y1):
    c.box(name, ((x0 + x1) / 2, (y0 + y1) / 2, WALL_H / 2), (x1 - x0, y1 - y0, WALL_H), "sand", "wall")


wall("Wall_South", -200, -900, LENGTH + 200, -700)
wall("Wall_North", -200, 700, LENGTH + 200, 900)
wall("Wall_West", -200, -700, 0, 700)
wall("Wall_East", LENGTH, -700, LENGTH + 200, 700)


def cross_wall(name, x, gates):
    """A wall across the hall, solid except for its gates (each a (low y, high y) pair, south to north)."""
    edges = [-700] + [v for gate in gates for v in gate] + [700]
    for i in range(0, len(edges), 2):
        if edges[i + 1] > edges[i]:
            wall(f"{name}{i // 2}", x - 60, edges[i], x + 60, edges[i + 1])


TEE = (400, 0)
c.set_tee(*TEE)

# The first wall. Plates south to north: A, B, C. Gates south to north: 1, 2, 3.
# The plates lie well short of the wall and the pad close behind it, so the lines that can thread a plate and its
# gate fan out only a little: C's fan covers the pad, and A's and B's miss it entirely. (With the plates near the wall
# a ball killed against the south wall could be reeled through A, through A's gate, and onto the pad: a second answer.)
X1, PLATES1 = 1700, 900
GATES1 = [(-480, -300), (-90, 90), (300, 480)]
cross_wall("First", X1, GATES1)
A, B, C = (PLATES1, -150), (PLATES1, 0), (PLATES1, 150)
plate_a = c.switch(*A, radius=70, weight=True)
plate_b = c.switch(*B, radius=70, weight=True)
plate_c = c.switch(*C, radius=70, weight=True)
# From the tee, the line through A runs to gate 1, through B to gate 2, through C to gate 3.
# Crossed: A opens gate 2, B opens gate 1. Only C opens the gate its own line runs to.
for name, gate, plate in (("Gate1", GATES1[0], plate_b), ("Gate2", GATES1[1], plate_a), ("Gate3", GATES1[2], plate_c)):
    c.gate(name, (X1, (gate[0] + gate[1]) / 2, 110), (60, gate[1] - gate[0], 220), plate)

# The pad lies on the line from the tee through C, beyond gate 3, with wet sand behind it.
PAD = (2200, 540)
c.safe_zone(*PAD, radius=170)
c.soft_patch("Backstop", (PAD[0] + 225, PAD[1] + 85, -15), (190, 150, 30), look="sand")

# The second wall. Plates south to north: D, E, F. Gates south to north: 4, 5, 6.
# (These plates are 10 m from the pad. At 13 m the pull only just beat the roll of the ground and few swings worked.)
X2, PLATES2 = 3900, 3200
GATES2 = [(-620, -420), (-280, -80), (230, 430)]
cross_wall("Second", X2, GATES2)
D, E, F = (PLATES2, -84), (PLATES2, 116), (PLATES2, 416)
plate_d = c.switch(*D, radius=70, weight=True)
plate_e = c.switch(*E, radius=70, weight=True)
plate_f = c.switch(*F, radius=70, weight=True)
# From the pad, the line through D runs to gate 4, through E to gate 5, through F to gate 6.
# Crossed: D opens gate 6, F opens gate 4. Only E opens the gate its own line runs to.
for name, gate, plate in (("Gate4", GATES2[0], plate_f), ("Gate5", GATES2[1], plate_e), ("Gate6", GATES2[2], plate_d)):
    c.gate(name, (X2, (gate[0] + gate[1]) / 2, 110), (60, gate[1] - gate[0], 220), plate)

# The cup lies on the line from the pad through E, beyond gate 5.
c.set_cup(4550, -455, court=300)

START = (500, 520)
for spot in (START, A, B, C, D, E, F):
    c.plinth(*spot, radius=65)
c.machine(*START)
c.placement_area(150, -600, LENGTH - 150, 600)
# Out: a soft swing at the north plate. The machine on it holds its gate open, draws the ball in and lets it go
# through the gate onto the pad. Back the same way with the middle plate of the second three.
c.solution(0, [C], yaw=16, power=0.1)
c.solution(1, [E], yaw=-24, power=0.175)
c.write()
