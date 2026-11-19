"""Hole 9, The Looking Glass: the canyon in the morning, three galleries of red rock turning two corners.

It is the canyon's level script (Tools/levels/canyon.py) with its own course, sun and outer walls, and no doorway.

Run inside the editor:  Tools/regen.sh glass --no-materials
"""

import runpy

runpy.run_path("/Users/ken/Code/gravitygolf/Tools/levels/canyon.py", run_name="__main__", init_globals={"GG_VARIANT": {
    "level": "/Game/Levels/Glass",
    "course": "glass",
    "door": None,
    "outer": ("South", "West", "East", "Top"),
    "center": (1700, 1000),
    "seed": 61,
    # Near noon: the sun stands almost overhead, so it reaches the floor of all three galleries between walls 19 m
    # tall. (At 46 degrees the corner where the glass has to stand was in black shadow.)
    "sun": dict(sun_pitch=-76.0, sun_yaw=20.0, fog_density=0.03, sun_kelvin=5900.0, sky_intensity=1.7, exposure_bias=0.3),
    "web_ambient": 3.6,
    "web_tint": (1.0, 0.82, 0.66),
}})
