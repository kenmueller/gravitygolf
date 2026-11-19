"""Hole 11, Leapfrog: the lantern court a third time, in the last hour of the night. Rose lanterns, a low moon ahead,
two walls across the court and three gates of light in them, two of them railed.

Collision geometry comes from Content/Courses/leapfrog.json (written by Tools/courses/leapfrog.py). The dressing is
Tools/levels/lantern.py, run with this hole's settings.

Run inside the editor:  Tools/regen.sh leapfrog --no-materials
"""

import runpy

runpy.run_path("/Users/ken/Code/gravitygolf/Tools/levels/lantern.py", run_name="__main__", init_globals={"GG_VARIANT": {
    "level": "/Game/Levels/Leapfrog",
    "course": "leapfrog",
    "prefix": "Leapfrog",
    "seed": 113,
    "lantern_color": (1.0, 0.42, 0.55),
    "far_color": (1.0, 0.5, 0.6),
    # The moon low ahead and to the right, so the two cross walls are lit along their length, not across it.
    "moon": dict(moon_pitch=-34.0, moon_yaw=110.0, moon_lux=0.8),
    # Lit lanterns over each of the three yards and over each gate.
    "lit": [(700, 350, 330), (900, -420, 330), (1500, 450, 380), (1500, 0, 380), (2000, 380, 330), (2550, -380, 330),
            (3000, 0, 380), (3500, -350, 330)],
}})
