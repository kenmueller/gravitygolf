#!/usr/bin/env python3
"""Hole 14, Crossed Wires. The last hole.

THE IDEA: "two machines, two jobs, the wires are crossed, and every gate is a grille." Two walls cross the hall, each
with gates of light in different colours. What opens a gate is a heavy plate of its colour, a long way behind the
ball, and it stays down only while a machine stands on it: that is the attractor's job here, to be a weight. The
plates do not lie in the order of their gates, so the colours have to be read. Beyond each wall the target is in the
wall's shadow: no straight shot through any gate reaches it. That is the mirror's job: to stand on the ball's line
beyond the gate, level with the target, and turn the ball onto it.

Every gate has rails, so the gate the weight opens is also the only way the mirror gets through: the weight first,
then the mirror. And the weight itself can never follow, because nothing a mirror stands on holds a plate. The first
wall has a fourth opening for that, which no plate behind the tee opens. The ball does, by rolling over a plate on
its way to the pad. So the first swing does two things: it reaches the pad, and it lets the weight through.

So each swing is choices that have to agree: which gate the straight line goes through, which plate has that gate's
colour, which plinth and which way round for the mirror, and in what order the two are carried.

Everything the course has taught is in it: the pad (2), two machines (3), the plate and the gate (4), the weight (7),
the plinths (8), the mirror (9), the grille and the order of carrying (11), the door the ball opens (12). Nothing in
it is a matter of aim.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from coursekit import Course  # noqa: E402

c = Course("wires", "Crossed Wires", "Wires")
WALL_H = 110
LENGTH = 4400
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

# ---- The first wall. Its gates are narrow and far apart, so most of the hall beyond is in its shadow.
X1 = 1700
GATES1 = [(-600, -420), (-90, 90), (420, 600)]
# A fourth opening, between the middle gate and the north one: the weight's own door. No plate behind the tee opens
# it. The ball does, by rolling over a plate on its way to the pad.
SERVICE = (190, 330)
cross_wall("First", X1, GATES1[:2] + [SERVICE] + GATES1[2:])
# The plates behind the tee, south to north. Crossed: the south plate opens the north gate, the middle plate the south
# gate, the north plate the middle gate.
BANK1 = [(170, -330), (170, 0), (170, 330)]
plates1 = [c.switch(*spot, radius=70, weight=True) for spot in BANK1]
for name, gate, plate in (("Gate1", GATES1[0], plates1[1]), ("Gate2", GATES1[1], plates1[2]), ("Gate3", GATES1[2], plates1[0])):
    c.gate(name, (X1, (gate[0] + gate[1]) / 2, 110), (60, gate[1] - gate[0], 220), plate, grille=True)
# Plinths for the mirror beyond the wall, on the straight line through the middle gate.
MIRRORS1 = [(2100, 0), (2500, 0), (2900, 0)]
# The pad is north of the second of them, in the wall's shadow, with wet sand behind it.
PAD = (2500, 400)
door_plate = c.switch(2500, 170, radius=60)
c.gate("Service", (X1, (SERVICE[0] + SERVICE[1]) / 2, 110), (60, SERVICE[1] - SERVICE[0], 220), door_plate, grille=True)
c.safe_zone(*PAD, radius=190, needs_switch=door_plate)
c.soft_patch("Backstop", (PAD[0], PAD[1] + 195, -15), (520, 210, 30), look="sand")

# ---- The second wall, played from the pad: the straight line runs east through its north gate. This wall has two
# gates and two plates. (With a middle gate there was a second answer: play the ball south onto the first wall's
# mirror plinth and turn it east through the middle of both walls.) It stands close to the pad: a mirrored shot at a
# cup is a small target, and gets smaller with distance.
X2 = 3100
GATES2 = [(-600, -420), (310, 490)]
cross_wall("Second", X2, GATES2)
# Its plates lie behind the pad, against the first wall. Crossed again: the south plate opens the north gate.
BANK2 = [(1930, -500), (1930, -250)]
plates2 = [c.switch(*spot, radius=70, weight=True) for spot in BANK2]
for name, gate, plate in (("Gate4", GATES2[0], plates2[1]), ("Gate5", GATES2[1], plates2[0])):
    c.gate(name, (X2, (gate[0] + gate[1]) / 2, 110), (60, gate[1] - gate[0], 220), plate, grille=True)
MIRRORS2 = [(3500, 400), (3900, 400)]
# The cup is south of the first of them, in the second wall's shadow.
c.set_cup(3500, 80, court=220)

STARTS = [(560, 540), (560, -540)]
for spot in STARTS + BANK1 + MIRRORS1 + BANK2 + MIRRORS2:
    c.plinth(*spot, radius=65)
c.machine(*STARTS[0])   # the attractor: here, a weight
c.mirror(*STARTS[1])
c.placement_area(150, -600, LENGTH - 150, 600)
# Out: the weight on the north plate behind the tee (the middle gate's colour), the mirror on the second plinth beyond
# the wall, turned to send the ball north. Then: the weight on the south plate of the second pair (the north gate's
# colour), the mirror on the near plinth beyond the second wall, turned to send the ball south.
c.solution(0, [BANK1[2], MIRRORS1[1]], yaw=0, power=0.4, turns=[0, 0])
c.solution(1, [BANK2[0], MIRRORS2[0]], yaw=0, power=0.25, turns=[0, 1])
c.extra["ambience"] = "night"
c.write()
