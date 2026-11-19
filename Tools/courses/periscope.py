#!/usr/bin/env python3
"""Hole 10, Periscope.

THE IDEA: "two mirrors draw a path, and the path has to pass through things in order." Out on the salt stand nine
plinths in a square. Beyond them a black wall with one slit in it, and the slit is barred until the ball has rolled
over a plate that lies between two of the plinths. So the ball has to go out, turn, turn again, cross the plate, and
only then arrive at the slit: an S. One pair of plinths, each glass turned one way, draws it.

Beyond the wall the cup is hidden behind another, open only round its far end, and the wet salt behind the pad
stops anything sent straight on. The same two mirrors, a different letter: down, along and up again, a U. This time
the two glasses have to face different ways.

With the mirrors on plinths nothing is a matter of aim or of placing to the centimetre. All of it is working out the
path: which two corners, and which way each glass faces.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from coursekit import Course  # noqa: E402

c = Course("periscope", "Periscope", "Periscope", next_level="Wires")
X0, X1, Y0, Y1 = 0, 6200, -500, 1900
c.box("Crust", ((X0 + X1) / 2, (Y0 + Y1) / 2, -15), (X1 - X0, Y1 - Y0, 30), "green", "crust")
BRINE = 3000
c.box("BrineS", ((X0 + X1) / 2, Y0 - BRINE / 2, -15), (X1 - X0 + 2 * BRINE, BRINE, 30), "sand", "brine")
c.box("BrineN", ((X0 + X1) / 2, Y1 + BRINE / 2, -15), (X1 - X0 + 2 * BRINE, BRINE, 30), "sand", "brine")
c.box("BrineW", (X0 - BRINE / 2, (Y0 + Y1) / 2, -15), (BRINE, Y1 - Y0, 30), "sand", "brine")
c.box("BrineE", (X1 + BRINE / 2, (Y0 + Y1) / 2, -15), (BRINE, Y1 - Y0, 30), "sand", "brine")

THICK = 120


def wall(name, x0, y0, x1, y1):
    # Dead to the ball, like every wall since hole 3: only the mirrors turn it.
    c.box(name, ((x0 + x1) / 2, (y0 + y1) / 2, 150), (x1 - x0, y1 - y0, 300), "sand", "monolith")


TEE = (400, 0)
c.set_tee(*TEE)

# ---- The square of plinths, the plate between two of them, and the wall with its slit.
COLUMNS = (1600, 2400, 3200)
ROWS = (0, 700, 1400)
SQUARE = [(x, y) for x in COLUMNS for y in ROWS]
# The plate lies on the middle row before the middle column: a path that joins the row any later misses it.
plate = c.switch(2000, 700, radius=70)
GATE_X = 3600
wall("GateWall_S", GATE_X - THICK / 2, Y0 - 400, GATE_X + THICK / 2, 610)
wall("GateWall_N", GATE_X - THICK / 2, 790, GATE_X + THICK / 2, Y1 + 400)
c.gate("Gate", (GATE_X, 700, 110), (60, 180, 220), plate)

# The pad beyond the slit, with wet salt behind it.
PAD = (4200, 700)
c.safe_zone(*PAD, radius=190)
c.soft_patch("Backstop", (PAD[0] + 195, PAD[1], -15), (210, 440, 30), look="brine")

# ---- Beyond: a wall east of the pad, open only round its south end, and the cup behind it.
# (The wet salt behind the pad stops anything sent straight on; the next swing has to leave the pad sideways.)
BLIND_X = 4650
wall("Blind", BLIND_X - THICK / 2, 250, BLIND_X + THICK / 2, Y1 + 400)
c.set_cup(5050, 500, court=200)
BEYOND = [(4200, 0), (5050, 0), (5650, 0), (4200, 1400), (5650, 1000)]

STARTS = [(700, -350), (700, 350)]
for spot in STARTS + SQUARE + BEYOND:
    c.plinth(*spot, radius=65)
c.mirror(*STARTS[0])
c.mirror(*STARTS[1])
c.placement_area(150, Y0 + 100, X1 - 150, Y1 - 100)
# The S: east to the first column, north to the middle row, east over the plate and through the slit.
c.solution(0, [(1600, 0), (1600, 700)], yaw=0, power=0.6, turns=[0, 0])
# The U: south to the bottom row, east past the end of the blind wall, and north again to the cup.
c.solution(1, [(4200, 0), (5050, 0)], yaw=-90, power=0.35, turns=[1, 0])
# Open salt has wind, not crickets; the music is the night's.
c.extra["ambience"] = "wind"
c.extra["music"] = "night"
c.write()
