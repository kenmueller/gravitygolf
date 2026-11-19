"""Hole 5, The Long Way Round: the same red canyon as The Narrows, late in the day, with the sun low along the slot.

Collision geometry comes from Content/Courses/returning.json (written by Tools/courses/returning.py). The dressing
is Tools/levels/canyon.py, run with this hole's settings.

Run inside the editor:  Tools/regen.sh returning
"""

import runpy

runpy.run_path("/Users/ken/Code/gravitygolf/Tools/levels/canyon.py", run_name="__main__", init_globals={"GG_VARIANT": {
    "level": "/Game/Levels/Returning",
    "course": "returning",
    "door": (3100, 3900),
    "seed": 61,
    # Evening: the sun looks down the length of the slot from behind the tee, so the way ahead is lit and gold.
    "sun": dict(sun_lux=5.0, sun_pitch=-42.0, sun_yaw=8.0, fog_density=0.04, sun_kelvin=4300.0, sky_intensity=1.4, exposure_bias=0.0),
}})
