"""Hole 13, Wait Here: the canyon at night. The moon almost overhead, blue light on red rock, and three grilles of
light across the canyon floor.

It is the canyon's level script (Tools/levels/canyon.py) with its own course and a night sky, and no doorway.

Run inside the editor:  Tools/regen.sh waithere --no-materials
"""

import runpy

runpy.run_path("/Users/ken/Code/gravitygolf/Tools/levels/canyon.py", run_name="__main__", init_globals={"GG_VARIANT": {
    "level": "/Game/Levels/WaitHere",
    "course": "waithere",
    "door": None,
    "center": (2100, 0),
    "seed": 149,
    # The moon nearly overhead and a little behind the tee: the walls are 19 m tall, and anything lower leaves the
    # floor black. Brighter than a real night, so the sand reads.
    "night": dict(moon_pitch=-80.0, moon_yaw=20.0, moon_lux=1.6, fog_density=0.012, exposure_bias=-0.1),
    "web_ambient": 2.4,
    "web_tint": (0.72, 0.82, 1.0),
}})
