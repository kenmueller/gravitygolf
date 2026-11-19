#!/usr/bin/env python3
"""Hole 8, Stepping Stones.

THE IDEA: "the machine can only stand where the plinths are, so read the lines." Out on the salt a machine sinks
into the crust anywhere but on a plinth of black glass, and there are only seven. A ball that is dying inside a
machine's reach is drawn in and let go along the line through the machine, so the question on each swing is which
plinth puts that line where it needs to go. The first swing has to curl round the end of the long wall and stop on a
pad at the far end. The second comes all the way back: the cup is behind the long wall, near where you started, and
behind a second wall as well.

Each swing has exactly one plinth that works (checked: from each of the other six, no swing completes the leg).
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from coursekit import Course  # noqa: E402

LENGTH, HALF = 5200, 1100
c = Course("stones", "Stepping Stones", "Stones", next_level="Wires")

c.box("Crust", (LENGTH / 2, 0, -15), (LENGTH, 2 * HALF, 30), "green", "crust")
BRINE = 3000
c.box("BrineL", (LENGTH / 2, -(HALF + BRINE / 2), -15), (LENGTH + 2 * BRINE, BRINE, 30), "sand", "brine")
c.box("BrineR", (LENGTH / 2, HALF + BRINE / 2, -15), (LENGTH + 2 * BRINE, BRINE, 30), "sand", "brine")
c.box("BrineStart", (-BRINE / 2, 0, -15), (BRINE, 2 * HALF, 30), "sand", "brine")
c.box("BrineEnd", (LENGTH + BRINE / 2, 0, -15), (BRINE, 2 * HALF, 30), "sand", "brine")

THICK = 140
# The long wall: it runs east from beyond the west edge and stops short of the east end. South of it, the tee; north
# of it, the cup.
LONG_END = 3100
# (The walls are dead to the ball, like every wall since hole 3: nothing banks.)
c.box("LongWall", ((LONG_END - 600) / 2, 0, 300), (LONG_END + 600, THICK, 600), "sand", "monolith")
# A second wall in the north lane hides the cup from the pad. It leaves a gap beside the long wall.
SCREEN_X, SCREEN_GAP = 2300, 420
c.box("Screen", (SCREEN_X, (SCREEN_GAP + HALF + 600) / 2, 300), (THICK, HALF + 600 - SCREEN_GAP, 600), "sand", "monolith")

c.set_tee(500, -260)
# The pad, with a strip of wet salt behind it as a backstop: a ball let go past the far plinth is still moving fast,
# and the wet salt stops it on the pad's far edge.
PAD = (4560, 520)
c.safe_zone(*PAD, radius=190)
c.soft_patch("Backstop", (PAD[0] + 90 + 110, PAD[1] + 40, -15), (220, 620, 30), look="brine")
c.set_cup(1200, 760, court=300)
# The plinths. One by the tee, where the machine waits; one past the end of the long wall; one in the cup's chamber;
# and three that look useful and are not.
START = (800, -550)
PAST_THE_END = (3700, 150)
BY_THE_CUP = (1800, 500)
# (Each of the four decoys was checked: no swing completes either leg from it.)
for spot in (START, PAST_THE_END, BY_THE_CUP, (2950, -700), (4300, -500), (4900, -200), (600, 600)):
    c.plinth(*spot)
c.machine(*START)
c.placement_area(150, -HALF + 100, LENGTH - 150, HALF - 100)
# Out: a gentle swing along the lane curls north round the wall's end, past the plinth, and dies in the wet salt
# behind the pad. Back: a firm swing kisses the long wall, is drawn through the gap and the plinth, and drops.
c.solution(0, [PAST_THE_END], yaw=-1, power=0.225)
c.solution(1, [BY_THE_CUP], yaw=-165, power=0.35)
c.extra["ambience"] = "wind"
c.write()
