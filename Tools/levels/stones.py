"""Hole 8, Stepping Stones: the salt again, at noon. White crust, hard blue sky, short black shadows, and seven
plinths of black glass ringed with violet light.

It is the Salt Mirror's level script (Tools/levels/saltmirror.py) with its own course, sun and colours.

Run inside the editor:  Tools/regen.sh stones --no-materials
"""

import runpy

runpy.run_path("/Users/ken/Code/gravitygolf/Tools/levels/saltmirror.py", run_name="__main__", init_globals={"GG_VARIANT": {
    "level": "/Game/Levels/Stones",
    "course": "stones",
    "prefix": "Stones",
    "seed": 29,
    "glyph": (0.62, 0.42, 1.0),
    # High sun from behind the tee, a little to the south: the long wall's shadow is a narrow band on its far side,
    # and both lanes lie in the light. (At 310 degrees the wall's shadow covered the whole tee lane.)
    "sun": dict(sun_pitch=-52.0, sun_yaw=18.0, fog_density=0.004, sun_kelvin=6200.0, sky_intensity=1.3, exposure_bias=-0.2),
    "monolith_height": 1000,
    "web_ambient": 1.6,
    "web_tint": (0.9, 0.95, 1.0),
}})
