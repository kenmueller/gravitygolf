#!/usr/bin/env python3
"""Hole 6, Second Wind.

THE IDEA: "a pull is also a push." The way to the cup turns a hard corner and then runs the whole length of the
court. Only a slow ball can be bent round the corner, and a slow ball dies long before the cup. Two attractors: one
at the corner to turn the ball, one down the gallery to pull it back up to speed as it passes. A machine is an engine
as well as a bend. No plates, no gates: nothing here but the two machines and the distance.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from coursekit import Course  # noqa: E402

c = Course("brief", "Second Wind", "Brief", next_level="Colonnade")
X0, X1, Y0, Y1 = -2800, 4200, -900, 1900
WALL_H = 260

c.box("Paving", ((X0 + X1) / 2, (Y0 + Y1) / 2, -15), (X1 - X0, Y1 - Y0, 30), "green", "paving")


def wall(name, x0, y0, x1, y1, look="hedge"):
    c.box(name, ((x0 + x1) / 2, (y0 + y1) / 2, WALL_H / 2), (x1 - x0, y1 - y0, WALL_H), "sand", look)


wall("Hedge_S", X0 - 150, Y0 - 150, X1 + 150, Y0)
wall("Hedge_N", X0 - 150, Y1, X1 + 150, Y1 + 150)
wall("Hedge_W", X0 - 150, Y0, X0, Y1)
wall("Hedge_E", X1, Y0, X1 + 150, Y1)

# A long wall splits the court into a south strip (the tee) and a narrow north gallery (the cup). The only
# way between them is the gap at the west end.
wall("Divide", -1900, 150, X1, 270, look="wall")
# The gallery is narrow, so a ball coming up through the gap has to turn hard at once or die on its far wall.
GALLERY_N = 1050
wall("Gallery_N", X0, GALLERY_N, X1, GALLERY_N + 120, look="wall")
CUP_X = 3400

c.set_tee(-2350, -500)
c.set_cup(CUP_X, 660, court=380)
c.machine(-1500, -550, strength=420, reach=1900)
c.machine(-1200, -550, strength=420, reach=1900)
c.placement_area(X0 + 150, Y0 + 150, X1 - 150, GALLERY_N - 100)
c.solution(0, [(-450, 500), (1450, 450)], yaw=64, power=0.35)
c.extra["ambience"] = "night"
c.write()
