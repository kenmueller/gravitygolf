#!/usr/bin/env python3
"""Hole 4, Lantern Court.

THE IDEA: "open the gate first." The cup sits in a walled court behind a gate of light. The plate that opens the
gate lies behind the tee, the wrong way, with a safe pad beyond it. Swing one: roll over the plate and curl onto
the pad, which banks the open gate. Swing two: the long way back, round the hooked screen wall and in through
the gate. One machine, carried twice; the puzzle is seeing the order.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from coursekit import Course  # noqa: E402

c = Course("lantern", "Lantern Court", "Lantern", next_level="Returning")
X0, X1, Y0, Y1 = -2800, 4200, -900, 1900
WALL_H = 260

c.box("Paving", ((X0 + X1) / 2, (Y0 + Y1) / 2, -15), (X1 - X0, Y1 - Y0, 30), "green", "paving")


def wall(name, x0, y0, x1, y1, look="hedge"):
    c.box(name, ((x0 + x1) / 2, (y0 + y1) / 2, WALL_H / 2), (x1 - x0, y1 - y0, WALL_H), "sand", look)


# Soft hedges all round.
wall("Hedge_S", X0 - 150, Y0 - 150, X1 + 150, Y0)
wall("Hedge_N", X0 - 150, Y1, X1 + 150, Y1 + 150)
wall("Hedge_W", X0 - 150, Y0, X0, Y1)
wall("Hedge_E", X1, Y0, X1 + 150, Y1)

# The screen wall and its hook hide the court's gate from everywhere to the west.
wall("Screen", 600, 350, 3600, 470, look="wall")
wall("Hook", 600, 470, 720, 1050, look="wall")
# The court: cup inside, gate in its west face.
wall("Court_N", 2200, 1450, 3600, 1570, look="wall")
wall("Court_E", 3480, 470, 3600, 1450, look="wall")
wall("Court_W_Low", 2200, 470, 2320, 560, look="wall")
wall("Court_W_High", 2200, 1360, 2320, 1450, look="wall")

plate = c.switch(-1300, 350, radius=140)
c.gate("Gate", (2260, 960, 110), (60, 800, 220), plate)

c.set_tee(0, 0)
c.safe_zone(-2300, 0, radius=230, needs_switch=plate)
c.set_cup(2950, 960, court=430)
c.machine(300, -500)
c.placement_area(X0 + 150, Y0 + 150, X1 - 150, Y1 - 150)
# Verified answers, one per leg.
c.solution(0, [(-750, 200)], yaw=92, power=0.125)
c.solution(1, [(1950, 1350)], yaw=35, power=0.875)
c.extra["ambience"] = "night"
c.write()
