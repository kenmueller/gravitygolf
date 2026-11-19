#!/usr/bin/env python3
"""Hole 9, The Looking Glass.

THE IDEA: "a mirror turns the ball a right angle." No field this time: the machine is a tall glass standing at 45
degrees, and a ball that meets it leaves at the angle it came in at. The galleries turn two corners. For each, the
glass has to stand where the line the ball comes along crosses the line it has to leave along, and be turned the right
way of its two. The first corner sends the ball north onto a pad at the end of the long gallery; then the glass is
carried on, and the second sends it south into the pocket where the cup is.

Where a field bends a ball by an amount you have to feel for, the glass is exact. That is the new thing to think
with: straight lines and right angles.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from coursekit import Course  # noqa: E402

c = Course("glass", "The Looking Glass", "Glass", next_level="Periscope")
WALL_H = 400
c.box("Floor", (1700, 1000, -15), (4000, 3600, 30), "green", "floor")


def wall(name, x0, y0, x1, y1):
    c.box(name, ((x0 + x1) / 2, (y0 + y1) / 2, WALL_H / 2), (x1 - x0, y1 - y0, WALL_H), "sand", "rock")


# Three galleries: one running east from the tee, one running north from its far end, and one running back west
# from the top of that, with a pocket opening south off it.
wall("South", -200, -600, 3600, -400)
wall("West", -200, -400, 0, 400)
wall("East", 3400, -400, 3600, 2600)
wall("Top", 1700, 2400, 3400, 2600)
wall("North_Near", 0, 400, 2600, 600)
wall("Between", 2400, 600, 2600, 1600)
wall("Pocket_Floor", 1700, 900, 2400, 1100)
wall("Pocket_West", 1700, 1100, 1900, 2400)

c.set_tee(400, 0)
# The pad at the top of the north gallery, with wet sand behind it against the end wall.
PAD = (3000, 2000)
c.safe_zone(*PAD, radius=190)
# (It starts close behind the pad's middle: a hard-hit ball runs nearly a metre into the sand before it stops, and
# has to stop inside the pad's ring.)
c.soft_patch("Backstop", (PAD[0], PAD[1] + 225, -15), (560, 270, 30), look="sand")
c.set_cup(2200, 1300, court=260)
c.mirror(800, -250)
c.placement_area(150, -300, 3300, 2300)
# The glass at each corner in turn, where the two gallery lines cross, and a straight swing down the gallery.
c.solution(0, [(3000, 0)], yaw=0, power=0.4, turns=[0])
c.solution(1, [(2200, 2000)], yaw=180, power=0.25, turns=[0])
c.extra["ambience"] = "canyon"
c.write()
