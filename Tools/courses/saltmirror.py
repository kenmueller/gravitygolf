#!/usr/bin/env python3
"""Hole 2, Salt Mirror.

THE IDEA: "the machine is yours to move, and a pad lets you stop." A hard white salt crust runs across an endless
mirror of brine; anything that rolls off the crust sinks into the soft wet salt and stops. Two black monoliths
block the way, one from each side. A soft pad lies behind the first, the cup behind the second. One machine has to
serve both turns: bend the ball onto the pad, then carry the machine on and bend the next swing the other way.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from coursekit import Course  # noqa: E402

LENGTH, HALF = 6400, 950
c = Course("saltmirror", "Salt Mirror", "SaltMirror", next_level="Canyon")

# The hard crust the ball rolls on, and the soft wet salt around it that swallows it.
c.box("Crust", (LENGTH / 2, 0, -15), (LENGTH, 2 * HALF, 30), "green", "crust")
BRINE = 3000
c.box("BrineL", (LENGTH / 2, -(HALF + BRINE / 2), -15), (LENGTH + 2 * BRINE, BRINE, 30), "sand", "brine")
c.box("BrineR", (LENGTH / 2, HALF + BRINE / 2, -15), (LENGTH + 2 * BRINE, BRINE, 30), "sand", "brine")
c.box("BrineStart", (-BRINE / 2, 0, -15), (BRINE, 2 * HALF, 30), "sand", "brine")
c.box("BrineEnd", (LENGTH + BRINE / 2, 0, -15), (BRINE, 2 * HALF, 30), "sand", "brine")

# Two monoliths. The first leaves a gap on the right, the second on the left.
THICK = 140
c.box("MonolithA", (1900, (-HALF + 250) / 2 - 150, 300), (THICK, HALF + 250 + 300, 600), look="monolith")
c.box("MonolithB", (4300, (HALF - 250) / 2 + 150, 300), (THICK, HALF + 250 + 300, 600), look="monolith")

c.set_tee(500, 380)
c.safe_zone(2950, -640, radius=180)
c.set_cup(5600, 420)
c.machine(700, -450)
c.placement_area(200, -HALF + 150, LENGTH - 200, HALF - 150)
c.write()
