"""Hole 11, Crossed Wires: the hall of columns again, at night. The moon on the water, and five gates of light in five
colours, each mirrored in the floor.

It is the Colonnade's level script (Tools/levels/colonnade.py) with its own course, sky and colours.

Run inside the editor:  Tools/regen.sh wires --no-materials
"""

import runpy

runpy.run_path("/Users/ken/Code/gravitygolf/Tools/levels/colonnade.py", run_name="__main__", init_globals={"GG_VARIANT": {
    "level": "/Game/Levels/Wires",
    "course": "wires",
    "prefix": "Wires",
    "seed": 71,
    "glyph": (0.75, 0.85, 1.0),
    # A high moon behind the tee's right shoulder: enough light to read the floor, dark enough that the gates are the
    # brightest things in the hall.
    "night": dict(moon_pitch=-48.0, moon_yaw=25.0, moon_lux=0.9, fog_density=0.012, exposure_bias=-0.6),
    "web_ambient": 3.0,
    "web_tint": (0.75, 0.85, 1.0),
}})
